# Monstro for C#

A .NET 8 binding over Monstro's C API, following the Mobius C# binding's two layers:

- `Native`: the whole public C API, one to one, under its C names. All enums,
  constants, styles, draw structs, and font structs are included. C's inline
  color and rectangle helpers have managed implementations.
- `MonstroContext`: an optional convenience layer for context ownership, frames,
  input, windows, common widgets, font loading, and copied renderer data.
  `context.Handle` is the native context pointer for mixing the layers.

Reference `Monstro/Monstro.csproj`. There are no managed package dependencies.
The native library must be on the loader's search path: `libmonstro.so` on Linux,
`monstro.dll` on Windows, or `libmonstro.dylib` on macOS. Only Linux x86_64 has been
validated. Build and ship the native library separately from `Monstro.dll`.
No graphics API, window toolkit, or renderer is part of the binding.

```csharp
using Monstro;

using var ui = new MonstroContext();
var font = ui.LoadFont("font.ttf", 18);
// Upload font.Pixels using font.Width, font.Height, and font.Format.
font.SetTexture(rendererTextureId);

// Feed the host's input before each frame.
ui.SetMousePosition(mouseX, mouseY);
ui.SetMouseButton(MonstroMouseButton.Left, leftDown);
ui.BeginFrame(width, height, deltaTime);
if (ui.BeginWindow("Controls"))
{
    ui.Label("Hello from C#");
    if (ui.Button("Launch")) Launch();
    ui.Checkbox("Enabled", ref enabled);
    ui.SliderFloat("Throttle", ref throttle, 0, 1);
}
ui.EndWindow();
ui.EndFrame();
var draw = ui.GetDrawData();
// Render draw.Vertices and draw.Indices using draw.Commands' textures and clips.
```

`GetDrawData` copies the native buffers, so the data survives the next frame and
context disposal. For allocation-free rendering, use `Native`'s borrowed buffer
getters instead; those pointers expire when the context changes or is destroyed.
Copied commands still reference host textures and custom callbacks: the host must
keep those resources alive until it finishes rendering the copied data.
Fonts loaded by `LoadFont` belong to the context and are destroyed with it; the
atlas pixels are a managed copy. Do not destroy or register those fonts again
through `Native`. Dispose contexts explicitly, as with MobiusState.
`InputText` copies the string into a UTF-8 buffer of `capacityBytes` for each
call; native editing keeps only the cursor and selection between frames.

Use one context on one thread at a time, including disposal. Balance every
window/layout/ID/style/disabled stack before `EndFrame`: native Monstro never
throws, and it aborts the process with a message on misuse, unbalanced stacks
or exhausted capacities (see docs/GUIDE.md, Errors and Limits). When mixing the two layers, manage the raw API's
lifetimes and stack pairing yourself. The convenience `BeginWindow` has no close
pointer. With `Native.monstro_begin_window`, `byte* p_open` must stay valid through
`monstro_end_window`; if it was already zero, the window is not begun and must not
be ended. A collapsed window still needs `monstro_end_window`.

## C API mapping

| C | C# |
| --- | --- |
| opaque pointers (`MonstroContext*`, `MonstroFont*`, ...) | `IntPtr` |
| `size_t` | `nuint` |
| `bool` parameter/result | `bool`, marshalled as one byte |
| `bool*`, bool struct fields | `byte*`, `byte` (0 or 1) |
| `const char*` input | nullable UTF-8 `string` |
| `char*` buffer, `const char*` result | `byte*`; decode immediately with `Native.Utf8` |
| arrays and out pointers | unsafe pointers; pin managed storage for the call |
| C structs/unions | sequential/explicit blittable structs, fixed inline arrays |
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
constants against a C probe, then exercises draw output, multiple frames, UTF-8,
one-byte booleans, pointers, font atlases, callbacks, styles, and disposal.

Regenerate the committed C API mirror and ABI probe after public header changes:

```sh
python3 bindings/csharp/generate.py
python3 bindings/csharp/generate.py --check
```

Generation needs Python 3 and `cc` for preprocessing. Normal .NET builds use the
committed generated files. The generator accepts the declarations used by these
headers and fails on unknown types or syntax rather than skipping declarations.
