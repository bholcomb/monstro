// Draw data: a zero-copy view of a finished frame.
using System;

namespace Monstro;

/// <summary>
/// A finished frame, viewed in the native canvas that built it: nothing is copied. It is valid until that
/// canvas builds another frame (with one canvas, the next BeginFrame of any context on the environment);
/// every property checks this and throws once it is stale. Spans taken from it are valid as long as the
/// view is: consume the frame before the canvas builds the next one.
/// </summary>
public readonly unsafe ref struct DrawData
{
    private readonly Env _env;
    private readonly MonstroDrawData* _data;
    private readonly ulong _frameId;
    private readonly bool _hostIndices;

    internal DrawData(Env env, MonstroDrawData* data, bool hostIndices)
    {
        _env = env;
        _data = data;
        _frameId = data->frame_id;
        _hostIndices = hostIndices;
    }

    /// <summary>The environment's number for this frame.</summary>
    public ulong FrameId => _frameId;

    /// <summary>Whether the canvas still holds this frame.</summary>
    public bool IsValid => _data != null && _env.Live && _data->frame_id == _frameId;

    private MonstroDrawData* Data
    {
        get
        {
            if (!IsValid)
                throw new InvalidOperationException(
                    "This draw data's canvas has built another frame since; consume draw data before the next frame");
            return _data;
        }
    }

    /// <summary>The vertices, empty when the frame was drawn into host buffers.</summary>
    public ReadOnlySpan<MonstroVertex> Vertices
    {
        get { MonstroDrawData* d = Data; return d->vertices == null ? default : new(d->vertices, checked((int)d->vertex_count)); }
    }
    public int VertexCount => checked((int)Data->vertex_count);
    /// <summary>The indices, empty when the frame was drawn into host index storage.</summary>
    public ReadOnlySpan<uint> Indices
    {
        get { MonstroDrawData* d = Data; return _hostIndices ? default : new(d->indices, checked((int)d->index_count)); }
    }
    public int IndexCount => checked((int)Data->index_count);
    public ReadOnlySpan<MonstroDrawCommand> Commands
    {
        get { MonstroDrawData* d = Data; return new(d->commands, checked((int)d->command_count)); }
    }
    /// <summary>Whether the vertices went to host buffers (FrameBuffers); they are there, not in Vertices.</summary>
    public bool DrawnIntoHostVertexBuffers => Data->vertices == null;
    /// <summary>Whether the indices went to host storage; they are there, not in Indices.</summary>
    public bool DrawnIntoHostIndices { get { _ = Data; return _hostIndices; } }
    public MonstroVec2 DisplaySize { get { MonstroDrawData* d = Data; return new() { x = d->display_size[0], y = d->display_size[1] }; } }
    /// <summary>Covers the commands and every vertex and index written: equal hashes mean identical frames.</summary>
    public ulong ContentHash => Data->content_hash;
    /// <summary>Whether this frame differs from the context's previous one.</summary>
    public bool Changed => Data->changed != 0;
    public ulong AtlasVersion => Data->atlas_version;
}
