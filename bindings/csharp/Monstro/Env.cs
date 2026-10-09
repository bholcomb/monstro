// Optional convenience layer over the one-to-one C API mirror in Native.cs.
using System;
using System.Collections.Generic;

namespace Monstro;

/// <summary>A font in an environment's atlas. It lives until the environment is disposed.</summary>
public readonly unsafe struct Font
{
    private readonly Env _env;
    private readonly MonstroFont _font;
    internal Font(Env env, MonstroFont font) { _env = env; _font = font; }

    /// <summary>The native font, for calling Native directly; valid until the environment is disposed.</summary>
    public MonstroFont Handle { get { _ = _env.Handle; return _font; } }
    public string Name => Native.Utf8(Native.monstro_font_get_name(Handle))!;
    public float Size => Native.monstro_font_get_size(Handle);
    public float LineHeight => Native.monstro_font_get_line_height(Handle);
    public MonstroVec2 Measure(string text, float wrapWidth = 0) =>
        Native.monstro_font_measure_text(Handle, text ?? throw new ArgumentNullException(nameof(text)), wrapWidth);
}

/// <summary>A theme registered in an environment: immutable, shared by any number of contexts.</summary>
public readonly unsafe struct Theme
{
    private readonly Env _env;
    private readonly MonstroTheme _theme;
    internal Theme(Env env, MonstroTheme theme) { _env = env; _theme = theme; }

    public MonstroTheme Handle { get { _ = _env.Handle; return _theme; } }
    internal Env Env => _env;
    public string Name => Native.Utf8(Native.monstro_theme_get_name(Handle))!;
    /// <summary>The theme's style, read-only native memory valid until the environment is disposed.</summary>
    public ref readonly MonstroStyle Style => ref *Native.monstro_theme_get_style(Handle);
}

/// <summary>
/// The environment: fonts and their atlas, themes, settings and the canvases frames are built in, shared by
/// every context created from it. Set it up between frames; it is read-only while a frame is open. Dispose
/// its contexts before it.
/// </summary>
public sealed unsafe class Env : IDisposable
{
    private MonstroEnv _handle;
    private readonly List<Context> _contexts = new();
    internal int OpenFrames;
    internal bool Live => !_handle.IsNull;

    /// <summary>Creates an environment, with Native.monstro_env_config_default() unless a config is given.</summary>
    public Env(MonstroEnvConfig? config = null)
    {
        MonstroEnvConfig c = config ?? Native.monstro_env_config_default();
        _handle = Native.monstro_init(&c);
        if (_handle.IsNull) throw new OutOfMemoryException("Could not create a Monstro environment");
        Config = c;
    }

    public MonstroEnvConfig Config { get; }

    /// <summary>The native environment, for calling Native directly.</summary>
    public MonstroEnv Handle
    {
        get { if (_handle.IsNull) throw new ObjectDisposedException(nameof(Env)); return _handle; }
    }

    public void Dispose()
    {
        if (_handle.IsNull) return;
        if (_contexts.Count > 0) throw new InvalidOperationException($"Dispose the environment's {_contexts.Count} contexts first");
        Native.monstro_shutdown(_handle);
        _handle = default;
    }

    public Context CreateContext(MonstroContextConfig? config = null)
    {
        RequireIdle();
        var context = new Context(this, config);
        _contexts.Add(context);
        return context;
    }

    internal void Forget(Context context) => _contexts.Remove(context);

    internal void RequireIdle()
    {
        _ = Handle;
        if (OpenFrames > 0) throw new InvalidOperationException("The environment is read-only while a frame is open");
    }

    public MonstroSettings Settings
    {
        get => Native.monstro_env_get_settings(Handle);
        set { RequireIdle(); Native.monstro_env_set_settings(Handle, &value); }
    }

    /// <summary>Loads a TrueType font from a file; throws if it cannot be read as one.</summary>
    public Font LoadFont(string name, string path, float pixelSize, uint firstCodePoint = 32, uint codePointCount = 224)
    {
        RequireFontArgs(name, pixelSize, codePointCount);
        MonstroFont font = Native.monstro_font_load_ttf_from_file(Handle, name, path ?? throw new ArgumentNullException(nameof(path)),
                                                                  pixelSize, firstCodePoint, codePointCount);
        if (font.IsNull) throw new InvalidOperationException($"Could not load Monstro font: {path}");
        return new Font(this, font);
    }

    /// <summary>Loads a TrueType font from its bytes, which are only read during the call.</summary>
    public Font LoadFont(string name, ReadOnlySpan<byte> ttf, float pixelSize, uint firstCodePoint = 32, uint codePointCount = 224)
    {
        RequireFontArgs(name, pixelSize, codePointCount);
        if (ttf.IsEmpty) throw new ArgumentException("The font data is empty", nameof(ttf));
        MonstroFont font;
        fixed (byte* data = ttf)
            font = Native.monstro_font_load_ttf(Handle, name, data, (nuint)ttf.Length, pixelSize, firstCodePoint, codePointCount);
        if (font.IsNull) throw new InvalidOperationException($"Could not read Monstro font data for {name}");
        return new Font(this, font);
    }

    /// <summary>Loads a bitmap font: a grid of glyph cells in an image file.</summary>
    public Font LoadBitmapFont(string name, string path, uint gridColumns, uint gridRows, uint firstCodePoint, float pixelSize)
    {
        RequireFontArgs(name, pixelSize, gridColumns * gridRows);
        MonstroFont font = Native.monstro_font_load_bitmap_from_file(Handle, name, path ?? throw new ArgumentNullException(nameof(path)),
                                                                     gridColumns, gridRows, firstCodePoint, pixelSize);
        if (font.IsNull) throw new InvalidOperationException($"Could not load Monstro bitmap font: {path}");
        return new Font(this, font);
    }

    private void RequireFontArgs(string name, float pixelSize, uint codePointCount)
    {
        RequireIdle();
        if (name == null) throw new ArgumentNullException(nameof(name));
        if (!float.IsFinite(pixelSize) || pixelSize <= 0) throw new ArgumentOutOfRangeException(nameof(pixelSize));
        if (codePointCount == 0) throw new ArgumentOutOfRangeException(nameof(codePointCount));
    }

    public Font? FindFont(string name, float size = 0)
    {
        MonstroFont font = Native.monstro_font_find(Handle, name ?? throw new ArgumentNullException(nameof(name)), size);
        return font.IsNull ? null : new Font(this, font);
    }

    /// <summary>The font themes without one of their own use: the first font loaded, until set.</summary>
    public Font? DefaultFont
    {
        get { MonstroFont font = Native.monstro_env_get_default_font(Handle); return font.IsNull ? null : new Font(this, font); }
        set
        {
            RequireIdle();
            if (value is not Font font) throw new ArgumentNullException(nameof(value));
            Native.monstro_env_set_default_font(Handle, font.Handle);
        }
    }

    /// <summary>Registers a theme; themes are immutable and their names unique.</summary>
    public Theme RegisterTheme(string name, in MonstroStyle style)
    {
        RequireIdle();
        if (name == null) throw new ArgumentNullException(nameof(name));
        if (FindTheme(name) != null) throw new ArgumentException($"A theme named '{name}' is already registered", nameof(name));
        fixed (MonstroStyle* s = &style)
            return new Theme(this, Native.monstro_theme_register(Handle, name, s));
    }

    public Theme? FindTheme(string name)
    {
        MonstroTheme theme = Native.monstro_theme_find(Handle, name ?? throw new ArgumentNullException(nameof(name)));
        return theme.IsNull ? null : new Theme(this, theme);
    }

    /// <summary>The atlas every font is packed into, with the white texel solid shapes sample.</summary>
    public AtlasView Atlas => new AtlasView(Native.monstro_env_get_atlas(Handle));

    /// <summary>The texture ID draw commands carry for the atlas: 0 until set.</summary>
    public ulong AtlasTexture
    {
        get => Native.monstro_env_get_atlas_texture(Handle);
        set { RequireIdle(); Native.monstro_env_set_atlas_texture(Handle, value); }
    }
}

/// <summary>
/// The atlas pixels, borrowed from native memory: RGBA8, Width * Height * 4 bytes. Upload them again whenever
/// Version changes; the span is valid until the next font load or the environment's disposal.
/// </summary>
public readonly unsafe ref struct AtlasView
{
    private readonly MonstroTextureData _data;
    internal AtlasView(MonstroTextureData data) { _data = data; }
    public uint Width => _data.width;
    public uint Height => _data.height;
    public MonstroTextureFormat Format => _data.format;
    public ulong Version => _data.version;
    public ReadOnlySpan<byte> Pixels => new(_data.data, checked((int)_data.data_size));
}
