// Host storage for frames: memory that never moves, so a frame is built into it without pinning.
using System;
using System.Runtime.InteropServices;

namespace Monstro;

/// <summary>
/// Host storage frames are built into: one vertex buffer per buffer of a vertex layout, and the indices.
/// The memory never moves, so nothing is pinned per frame: either these buffers allocate it once on the
/// pinned object heap, or the host passes pointers to memory that stays put (native or mapped memory, or
/// its own pinned arrays) until it disposes the buffers. A capacity of 0 (a null pointer) leaves that part
/// to the canvas's own storage. One frame at a time writes into the buffers, from BeginFrame to EndFrame;
/// read them between frames. Disposing them during a frame, or using them once disposed, throws.
/// </summary>
public sealed unsafe class FrameBuffers : IDisposable
{
    private readonly MonstroVertexLayout _layout;
    private MonstroFrameBuffers _buffers;
    // The pinned object heap arrays behind _buffers, when these buffers allocated them. Spans over them keep them alive.
    private byte[][]? _vertexArrays;
    private uint[]? _indexArray;
    private bool _disposed;
    internal bool InFrame;

    /// <summary>Allocates buffers for vertexCapacity vertices of the layout and indexCapacity indices, once.</summary>
    public FrameBuffers(in MonstroVertexLayout layout, int vertexCapacity, int indexCapacity)
    {
        RequireLayout(layout);
        if (vertexCapacity < 0) throw new ArgumentOutOfRangeException(nameof(vertexCapacity));
        if (indexCapacity < 0) throw new ArgumentOutOfRangeException(nameof(indexCapacity));
        _layout = layout;
        if (vertexCapacity > 0)
        {
            _vertexArrays = new byte[layout.buffer_count][];
            for (int b = 0; b < layout.buffer_count; b++)
            {
                _vertexArrays[b] = GC.AllocateArray<byte>(checked((int)(layout.stride[b] * (long)vertexCapacity)), pinned: true);
                _buffers.vertex_buffers[b] = (ulong)Marshal.UnsafeAddrOfPinnedArrayElement(_vertexArrays[b], 0);
            }
            _buffers.vertex_capacity = (uint)vertexCapacity;
        }
        if (indexCapacity > 0)
        {
            _indexArray = GC.AllocateArray<uint>(indexCapacity, pinned: true);
            _buffers.indices = (uint*)Marshal.UnsafeAddrOfPinnedArrayElement(_indexArray, 0);
            _buffers.index_capacity = (uint)indexCapacity;
        }
    }

    /// <summary>
    /// Wraps host memory that stays put and alive until these buffers are disposed: a pointer per layout buffer,
    /// each holding vertex_capacity strides, or none; and indices for index_capacity indices, or none.
    /// Monstro writes through these pointers on every draw call of a frame, not only during BeginFrame, so a
    /// managed array must be pinned for the buffers' whole lifetime (allocated with
    /// GC.AllocateArray(n, pinned: true), or held by a pinned GCHandle); a plain array that the garbage
    /// collector can move is not safe. Most hosts should use the allocating constructor instead.
    /// </summary>
    public FrameBuffers(in MonstroVertexLayout layout, in MonstroFrameBuffers buffers)
    {
        RequireLayout(layout);
        bool vertices = buffers.vertex_buffers[0] != 0;
        for (int b = 0; b < 4; b++)
            if ((buffers.vertex_buffers[b] != 0) != (vertices && b < layout.buffer_count))
                throw new ArgumentException($"The vertex layout has {layout.buffer_count} buffers; give a pointer for each or none",
                                            nameof(buffers));
        if (vertices != (buffers.vertex_capacity != 0))
            throw new ArgumentException("Give vertex buffers with a vertex capacity, or neither", nameof(buffers));
        if ((buffers.indices != null) != (buffers.index_capacity != 0))
            throw new ArgumentException("Give indices with an index capacity, or neither", nameof(buffers));
        _layout = layout;
        _buffers = buffers;
    }

    private static void RequireLayout(in MonstroVertexLayout layout)
    {
        if (layout.buffer_count < 1 || layout.buffer_count > 4)
            throw new ArgumentException($"The vertex layout has {layout.buffer_count} buffers; it needs 1 to 4", nameof(layout));
        for (int b = 0; b < layout.buffer_count; b++)
            if (layout.stride[b] == 0) throw new ArgumentException($"Vertex buffer {b} has stride 0", nameof(layout));
    }

    public MonstroVertexLayout Layout => _layout;
    public int VertexCapacity => checked((int)Buffers.vertex_capacity);
    public int IndexCapacity => checked((int)Buffers.index_capacity);

    /// <summary>Vertex buffer <paramref name="buffer"/> of the layout: VertexCapacity strides; empty if the canvas keeps the vertices.</summary>
    public Span<byte> VertexBuffer(int buffer)
    {
        MonstroFrameBuffers buffers = Buffers;
        if ((uint)buffer >= _layout.buffer_count) throw new ArgumentOutOfRangeException(nameof(buffer));
        if (_vertexArrays != null) return _vertexArrays[buffer];
        return new Span<byte>((void*)buffers.vertex_buffers[buffer], checked((int)(_layout.stride[buffer] * (long)buffers.vertex_capacity)));
    }

    /// <summary>The indices: IndexCapacity of them; empty if the canvas keeps the indices.</summary>
    public Span<uint> Indices
    {
        get
        {
            MonstroFrameBuffers buffers = Buffers;
            if (_indexArray != null) return _indexArray;
            return new Span<uint>(buffers.indices, checked((int)buffers.index_capacity));
        }
    }

    /// <summary>The native description of the buffers, for monstro_begin_frame_into.</summary>
    internal MonstroFrameBuffers Buffers
    {
        get { if (_disposed) throw new ObjectDisposedException(nameof(FrameBuffers)); return _buffers; }
    }

    public void Dispose()
    {
        if (InFrame) throw new InvalidOperationException("End the frame before disposing its buffers");
        _disposed = true;
        _buffers = default;
        _vertexArrays = null;
        _indexArray = null;
    }
}
