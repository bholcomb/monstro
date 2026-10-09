// Optional convenience layer over the one-to-one C API mirror in Native.cs.
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;

namespace Monstro;

/// <summary>A copy of a completed frame, independent of the context's native buffers.</summary>
public sealed class MonstroDrawData
{
    public MonstroVertex[] Vertices { get; }
    public uint[] Indices { get; }
    public MonstroDrawCommand[] Commands { get; }
    internal MonstroDrawData(MonstroVertex[] vertices, uint[] indices, MonstroDrawCommand[] commands)
    { Vertices = vertices; Indices = indices; Commands = commands; }
}

/// <summary>A context-owned font and a managed copy of its texture atlas.</summary>
public sealed class MonstroFontAtlas
{
    private readonly MonstroContext _owner;
    private readonly IntPtr _font;
    public uint Width { get; }
    public uint Height { get; }
    public MonstroTextureFormat Format { get; }
    public byte[] Pixels { get; }
    /// <summary>Borrowed font pointer, valid until its context is disposed. Do not destroy it.</summary>
    public IntPtr Handle { get { _ = _owner.Handle; return _font; } }
    internal MonstroFontAtlas(MonstroContext owner, IntPtr font, uint width, uint height, MonstroTextureFormat format, byte[] pixels)
    { _owner = owner; _font = font; Width = width; Height = height; Format = format; Pixels = pixels; }
    public void SetTexture(ulong textureId) => Native.monstro_font_set_atlas_texture(Handle, textureId);
}

/// <summary>
/// One UI context. Use it on one thread at a time, including disposal. Input goes
/// in before BeginFrame; build UI between BeginFrame and EndFrame; render its draw
/// data afterwards. Native contains the full API; Handle lets you mix the layers.
/// When mixing them, balance native stacks yourself and do not destroy borrowed handles.
/// </summary>
public sealed unsafe class MonstroContext : IDisposable
{
    // Like MobiusState, ownership is explicit: dispose the context. There is no
    // finalizer that could release it while a raw Native call is using Handle.
    private IntPtr _handle;
    // Native EditState retains the active widget's buffer between calls/frames.
    // Pinned, context-owned buffers preserve that address until context disposal.
    private readonly Dictionary<ulong, byte[]> _textBuffers = new();
    private bool _inFrame;
    private bool _hasFrame;

    public MonstroContext()
    {
        _handle = Native.monstro_context_create();
        if (_handle == IntPtr.Zero) throw new InvalidOperationException("Could not create a Monstro context");
    }

    /// <summary>Borrowed MonstroContext*, for calling Native directly.</summary>
    public IntPtr Handle
    {
        get { if (_handle == IntPtr.Zero) throw new ObjectDisposedException(nameof(MonstroContext)); return _handle; }
    }
    public void Dispose()
    {
        if (_handle == IntPtr.Zero) return;
        Native.monstro_context_destroy(_handle);
        _handle = IntPtr.Zero;
        _textBuffers.Clear();
    }

    public void BeginFrame(float width, float height, float deltaTime)
    {
        _ = Handle;
        if (_inFrame) throw new InvalidOperationException("End the current frame before beginning another");
        if (!float.IsFinite(width) || width <= 0) throw new ArgumentOutOfRangeException(nameof(width));
        if (!float.IsFinite(height) || height <= 0) throw new ArgumentOutOfRangeException(nameof(height));
        if (!float.IsFinite(deltaTime) || deltaTime < 0) throw new ArgumentOutOfRangeException(nameof(deltaTime));
        Native.monstro_set_display_size(Handle, width, height);
        Native.monstro_begin_frame(Handle, deltaTime);
        _inFrame = true;
        _hasFrame = false;
    }

    public void EndFrame()
    {
        RequireFrame();
        Native.monstro_end_frame(Handle);
        _inFrame = false;
        _hasFrame = true;
    }

    private IntPtr RequireFrame()
    {
        IntPtr handle = Handle;
        if (!_inFrame) throw new InvalidOperationException("Begin a frame before building UI");
        return handle;
    }

    // As in C, pair BeginWindow with EndWindow even when BeginWindow returns false.
    public bool BeginWindow(string name, MonstroWindowFlags flags = MonstroWindowFlags.Default) =>
        Native.monstro_begin_window(RequireFrame(), name ?? throw new ArgumentNullException(nameof(name)), null, flags);
    public void EndWindow() => Native.monstro_end_window(RequireFrame());
    public void SetNextWindowPosition(float x, float y, MonstroCondition condition = MonstroCondition.Always) =>
        Native.monstro_set_next_window_pos(RequireFrame(), x, y, condition);
    public void SetNextWindowSize(float width, float height, MonstroCondition condition = MonstroCondition.Always) =>
        Native.monstro_set_next_window_size(RequireFrame(), width, height, condition);
    public void Label(string text) => Native.monstro_label(RequireFrame(), text);
    public bool Button(string label, string? tooltip = null) => Native.monstro_button(RequireFrame(), label, tooltip);
    public bool Checkbox(string label, ref bool value, string? tooltip = null)
    {
        byte native = value ? (byte)1 : (byte)0;
        bool changed = Native.monstro_checkbox(RequireFrame(), label, &native, tooltip);
        value = native != 0;
        return changed;
    }
    public bool SliderFloat(string label, ref float value, float min, float max, string? format = null, string? tooltip = null)
    {
        fixed (float* pointer = &value)
            return Native.monstro_slider_float(RequireFrame(), label, pointer, min, max, format, tooltip);
    }
    public bool SliderInt(string label, ref int value, int min, int max, string? format = null, string? tooltip = null)
    {
        fixed (int* pointer = &value)
            return Native.monstro_slider_int(RequireFrame(), label, pointer, min, max, format, tooltip);
    }
    /// <summary>capacityBytes includes the UTF-8 NUL terminator, rather than UTF-16 characters.</summary>
    public bool InputText(string label, ref string value, int capacityBytes = 1024,
                          MonstroInputTextFlags flags = MonstroInputTextFlags.None, string? tooltip = null)
    {
        IntPtr ctx = RequireFrame();
        if (value == null) throw new ArgumentNullException(nameof(value));
        if (capacityBytes <= Encoding.UTF8.GetByteCount(value)) throw new ArgumentOutOfRangeException(nameof(capacityBytes));
        ulong id = Native.monstro_get_id(ctx, label);
        if (!_textBuffers.TryGetValue(id, out byte[]? buffer))
        {
            buffer = GC.AllocateArray<byte>(capacityBytes, pinned: true);
            _textBuffers.Add(id, buffer);
        }
        if (buffer.Length != capacityBytes)
            throw new ArgumentException("A text widget's byte capacity must remain fixed for the context's lifetime", nameof(capacityBytes));
        int length = Encoding.UTF8.GetBytes(value, buffer);
        buffer[length] = 0;
        fixed (byte* pointer = buffer)
        {
            bool changed = Native.monstro_input_text(ctx, label, pointer, (nuint)buffer.Length, flags, tooltip);
            value = Native.Utf8(pointer)!;
            return changed;
        }
    }
    public void BeginLayout(MonstroLayoutDirection direction) => Native.monstro_begin_layout(RequireFrame(), direction);
    public void EndLayout() => Native.monstro_end_layout(RequireFrame());
    public void SetCursorPosition(float x, float y) => Native.monstro_set_cursor_pos(RequireFrame(), x, y);
    public void SetMousePosition(float x, float y) => Native.monstro_set_mouse_pos(Handle, x, y);
    public void SetMouseButton(MonstroMouseButton button, bool down) => Native.monstro_set_mouse_button(Handle, button, down);
    public void SetMouseWheel(float x, float y) => Native.monstro_set_mouse_wheel(Handle, x, y);
    public void SetKey(MonstroKey key, bool down) => Native.monstro_set_key(Handle, key, down);
    public void SetKeyModifiers(MonstroModFlags modifiers) => Native.monstro_set_key_mods(Handle, modifiers);
    public void AddInputText(string text) => Native.monstro_add_input_text(Handle, text);
    public bool WantCaptureMouse => Native.monstro_want_capture_mouse(Handle);
    public bool WantCaptureKeyboard => Native.monstro_want_capture_keyboard(Handle);
    public bool WantTextInput => Native.monstro_want_text_input(Handle);

    public void DrawTriangle(MonstroVec2 a, MonstroVec2 b, MonstroVec2 c, uint color) =>
        Native.monstro_draw_triangle_filled(Native.monstro_get_window_canvas(RequireFrame()), a, b, c, color);

    public MonstroFontAtlas LoadFont(string path, float pixelSize, uint firstCodePoint = 32, uint codePointCount = 224)
    {
        IntPtr ctx = Handle;
        if (path == null) throw new ArgumentNullException(nameof(path));
        if (!float.IsFinite(pixelSize) || pixelSize <= 0) throw new ArgumentOutOfRangeException(nameof(pixelSize));
        if (codePointCount == 0) throw new ArgumentOutOfRangeException(nameof(codePointCount));
        MonstroTextureData data;
        IntPtr manager = Native.monstro_context_get_font_manager(ctx);
        IntPtr font = Native.monstro_font_load_ttf_from_file(manager, path, pixelSize, firstCodePoint, codePointCount, &data);
        if (font == IntPtr.Zero) throw new InvalidOperationException($"Could not load Monstro font: {path}");
        bool ownedByContext = false;
        try
        {
            byte[] pixels = new ReadOnlySpan<byte>(data.data, checked((int)data.data_size)).ToArray();
            var atlas = new MonstroFontAtlas(this, font, data.width, data.height, data.format, pixels);
            string name = "dotnet-" + Guid.NewGuid().ToString("N");
            Native.monstro_font_add(manager, font, name, pixelSize);
            if (Native.monstro_font_find(manager, name, pixelSize) != font)
                throw new InvalidOperationException("Monstro's font manager is full");
            ownedByContext = true;
            if (Native.monstro_context_get_default_font(ctx) == IntPtr.Zero)
                Native.monstro_context_set_default_font(ctx, font);
            return atlas;
        }
        finally { if (!ownedByContext) Native.monstro_font_destroy(font); }
    }

    /// <summary>Copy renderer data after EndFrame. Native also exposes borrowed buffers for zero-copy rendering.</summary>
    public MonstroDrawData GetDrawData()
    {
        IntPtr ctx = Handle;
        if (!_hasFrame || _inFrame) throw new InvalidOperationException("End a frame before reading its draw data");
        MonstroVertex* vertices;
        nuint vertexBytes;
        uint* indices;
        uint indexCount;
        MonstroDrawCommand* commands;
        uint commandCount;
        IntPtr buffer = Native.monstro_context_get_draw_buffer(ctx);
        Native.monstro_draw_buffer_get_vertex_buffer(buffer, &vertices, &vertexBytes);
        Native.monstro_draw_buffer_get_index_buffer(buffer, &indices, &indexCount);
        Native.monstro_context_get_draw_commands(ctx, &commands, &commandCount);
        return new MonstroDrawData(new ReadOnlySpan<MonstroVertex>(vertices, checked((int)(vertexBytes / (nuint)sizeof(MonstroVertex)))).ToArray(),
                                  new ReadOnlySpan<uint>(indices, checked((int)indexCount)).ToArray(),
                                  new ReadOnlySpan<MonstroDrawCommand>(commands, checked((int)commandCount)).ToArray());
    }
}
