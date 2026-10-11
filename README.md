# Monstro

A C API immediate mode UI library with a C++ internal implementation.

## Features

- Immediate mode UI paradigm
- Graphics API agnostic - generates abstract draw commands
- Comprehensive widget set
- Flexible styling system
- Font rendering with TTF and bitmap font support, in one shared atlas
- One environment shared by many small UI contexts
- Interaction a host can rely on: one hovered item (the one on top), no hover
  where an item is clipped away, activated and deactivated edges, IDs that
  survive live labels, and queued input so no click is lost between frames
  and a tap fires in the next frame (see the
  [guide](docs/GUIDE.md#interaction-rules))
- Fails hard and loudly on misuse and exhausted capacities instead of throwing
  or silently dropping output (see the [guide](docs/GUIDE.md#errors-and-limits))
- What game cockpit screens need: page tabs whose selection the host keeps,
  vertical sliders and progress bars, knobs that point where the pointer is,
  lit buttons with centred text, ellipses, text turned about a point (a
  horizon's pitch ladder), host textures such as camera feeds, and no geometry
  for anything wholly clipped away (see the `cockpit` example)

## Building

Monstro builds with [buildy](https://github.com/bholcomb/buildy): put a buildy
from its releases on your `PATH` (CI uses the release pinned in
`ci/buildy.json`).

```bash
buildy
./tests/run.sh   # headless tests, normal and with ASan/UBSan
```

The library and tests need nothing else (`buildy --target libmonstro` or
`--target monstro_tests`). The library is named `libmonstro` on every platform
(`libmonstro.so`, `libmonstro.dylib`, `libmonstro.dll`), which the C# binding
imports; on Windows, `monstro.dll` would be the binding's own `Monstro.dll`,
since file names ignore case. The examples render with
[VulkEase](https://github.com/bholcomb/vulkease), which needs Vulkan 1.4, so
they need the LunarG Vulkan SDK and a built VulkEase checkout; without them
the examples stop at compile time and say which is missing. Source the SDK's
environment script first. VulkEase is found at `VULKEASE_ROOT`, by default
`../vulkease` next to this repository, and its `dist/` must hold the library
and headers (run `buildy` there first; a debug build turns on the
validation layers).

```bash
source /path/to/vulkan/1.4.350.1/setup-env.sh
export VULKEASE_ROOT=/path/to/vulkease   # only if it is not ../vulkease
buildy
```

Run the examples from `bin/`, where their shaders and fonts are staged, in a
shell with the SDK environment and VulkEase on the library path:

```bash
export LD_LIBRARY_PATH="${VULKEASE_ROOT:-$PWD/../vulkease}/dist/bin:$LD_LIBRARY_PATH"
cd bin && ./monstro_basic
```

### Continuous integration and releases

`.github/workflows/build.yml` builds the shared library on Linux x64, Windows
x64 (MSVC) and macOS arm64 with `ci/build.py`, and tests it there: on all
three, that it loads and exports every function the binding imports, and the
.NET binding's checks; on Linux and macOS, also the headless suite (it needs
`fork`). Pushes and pull requests build and test. A pushed version tag also
publishes a GitHub release holding `linux-x64-libmonstro.so`,
`win-x64-libmonstro.dll`, `osx-arm64-libmonstro.dylib` and their `SHA256SUMS`:

```bash
git tag v0.1.0
git push origin v0.1.0
```

Each job first runs `ci/fetch_buildy.py`, which downloads the buildy release
pinned in `ci/buildy.json` (its version and each platform archive's SHA-256)
and fails on a hash mismatch or while the pin is unset. `python3 ci/build.py`
runs the same build and tests locally with the buildy on your `PATH` (or
`--buildy PATH`); on Windows, run it from a Visual Studio developer prompt.

## Example

```c
#include <monstro.h>

// One environment holds the fonts, themes and frame scratch every UI shares
MonstroEnv* env = monstro_init(NULL);
monstro_font_load_ttf_from_file(env, "default", "font.ttf", 16.0f, 32, 96);
MonstroTextureData atlas = monstro_env_get_atlas(env);
monstro_env_set_atlas_texture(env, upload_texture(atlas.data, atlas.width, atlas.height));

// One context per UI
MonstroContext* ctx = monstro_context_create(env, NULL);
monstro_set_display_size(ctx, width, height);

// Main loop
while (running) {
    monstro_begin_frame(ctx, delta_time);

    if (monstro_begin_window(ctx, "Demo", NULL, MONSTRO_WINDOW_DEFAULT)) {
        if (monstro_button(ctx, "Click Me!", NULL)) {
            // Button was clicked
        }
        monstro_end_window(ctx);
    }

    const MonstroDrawData* draw = monstro_end_frame(ctx);

    // Vertices, indices and draw commands, valid until the next frame begins
    for (uint32_t i = 0; i < draw->command_count; i++) {
        const MonstroDrawCommand* cmd = &draw->commands[i];
        // ... draw cmd->index_count indices from cmd->index_offset,
        //     base vertex cmd->vertex_offset, texture cmd->texture_id
    }
}

monstro_context_destroy(ctx);
monstro_shutdown(env);
```

See the [guide](docs/GUIDE.md) for the environment and context model, the
renderer integration and host buffers.

## .NET bindings

The .NET 8 binding is in [`bindings/csharp/`](bindings/csharp/README.md), with a
one-to-one C API mirror with typed handles, disposable environment and context
wrappers with zero-copy draw data, and native-backed tests.

## License

[MIT](LICENSE)
