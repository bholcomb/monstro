// Console checks against the real native library, like Mobius.Tests.
using System;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using Monstro;

unsafe class Program
{
    static int _failures;
    static int _checks;
    static int _callbacks;
    static void Check(string name, bool ok)
    { _checks++; if (!ok) { _failures++; Console.WriteLine("FAIL: " + name); } }
    static void Throws<T>(string name, Action action) where T : Exception
    {
        try { action(); Check(name, false); }
        catch (T) { Check(name, true); }
    }

    [DllImport("monstro-binding-test", CallingConvention = CallingConvention.Cdecl)]
    static extern nuint monstro_binding_sizeof([MarshalAs(UnmanagedType.LPUTF8Str)] string name);
    [DllImport("monstro-binding-test", CallingConvention = CallingConvention.Cdecl)]
    static extern nuint monstro_binding_offsetof([MarshalAs(UnmanagedType.LPUTF8Str)] string name, [MarshalAs(UnmanagedType.LPUTF8Str)] string field);
    [DllImport("monstro-binding-test", CallingConvention = CallingConvention.Cdecl)]
    static extern int monstro_binding_constant([MarshalAs(UnmanagedType.LPUTF8Str)] string name);

    [UnmanagedCallersOnly(CallConvs = new[] { typeof(CallConvCdecl) })]
    static void DrawCallback(IntPtr userData, float* clip, int layer)
    {
        if (userData == (IntPtr)42 && clip != null && float.IsFinite(clip[0]) && layer >= 0)
            _callbacks++;
    }

    static int Main(string[] args)
    {
        if (args.Length != 2) { Console.Error.WriteLine("Usage: Monstro.Tests <native-library> <font.ttf>"); return 2; }
        // Every exported declaration must be callable, not just compile in C#.
        IntPtr library = NativeLibrary.Load(args[0]);
        try
        {
            var imports = typeof(Native).GetMethods().Where(m => m.GetCustomAttribute<DllImportAttribute>() != null).ToArray();
            foreach (var method in imports)
                Check("export " + method.Name, NativeLibrary.TryGetExport(library, method.Name, out _));
            foreach (var type in typeof(Native).Assembly.GetTypes().Where(t => t.IsPublic && t.IsValueType && t.Name.StartsWith("Monstro")))
            {
                int size = type.IsEnum ? sizeof(int) : Marshal.SizeOf(type);
                Check("sizeof " + type.Name, (nuint)size == monstro_binding_sizeof(type.Name));
                if (!type.IsEnum)
                    foreach (var field in type.GetFields(BindingFlags.Public | BindingFlags.Instance))
                        Check("offsetof " + type.Name + "." + field.Name,
                              (nuint)Marshal.OffsetOf(type, field.Name) == monstro_binding_offsetof(type.Name, field.Name));
            }
            foreach (var field in typeof(Native).GetFields().Where(f => f.IsLiteral && f.FieldType.IsEnum))
                Check("constant " + field.Name, Convert.ToInt32(field.GetRawConstantValue()) == monstro_binding_constant(field.Name));
        }
        finally { NativeLibrary.Free(library); }

        // Opaque C pointers are distinct handle types, never a bare IntPtr.
        var handleTypes = new[] { typeof(MonstroEnv), typeof(MonstroContext), typeof(MonstroFont), typeof(MonstroTheme), typeof(MonstroCanvas), typeof(MonstroFileDialogState) };
        Check("typed handles", handleTypes.All(h => h.IsValueType && Marshal.SizeOf(h) == IntPtr.Size));
        var nativeMethods = typeof(Native).GetMethods().Where(m => m.GetCustomAttribute<DllImportAttribute>() != null).ToArray();
        Check("no IntPtr in the C API mirror", nativeMethods.All(m => m.ReturnType != typeof(IntPtr) && m.GetParameters().All(p => p.ParameterType != typeof(IntPtr))));
        Check("init returns an environment", typeof(Native).GetMethod("monstro_init")!.ReturnType == typeof(MonstroEnv));
        Check("context create takes an environment", typeof(Native).GetMethod("monstro_context_create")!.GetParameters()[0].ParameterType == typeof(MonstroEnv));

        using (var env = new Env())
        using (var ui = env.CreateContext())
        {
            Throws<InvalidOperationException>("widget before frame", () => ui.Button("early"));
            Throws<InvalidOperationException>("end before begin", () => ui.EndFrame());
            Throws<ArgumentOutOfRangeException>("non-finite dimensions", () => ui.BeginFrame(float.NaN, 480, 0.01f));
            for (int frame = 0; frame < 2; frame++)
            {
                ui.BeginFrame(640, 480, 1.0f / 60);
                Throws<InvalidOperationException>("double begin", () => ui.BeginFrame(640, 480, 0.01f));
                ui.DrawTriangle(new() { x = 10, y = 10 }, new() { x = 100, y = 10 }, new() { x = 10, y = 100 }, 0xffffffff);
                DrawData data = ui.EndFrame();
                Check("frame resets vertex/index buffers", data.Vertices.Length == 3 && data.Indices.Length == 3);
                Check("triangle position by-value struct", data.Vertices[0].position[0] == 10 && data.Vertices[0].position[1] == 10);
                var commands = data.Commands;
                Check("draw commands", commands.Length == 1 && commands[0].index_count == 3 && commands[0].vertex_count == 3);
                bool inRange = true;
                foreach (uint index in data.Indices) inRange &= index < commands[0].vertex_count;
                Check("indices count from the command's vertex offset", inRange);
                Check("a frame unlike the last is changed", data.Changed);
                if (frame == 0)
                {
                    ulong firstFrame = data.FrameId;
                    ui.BeginFrame(640, 480, 0);
                    Check("the view is stale once its canvas builds another frame", !data.IsValid);
                    bool threw = false;
                    try { _ = data.Vertices.Length; }
                    catch (InvalidOperationException) { threw = true; }
                    Check("a stale view throws", threw);
                    DrawData empty = ui.EndFrame();
                    Check("empty frame", empty.Indices.Length == 0 && empty.FrameId == firstFrame + 1);
                }
            }

            // Input waits for the next frame.
            ui.SetMousePosition(23, 45);
            ui.SetKey(MonstroKey.A, true);
            Check("input is queued", !Native.monstro_is_key_down(ui.Handle, MonstroKey.A));
            ui.BeginFrame(640, 480, 1.0f / 60);
            ui.EndFrame();
            MonstroVec2 mouse = Native.monstro_get_mouse_pos(ui.Handle);
            Check("struct return", mouse.x == 23 && mouse.y == 45);
            Check("one-byte bool true", Native.monstro_is_key_down(ui.Handle, MonstroKey.A));
            ui.SetKey(MonstroKey.A, false);
            ui.BeginFrame(640, 480, 1.0f / 60);
            ui.EndFrame();
            Check("one-byte bool false", !Native.monstro_is_key_down(ui.Handle, MonstroKey.A));
            ui.AddInputText("héλlo 🚀");
            Check("UTF-8 input/result", Native.Utf8(Native.monstro_get_input_text(ui.Handle)) == "héλlo 🚀");

            ulong versionBefore = env.Atlas.Version;
            Font font = env.LoadFont("sans", args[1], 18);
            Font fromBytes = env.LoadFont("sans-bytes", File.ReadAllBytes(args[1]), 14);
            AtlasView atlas = env.Atlas;
            Check("one RGBA8 atlas", atlas.Format == MonstroTextureFormat.Rgba8 && atlas.Pixels.Length == atlas.Width * atlas.Height * 4);
            Check("atlas version follows loads", atlas.Version > versionBefore);
            Check("default font is the first loaded", env.DefaultFont is Font d && d.Handle == font.Handle);
            Check("font lookup", env.FindFont("sans-bytes", 14) is Font f && f.Handle == fromBytes.Handle && fromBytes.Name == "sans-bytes");
            env.AtlasTexture = 123;
            Check("atlas texture ID", env.AtlasTexture == 123);
            MonstroVec2 measured = font.Measure("café");
            Check("UTF-8 font measurement", measured.x > 0 && measured.y > 0);
            MonstroGlyph glyph;
            Check("glyph output struct", Native.monstro_font_get_glyph(font.Handle, (uint)'A', &glyph) && glyph.advance[0] > 0);
            Throws<InvalidOperationException>("font failure", () => env.LoadFont("missing", "no-such-font.ttf", 18));

            MonstroSettings settings = env.Settings;
            settings.key_repeat_delay = 0.5f;
            env.Settings = settings;
            Check("settings round trip", env.Settings.key_repeat_delay == 0.5f);

            ui.BeginFrame(640, 480, 1.0f / 60);
            Throws<InvalidOperationException>("environment read-only during a frame", () => env.LoadFont("late", args[1], 12));
            ui.SetNextWindowPosition(10, 10);
            ui.SetNextWindowSize(300, 350);
            if (ui.BeginWindow("héllo"))
            {
                ui.Label("café");
                Check("button no input", !ui.Button("press"));
                bool enabled = true;
                Check("checkbox pointer", !ui.Checkbox("enabled", ref enabled) && enabled);
                float speed = 0.25f;
                Check("float pointer", !ui.SliderFloat("speed", ref speed, 0, 1) && speed == 0.25f);
                int count = 2;
                ui.SliderInt("count", ref count, 0, 10);
                Check("int pointer", count == 2);
                string text = "café 🚀";
                ui.InputText("text", ref text, 64);
                Check("mutable UTF-8 buffer", text == "café 🚀");
                Throws<ArgumentOutOfRangeException>("UTF-8 byte capacity", () => ui.InputText("small", ref text, 5));
            }
            ui.EndWindow();
            Native.monstro_draw_custom(Native.monstro_get_window_canvas(ui.Handle), &DrawCallback, (void*)42);
            DrawData draw = ui.EndFrame();
            Check("widgets generate geometry", draw.Vertices.Length > 3 && draw.Indices.Length > 3);
            Check("text and fills batch on the atlas", draw.Commands.ToArray().Count(c => c.texture_id == 123) >= 1 &&
                                                       draw.Commands.ToArray().All(c => c.custom_draw != null || c.texture_id == 123));
            GC.Collect();
            foreach (var command in draw.Commands)
            {
                if (command.custom_draw == null) continue;
                var copy = command;
                copy.custom_draw((IntPtr)copy.user_data, copy.clip_rect, copy.layer);
            }
            Check("Cdecl custom draw callback", _callbacks == 1);

            MonstroStyle styled = ui.Style;
            styled.button.normal.type = MonstroStyleItemType.Color;
            styled.button.normal.data.color = Native.MONSTRO_COLOR_RED;
            styled.button.rounding = 7;
            Theme hud = env.RegisterTheme("hud", styled);
            Check("style union through a theme", hud.Style.button.normal.data.color.r == 1 && hud.Name == "hud");
            Throws<ArgumentException>("theme names are unique", () => env.RegisterTheme("hud", styled));
            using (var other = env.CreateContext())
            {
                ui.Theme = hud;
                other.Theme = hud;
                ui.BeginFrame(640, 480, 0.01f);
                Throws<InvalidOperationException>("theme between frames", () => ui.Theme = hud);
                MonstroStyleButton pushed = ui.Style.button;
                pushed.rounding = 33;
                ui.PushButtonStyle(pushed);
                Check("push changes the frame's copy", ui.Style.button.rounding == 33);
                Check("push leaves the shared theme", hud.Style.button.rounding == 7);
                ui.PopStyle();
                ui.Label("shared");
                ulong uiHash = ui.EndFrame().ContentHash;
                other.BeginFrame(640, 480, 0.01f);
                Check("push leaves the next context", other.Style.button.rounding == 7);
                other.Label("shared");
                DrawData otherFrame = other.EndFrame();
                Check("two contexts share the font atlas", otherFrame.ContentHash == uiHash);
                Throws<InvalidOperationException>("environment disposal waits for its contexts", env.Dispose);
            }

            ui.Dispose();
            ui.Dispose();
            Throws<ObjectDisposedException>("disposed context", () => { _ = ui.Handle; });
        }

        // Host buffers: the same frame into the canvas's own buffers and into split host streams.
        {
            MonstroEnvConfig config = Native.monstro_env_config_default();
            config.vertex_layout.buffer_count = 2;
            config.vertex_layout.stride[0] = 8;
            config.vertex_layout.stride[1] = 12;
            config.vertex_layout.position = new() { buffer = 0, offset = 0 };
            config.vertex_layout.color = new() { buffer = 1, offset = 0 };
            config.vertex_layout.texcoord = new() { buffer = 1, offset = 4 };
            using var env = new Env(config);
            env.LoadFont("sans", args[1], 16);
            using var ui = env.CreateContext();
            // The reference frame comes from an environment of its own, so its draw data stays valid meanwhile.
            using var ownEnv = new Env();
            ownEnv.LoadFont("sans", args[1], 16);
            using var ownUi = ownEnv.CreateContext();
            static void Draw(Context c)
            {
                c.SetNextWindowPosition(20, 20);
                c.SetNextWindowSize(300, 200);
                c.BeginWindow("Host");
                c.Label("streams");
                c.Button("Go");
                c.EndWindow();
            }
            ownUi.BeginFrame(400, 300, 0.01f);
            Draw(ownUi);
            DrawData own = ownUi.EndFrame();
            byte[] positions = new byte[4096 * 8];
            byte[] attributes = new byte[4096 * 12];
            uint[] indices = new uint[4096 * 3];
            Array.Fill(positions, (byte)0xcd);
            Array.Fill(attributes, (byte)0xcd);
            var buffers = new FrameBuffers(new Memory<byte>[] { positions, attributes }, indices);
            ui.BeginFrame(400, 300, 0.01f, buffers);
            Draw(ui);
            DrawData host = ui.EndFrame();
            Check("host frame equals the own frame", own.IsValid && host.ContentHash == own.ContentHash && host.DrawnIntoHostVertexBuffers &&
                                                     host.DrawnIntoHostIndices && host.Vertices.IsEmpty && host.IndexCount == own.Indices.Length);
            bool same = indices.AsSpan(0, own.Indices.Length).SequenceEqual(own.Indices);
            ReadOnlySpan<MonstroVertex> ownVertices = own.Vertices;
            for (int i = 0; i < ownVertices.Length; i++)
            {
                MonstroVertex v = ownVertices[i];
                same &= BitConverter.ToSingle(positions, i * 8) == v.position[0] && BitConverter.ToSingle(positions, i * 8 + 4) == v.position[1];
                same &= BitConverter.ToUInt32(attributes, i * 12) == v.color;
                same &= BitConverter.ToSingle(attributes, i * 12 + 4) == v.texcoord[0] && BitConverter.ToSingle(attributes, i * 12 + 8) == v.texcoord[1];
            }
            Check("split host streams hold the same vertices", same);
            Throws<ArgumentException>("streams must match the layout", () => ui.BeginFrame(400, 300, 0.01f, new FrameBuffers(new Memory<byte>[] { positions }, default)));

            using var second = env.CreateContext();
            ui.BeginFrame(400, 300, 0.01f);
            Throws<InvalidOperationException>("one canvas is lent at a time", () => second.BeginFrame(400, 300, 0.01f));
            ui.EndFrame();
        }

        // A slider drag through the convenience layer: one activation, then one deactivation after the edit.
        {
            using var env = new Env();
            env.LoadFont("sans", args[1], 16);
            using var ui = env.CreateContext();
            float throttle = 0.5f;
            MonstroRect rect = default;
            int activations = 0, deactivations = 0, commits = 0;
            bool activeThroughout = true;
            void Frame(bool dragging)
            {
                ui.BeginFrame(640, 480, 1.0f / 60);
                ui.SetNextWindowPosition(20, 20);
                ui.SetNextWindowSize(300, 200);
                if (ui.BeginWindow("Engine"))
                {
                    ui.SliderFloat("throttle##Throttle " + throttle.ToString("0.00"), ref throttle, 0, 1);
                    rect = ui.ItemRect;
                    activations += ui.ItemActivated ? 1 : 0;
                    deactivations += ui.ItemDeactivated ? 1 : 0;
                    commits += ui.ItemDeactivatedAfterEdit ? 1 : 0;
                    if (dragging && !ui.ItemActive) activeThroughout = false;
                }
                ui.EndWindow();
                ui.EndFrame();
            }
            Frame(false);
            ui.SetMousePosition(rect.x + rect.w * 0.5f, rect.y + rect.h * 0.5f);
            Frame(false);
            ui.SetMouseButton(MonstroMouseButton.Left, true);
            Frame(true);
            for (int i = 1; i <= 4; i++)
            {
                ui.SetMousePosition(rect.x + rect.w * 0.5f + i * 10, rect.y + rect.h * 0.5f);
                Frame(true);
            }
            ui.SetMouseButton(MonstroMouseButton.Left, false);
            Frame(false);
            Frame(false);
            Check("item edges over a drag", activations == 1 && deactivations == 1 && commits == 1 && activeThroughout && throttle != 0.5f);
            MonstroContextConfig defaults = Native.monstro_context_config_default();
            Check("context config defaults", defaults.max_input_events == 64 && defaults.max_items == 512);
        }

        using (var edit = new Env())
        using (var ctx = edit.CreateContext())
        {
            edit.LoadFont("sans", args[1], 18);
            string text = "A";
            ctx.BeginFrame(400, 100, 0.01f);
            ctx.InputText("edit", ref text, 64);
            MonstroRect rect = Native.monstro_get_item_rect(ctx.Handle);
            ctx.EndFrame();
            // The move and the press are taken in separate frames.
            ctx.SetMousePosition(rect.x + rect.w - 10, rect.y + rect.h / 2);
            ctx.SetMouseButton(MonstroMouseButton.Left, true);
            ctx.BeginFrame(400, 100, 0.01f);
            ctx.InputText("edit", ref text, 64);
            ctx.EndFrame();
            ctx.BeginFrame(400, 100, 0.01f);
            ctx.InputText("edit", ref text, 64);
            Check("text activated", ctx.WantTextInput);
            ctx.EndFrame();
            ctx.SetMouseButton(MonstroMouseButton.Left, false);
            GC.Collect(2, GCCollectionMode.Forced, blocking: true, compacting: true);
            ctx.AddInputText("é");
            ctx.BeginFrame(400, 100, 0.01f);
            Check("active UTF-8 edit after GC", ctx.InputText("edit", ref text, 64) && text == "Aé");
            ctx.EndFrame();
            GC.Collect(2, GCCollectionMode.Forced, blocking: true, compacting: true);
            ctx.AddInputText("🚀");
            ctx.BeginFrame(400, 100, 0.01f);
            Check("active edit takes a new buffer each frame", ctx.InputText("edit", ref text, 128) && text == "Aé🚀");
            ctx.EndFrame();
        }
        Check("color helpers", Native.monstro_color_to_u32(Native.monstro_color_from_u32(0xaabbccdd)) == 0xaabbccdd);
        Check("color clamping", Native.monstro_color_to_u32(new() { r = 2, g = -1, b = float.NaN, a = 0.5f }) == 0x800000ff);

        Check("rect boundary", Native.monstro_rect_contains(new() { w = 10, h = 10 }, 0, 0) && !Native.monstro_rect_contains(new() { w = 10, h = 10 }, 10, 5));
        Console.WriteLine($"C# binding: {(_failures == 0 ? "PASS" : "FAIL")} ({_checks} checks, {_failures} failures)");
        return _failures == 0 ? 0 : 1;
    }
}
