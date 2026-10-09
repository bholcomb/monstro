# Monstro for C#

A .NET 8 binding over Monstro's C API, following the Mobius C# binding's two layers:

- `Native`: the whole public C API, one to one, under its C names. All enums,
  constants, styles, draw structs, and font structs are included. Opaque C
  pointers are typed handle structs (`MonstroEnv`, `MonstroContext`,
  `MonstroFont`, `MonstroTheme`, `MonstroCanvas`, `MonstroFileDialogState`), so
  one cannot be passed where another is expected. C's inline color and
  rectangle helpers have managed implementations.
- A convenience layer: an `Env` owns the environment (fonts loaded from paths
  or bytes, themes, settings, the atlas), and each `Context` is one UI created
  from it, with frames, input, windows and common widgets. `Handle` on either
  is the native handle for mixing the layers.

Reference `Monstro/Monstro.csproj`. There are no managed package dependencies.
The native library must be on the loader's search path: `libmonstro.so` on Linux,
`monstro.dll` on Windows, or `libmonstro.dylib` on macOS. Only Linux x86_64 has been
validated. Build and ship the native library separately from `Monstro.dll`.
No graphics API, window toolkit, or renderer is part of the binding.

```csharp
using Monstro;

using var env = new Env();
env.LoadFont("sans", "font.ttf", 18);
env.AtlasTexture = rendererAtlasId;
// Upload env.Atlas.Pixels (RGBA8, Width x Height) again whenever env.Atlas.Version changes.

using var hud = env.CreateContext();
using var console = env.CreateContext();   // shares the fonts, atlas and themes

// Feed the host's input before each frame.
hud.SetMousePosition(mouseX, mouseY);
hud.SetMouseButton(MonstroMouseButton.Left, leftDown);
hud.BeginFrame(width, height, deltaTime);
if (hud.BeginWindow("Controls"))
{
    hud.Label("Hello from C#");
    if (hud.Button("Launch")) Launch();
    hud.Checkbox("Enabled", ref enabled);
    hud.SliderFloat("Throttle", ref throttle, 0, 1);
}
hud.EndWindow();
DrawData draw = hud.EndFrame();
if (draw.Changed)
    Render(draw.Vertices, draw.Indices, draw.Commands);
```

Each command draws `index_count` indices from `index_offset`, counting from its
`vertex_offset` (the base vertex); text and solid shapes carry the atlas texture
ID, so they batch together.

`DrawData` is a `ref struct` view of the native canvas that built the frame:
nothing is copied. It is valid until that canvas builds another frame. With
the default single canvas that is the next `BeginFrame` of any context on the
environment; with `canvas_count` N, canvases are lent in turn. Every property
checks the view and throws once it is stale (`IsValid` asks first); spans taken
from it are only as valid as the view. Consume the frame -- upload or render it
-- before the canvas builds the next one; there is no copy.

To build a frame straight into the host's own arrays, describe where vertex
attributes go once, in `MonstroEnvConfig.vertex_layout` (position, texcoord and
color, each in a buffer at an offset within its stride), and pass `FrameBuffers`
to `BeginFrame`: one `Memory<byte>` per layout buffer and a `Memory<uint>` for
the indices. They are pinned until `EndFrame` and only written; the vertex
capacity is the smallest buffer's length over its stride. The draw data then
reports `DrawnIntoHostVertexBuffers`/`DrawnIntoHostIndices`, and that geometry is
in the host's arrays rather than in `Vertices`/`Indices`. For mapped GPU memory,
call `Native.monstro_begin_frame_into` with a `MonstroFrameBuffers` of raw
pointers.

Fonts, themes and the atlas belong to the `Env` and live until it is disposed;
themes are immutable once registered, and `Context.Theme` picks one. Pushed
styles change only the frame's working copy. The environment is read-only while
any frame is open: loading fonts, registering themes or changing settings then
throws. Dispose contexts before their environment, explicitly, as with
MobiusState.

`InputText` copies the string into a UTF-8 buffer of `capacityBytes` for each
call; native editing keeps only the cursor and selection between frames.

Use an environment and its contexts on one thread at a time, including
disposal. Balance every window/layout/ID/style/disabled stack before `EndFrame`:
native Monstro never throws, and it aborts the process with a message on misuse,
null arguments, unbalanced stacks or exhausted capacities (see docs/GUIDE.md,
Errors and Limits). When mixing the two layers, manage the raw API's lifetimes
and stack pairing yourself. The convenience `BeginWindow` has no close pointer.
With `Native.monstro_begin_window`, `byte* p_open` must stay valid through
`monstro_end_window`; if it was already zero, the window is not begun and must
not be ended. A collapsed window still needs `monstro_end_window`.

## C API mapping

| C | C# |
| --- | --- |
| opaque pointers (`MonstroEnv*`, `MonstroContext*`, `MonstroFont*`, ...) | handle structs of the same name, with `Handle` and `IsNull` |
| `size_t` | `nuint` |
| `bool` parameter/result | `bool`, marshalled as one byte |
| `bool*`, bool struct fields | `byte*`, `byte` (0 or 1) |
| `const char*` input | nullable UTF-8 `string` |
| `char*` buffer, `const char*` result | `byte*`; decode immediately with `Native.Utf8` |
| arrays and out pointers | unsafe pointers; pin managed storage for the call |
| C structs/unions | sequential/explicit blittable structs, fixed inline arrays |
| arrays of pointers in structs | `fixed ulong` (64-bit platforms) |
| custom draw callback | `delegate* unmanaged[Cdecl]<IntPtr, float*, int, void>` |

Native text editing uses its mutable buffer only during the call; pin it for
that call. Pass the same text each frame (the buffer may move between frames).

Callbacks must be static `[UnmanagedCallersOnly(CallConvs = new[] {
typeof(CallConvCdecl) })]` methods. A renderer invokes custom draw commands; the
callback pointer and user data must remain valid until the commands are consumed.
Never throw from a callback into native code. Mutable text buffer capacities count
UTF-8 bytes including the terminating NUL. Match the native library revision to
these headers; loading a different ABI is unsupported.

## Tests and regeneration

After building the native library, run `./bindings/csharp/test.sh`. Set
`MONSTRO_NATIVE` to its absolute path if it is not in the default Buildy release
output directory. The console runner uses the real library and bundled example
font. It checks every exported entry point, all struct sizes/offsets and enum
constants against a C probe, and that no entry point takes or returns a bare
`IntPtr`. It then exercises draw output and its validity, multiple
frames, UTF-8, one-byte booleans, pointers, the shared atlas, fonts from paths
and bytes, themes shared by contexts with pushes kept in the frame, settings,
host buffers in split streams, callbacks, and disposal.

Regenerate the committed C API mirror and ABI probe after public header changes:

```sh
python3 bindings/csharp/generate.py
python3 bindings/csharp/generate.py --check
```

Generation needs Python 3 and `cc` for preprocessing. Normal .NET builds use the
committed generated files. The generator accepts the declarations used by these
headers and fails on unknown types or syntax rather than skipping declarations.
