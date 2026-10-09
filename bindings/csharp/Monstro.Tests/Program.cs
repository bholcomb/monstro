// Console checks against the real native library, like Mobius.Tests.
using System;
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

        Native.monstro_init();
        using (var ui = new MonstroContext())
        {
            Throws<InvalidOperationException>("draw before frame", () => ui.GetDrawData());
            Throws<InvalidOperationException>("widget before frame", () => ui.Button("early"));
            Throws<InvalidOperationException>("end before begin", ui.EndFrame);
            Throws<ArgumentOutOfRangeException>("non-finite dimensions", () => ui.BeginFrame(float.NaN, 480, 0.01f));
            for (int frame = 0; frame < 2; frame++)
            {
                ui.BeginFrame(640, 480, 1.0f / 60);
                Throws<InvalidOperationException>("double begin", () => ui.BeginFrame(640, 480, 0.01f));
                Throws<InvalidOperationException>("read during frame", () => ui.GetDrawData());
                ui.DrawTriangle(new() { x = 10, y = 10 }, new() { x = 100, y = 10 }, new() { x = 10, y = 100 }, 0xffffffff);
                ui.EndFrame();
                var data = ui.GetDrawData();
                Check("frame resets vertex/index buffers", data.Vertices.Length == 3 && data.Indices.Length == 3);
                Check("triangle position by-value struct", data.Vertices[0].position[0] == 10 && data.Vertices[0].position[1] == 10);
                Check("draw commands", data.Commands.Length == 1 && data.Commands[0].index_count == 3);
                Check("indices reference vertices", data.Indices.All(i => i < data.Vertices.Length));
                if (frame == 0)
                {
                    ui.BeginFrame(640, 480, 0);
                    ui.EndFrame();
                    Check("empty frame", ui.GetDrawData().Indices.Length == 0);
                    Check("copied data survives next frame", data.Indices.Length == 3 && data.Vertices[0].position[0] == 10);
                }
            }

            ui.SetMousePosition(23, 45);
            MonstroVec2 mouse = Native.monstro_get_mouse_pos(ui.Handle);
            Check("struct return", mouse.x == 23 && mouse.y == 45);
            ui.SetKey(MonstroKey.A, true);
            Check("one-byte bool true", Native.monstro_is_key_down(ui.Handle, MonstroKey.A));
            ui.SetKey(MonstroKey.A, false);
            Check("one-byte bool false", !Native.monstro_is_key_down(ui.Handle, MonstroKey.A));
            ui.AddInputText("héλlo 🚀");
            Check("UTF-8 input/result", Native.Utf8(Native.monstro_get_input_text(ui.Handle)) == "héλlo 🚀");

            var font = ui.LoadFont(args[1], 18);
            Check("font atlas", font.Width > 0 && font.Height > 0 && font.Pixels.Length == font.Width * font.Height);
            Check("font format", font.Format == MonstroTextureFormat.R8);
            font.SetTexture(123);
            Check("font texture ID", Native.monstro_font_get_atlas_texture(font.Handle) == 123);
            MonstroVec2 measured = Native.monstro_font_measure_text(font.Handle, "café", 0);
            Check("UTF-8 font measurement", measured.x > 0 && measured.y > 0);
            MonstroGlyph glyph;
            Check("glyph output struct", Native.monstro_font_get_glyph(font.Handle, (uint)'A', &glyph) && glyph.advance[0] > 0);
            Throws<InvalidOperationException>("font failure", () => ui.LoadFont("no-such-font.ttf", 18));

            ui.BeginFrame(640, 480, 1.0f / 60);
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
            ui.EndFrame();
            var draw = ui.GetDrawData();
            Check("widgets generate geometry", draw.Vertices.Length > 3 && draw.Indices.Length > 3);
            Check("font texture in draw commands", draw.Commands.Any(c => c.texture_id == 123));
            GC.Collect();
            foreach (var command in draw.Commands)
            {
                if (command.custom_draw == null) continue;
                var copy = command;
                copy.custom_draw((IntPtr)copy.user_data, copy.clip_rect, copy.layer);
            }
            Check("Cdecl custom draw callback", _callbacks == 1);

            MonstroStyle original = *Native.monstro_get_style(ui.Handle);
            MonstroStyle styled = original;
            styled.button.normal.type = MonstroStyleItemType.Color;
            styled.button.normal.data.color = Native.MONSTRO_COLOR_RED;
            Native.monstro_set_style(ui.Handle, &styled);
            Check("style union write/read", Native.monstro_get_style(ui.Handle)->button.normal.data.color.r == 1);
            Native.monstro_set_style(ui.Handle, &original);

            ui.Dispose();
            ui.Dispose();
            Throws<ObjectDisposedException>("disposed context", () => { _ = ui.Handle; });
            Throws<ObjectDisposedException>("disposed font owner", () => font.SetTexture(123));
        }
        using (var other = new MonstroContext())
        { other.BeginFrame(100, 100, 0.01f); other.EndFrame(); Check("independent context", other.GetDrawData().Indices.Length == 0); }
        using (var edit = new MonstroContext())
        {
            edit.LoadFont(args[1], 18);
            string text = "A";
            edit.BeginFrame(400, 100, 0.01f);
            edit.InputText("edit", ref text, 64);
            MonstroRect rect = Native.monstro_get_item_rect(edit.Handle);
            edit.EndFrame();
            edit.SetMousePosition(rect.x + rect.w - 10, rect.y + rect.h / 2);
            edit.SetMouseButton(MonstroMouseButton.Left, true);
            edit.BeginFrame(400, 100, 0.01f);
            edit.InputText("edit", ref text, 64);
            Check("text activated", edit.WantTextInput);
            edit.EndFrame();
            edit.SetMouseButton(MonstroMouseButton.Left, false);
            GC.Collect(2, GCCollectionMode.Forced, blocking: true, compacting: true);
            edit.AddInputText("é");
            edit.BeginFrame(400, 100, 0.01f);
            Check("active UTF-8 edit after GC", edit.InputText("edit", ref text, 64) && text == "Aé");
            Throws<ArgumentException>("active text capacity fixed", () => edit.InputText("edit", ref text, 128));
            edit.EndFrame();
            GC.Collect(2, GCCollectionMode.Forced, blocking: true, compacting: true);
            edit.AddInputText("🚀");
            edit.BeginFrame(400, 100, 0.01f);
            Check("active edit retains buffer across frames", edit.InputText("edit", ref text, 64) && text == "Aé🚀");
            edit.EndFrame();
        }
        Check("color helpers", Native.monstro_color_to_u32(Native.monstro_color_from_u32(0xaabbccdd)) == 0xaabbccdd);
        Check("rect boundary", Native.monstro_rect_contains(new() { w = 10, h = 10 }, 0, 0) && !Native.monstro_rect_contains(new() { w = 10, h = 10 }, 10, 5));
        Native.monstro_shutdown();
        Console.WriteLine($"C# binding: {(_failures == 0 ? "PASS" : "FAIL")} ({_checks} checks, {_failures} failures)");
        return _failures == 0 ? 0 : 1;
    }
}
