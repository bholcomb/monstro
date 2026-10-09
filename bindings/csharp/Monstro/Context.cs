// Optional convenience layer over the one-to-one C API mirror in Native.cs.
using System;
using System.Buffers;
using System.Text;

namespace Monstro;

/// <summary>
/// Host storage for one frame: one byte buffer per buffer of the environment's vertex layout
/// (MonstroEnvConfig.vertex_layout), and the indices. Leave VertexBuffers empty, or Indices empty, to use
/// the canvas's own storage for them. The memory is pinned from BeginFrame to EndFrame and only written.
/// </summary>
public sealed class FrameBuffers
{
    public Memory<byte>[] VertexBuffers { get; }
    public Memory<uint> Indices { get; }

    public FrameBuffers(Memory<byte>[] vertexBuffers, Memory<uint> indices)
    {
        VertexBuffers = vertexBuffers ?? throw new ArgumentNullException(nameof(vertexBuffers));
        Indices = indices;
    }
}

/// <summary>
/// One UI on an environment: its input, display size and the state it keeps between frames. Use it on one
/// thread at a time, including disposal. Input goes in before BeginFrame; build UI between BeginFrame and
/// EndFrame; render the DrawData EndFrame returns. Native has the full API; Handle lets you mix the layers.
/// </summary>
public sealed unsafe class Context : IDisposable
{
    // Ownership is explicit: dispose the context. No finalizer could release it while Native uses Handle.
    private readonly Env _env;
    private MonstroContext _handle;
    private bool _inFrame;
    private MemoryHandle[] _pins = Array.Empty<MemoryHandle>();
    private bool _hostIndices;

    internal Context(Env env, MonstroContextConfig? config)
    {
        _env = env;
        if (config is MonstroContextConfig c)
            _handle = Native.monstro_context_create(env.Handle, &c);
        else
            _handle = Native.monstro_context_create(env.Handle, null);
        if (_handle.IsNull) throw new OutOfMemoryException("Could not create a Monstro context");
    }

    public Env Env => _env;

    /// <summary>The native context, for calling Native directly.</summary>
    public MonstroContext Handle
    {
        get { if (_handle.IsNull) throw new ObjectDisposedException(nameof(Context)); return _handle; }
    }

    public void Dispose()
    {
        if (_handle.IsNull) return;
        if (_inFrame) throw new InvalidOperationException("End the frame before disposing the context");
        _env.RequireIdle();
        Native.monstro_context_destroy(_handle);
        _handle = default;
        _env.Forget(this);
    }

    /// <summary>The theme the context's frames start from; change it between frames.</summary>
    public Theme Theme
    {
        get => new(_env, Native.monstro_context_get_theme(Handle));
        set
        {
            if (_inFrame) throw new InvalidOperationException("Change the theme between frames");
            if (value.Env != _env) throw new ArgumentException("The theme belongs to another environment", nameof(value));
            Native.monstro_context_set_theme(Handle, value.Handle);
        }
    }

    public void BeginFrame(float width, float height, float deltaTime) => Begin(width, height, deltaTime, null);

    /// <summary>Builds the frame into host buffers, pinned until EndFrame.</summary>
    public void BeginFrame(float width, float height, float deltaTime, FrameBuffers buffers) =>
        Begin(width, height, deltaTime, buffers ?? throw new ArgumentNullException(nameof(buffers)));

    private void Begin(float width, float height, float deltaTime, FrameBuffers? buffers)
    {
        _ = Handle;
        if (_inFrame) throw new InvalidOperationException("End the current frame before beginning another");
        if (!float.IsFinite(width) || width <= 0) throw new ArgumentOutOfRangeException(nameof(width));
        if (!float.IsFinite(height) || height <= 0) throw new ArgumentOutOfRangeException(nameof(height));
        if (!float.IsFinite(deltaTime) || deltaTime < 0) throw new ArgumentOutOfRangeException(nameof(deltaTime));
        if (_env.OpenFrames >= _env.Config.canvas_count)
            throw new InvalidOperationException($"All {_env.Config.canvas_count} canvases are lent; end a frame first");

        Native.monstro_set_display_size(Handle, width, height);
        if (buffers == null)
        {
            _hostIndices = false;
            Native.monstro_begin_frame(Handle, deltaTime);
        }
        else
        {
            MonstroFrameBuffers native = Pin(buffers);
            try { Native.monstro_begin_frame_into(Handle, deltaTime, &native); }
            catch { Unpin(); throw; }
        }
        _inFrame = true;
        _env.OpenFrames++;
    }

    private MonstroFrameBuffers Pin(FrameBuffers buffers)
    {
        MonstroVertexLayout layout = _env.Config.vertex_layout;
        int streams = buffers.VertexBuffers.Length;
        if (streams != 0 && streams != layout.buffer_count)
            throw new ArgumentException($"The vertex layout has {layout.buffer_count} buffers; {streams} were given");
        MonstroFrameBuffers native = default;
        _pins = new MemoryHandle[streams + 1];
        _hostIndices = !buffers.Indices.IsEmpty;
        if (streams > 0)
        {
            long capacity = long.MaxValue;
            for (int b = 0; b < streams; b++)
                capacity = Math.Min(capacity, buffers.VertexBuffers[b].Length / layout.stride[b]);
            if (capacity <= 0) throw new ArgumentException("The vertex buffers hold no vertex");
            for (int b = 0; b < streams; b++)
            {
                _pins[b] = buffers.VertexBuffers[b].Pin();
                native.vertex_buffers[b] = (ulong)_pins[b].Pointer;
            }
            native.vertex_capacity = (uint)Math.Min(capacity, uint.MaxValue);
        }
        if (!buffers.Indices.IsEmpty)
        {
            _pins[streams] = buffers.Indices.Pin();
            native.indices = (uint*)_pins[streams].Pointer;
            native.index_capacity = (uint)buffers.Indices.Length;
        }
        return native;
    }

    private void Unpin()
    {
        foreach (MemoryHandle pin in _pins)
            pin.Dispose();
        _pins = Array.Empty<MemoryHandle>();
    }

    /// <summary>Ends the frame; its draw data is a view into the canvas, valid until the canvas builds another frame.</summary>
    public DrawData EndFrame()
    {
        RequireFrame();
        MonstroDrawData* data = Native.monstro_end_frame(Handle);
        _inFrame = false;
        _env.OpenFrames--;
        Unpin();
        return new DrawData(_env, data, _hostIndices);
    }

    private MonstroContext RequireFrame()
    {
        MonstroContext handle = Handle;
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
    public void Label(string text) => Native.monstro_label(RequireFrame(), text ?? throw new ArgumentNullException(nameof(text)));
    public bool Button(string label, string? tooltip = null) =>
        Native.monstro_button(RequireFrame(), label ?? throw new ArgumentNullException(nameof(label)), tooltip);
    public bool Checkbox(string label, ref bool value, string? tooltip = null)
    {
        byte native = value ? (byte)1 : (byte)0;
        bool changed = Native.monstro_checkbox(RequireFrame(), label ?? throw new ArgumentNullException(nameof(label)), &native, tooltip);
        value = native != 0;
        return changed;
    }
    public bool SliderFloat(string label, ref float value, float min, float max, string? format = null, string? tooltip = null)
    {
        MonstroContext ctx = RequireFrame();
        fixed (float* pointer = &value)
            return Native.monstro_slider_float(ctx, label ?? throw new ArgumentNullException(nameof(label)), pointer, min, max, format, tooltip);
    }
    public bool SliderInt(string label, ref int value, int min, int max, string? format = null, string? tooltip = null)
    {
        MonstroContext ctx = RequireFrame();
        fixed (int* pointer = &value)
            return Native.monstro_slider_int(ctx, label ?? throw new ArgumentNullException(nameof(label)), pointer, min, max, format, tooltip);
    }
    /// <summary>capacityBytes includes the UTF-8 NUL terminator, rather than UTF-16 characters.</summary>
    public bool InputText(string label, ref string value, int capacityBytes = 1024,
                          MonstroInputTextFlags flags = MonstroInputTextFlags.None, string? tooltip = null)
    {
        MonstroContext ctx = RequireFrame();
        if (label == null) throw new ArgumentNullException(nameof(label));
        if (value == null) throw new ArgumentNullException(nameof(value));
        if (capacityBytes <= Encoding.UTF8.GetByteCount(value)) throw new ArgumentOutOfRangeException(nameof(capacityBytes));
        // Native editing reads and writes the buffer only during the call.
        byte[] buffer = new byte[capacityBytes];
        int length = Encoding.UTF8.GetBytes(value, buffer);
        buffer[length] = 0;
        fixed (byte* pointer = buffer)
        {
            bool changed = Native.monstro_input_text(ctx, label, pointer, (nuint)buffer.Length, flags, tooltip);
            value = Native.Utf8(pointer)!;
            return changed;
        }
    }
    // The state of the widget just built; see the item state queries in widgets.h.
    public bool ItemHovered => Native.monstro_is_item_hovered(RequireFrame());
    /// <summary>Held, dragged, being edited or open.</summary>
    public bool ItemActive => Native.monstro_is_item_active(RequireFrame());
    /// <summary>Its value changed this frame.</summary>
    public bool ItemEdited => Native.monstro_is_item_edited(RequireFrame());
    /// <summary>It became active this frame: a press, a drag or an edit started.</summary>
    public bool ItemActivated => Native.monstro_is_item_activated(RequireFrame());
    /// <summary>It stopped being active this frame.</summary>
    public bool ItemDeactivated => Native.monstro_is_item_deactivated(RequireFrame());
    /// <summary>It stopped being active this frame and changed its value while it was: the moment to commit.</summary>
    public bool ItemDeactivatedAfterEdit => Native.monstro_is_item_deactivated_after_edit(RequireFrame());
    public MonstroRect ItemRect => Native.monstro_get_item_rect(RequireFrame());

    public void BeginLayout(MonstroLayoutDirection direction) => Native.monstro_begin_layout(RequireFrame(), direction);
    public void EndLayout() => Native.monstro_end_layout(RequireFrame());
    public void SetCursorPosition(float x, float y) => Native.monstro_set_cursor_pos(RequireFrame(), x, y);
    public void PushButtonStyle(in MonstroStyleButton style)
    {
        MonstroContext ctx = RequireFrame();
        fixed (MonstroStyleButton* s = &style)
            Native.monstro_push_button_style(ctx, s);
    }
    public void PopStyle(int count = 1) => Native.monstro_pop_style(RequireFrame(), count);
    /// <summary>The current style: the frame's working copy, or the theme's between frames.</summary>
    public ref readonly MonstroStyle Style => ref *Native.monstro_get_style(Handle);

    // Input changes are queued and taken by the next frames, so none is lost between frames; see input.h.
    public void SetMousePosition(float x, float y) => Native.monstro_set_mouse_pos(Handle, x, y);
    public void SetMouseButton(MonstroMouseButton button, bool down) => Native.monstro_set_mouse_button(Handle, button, down);
    /// <summary>Adds a wheel movement.</summary>
    public void SetMouseWheel(float x, float y) => Native.monstro_set_mouse_wheel(Handle, x, y);
    public void SetKey(MonstroKey key, bool down) => Native.monstro_set_key(Handle, key, down);
    public void SetKeyModifiers(MonstroModFlags modifiers) => Native.monstro_set_key_mods(Handle, modifiers);
    public void AddInputText(string text) => Native.monstro_add_input_text(Handle, text ?? throw new ArgumentNullException(nameof(text)));
    public bool WantCaptureMouse => Native.monstro_want_capture_mouse(Handle);
    public bool WantCaptureKeyboard => Native.monstro_want_capture_keyboard(Handle);
    public bool WantTextInput => Native.monstro_want_text_input(Handle);

    public void DrawTriangle(MonstroVec2 a, MonstroVec2 b, MonstroVec2 c, uint color) =>
        Native.monstro_draw_triangle_filled(Native.monstro_get_window_canvas(RequireFrame()), a, b, c, color);
}
