# Monstro

A C API immediate mode UI library with a C++ internal implementation.

## Features

- Immediate mode UI paradigm
- Graphics API agnostic - generates abstract draw commands
- Comprehensive widget set
- Flexible styling system
- Font rendering with TTF and bitmap font support, in one shared atlas
- One environment shared by many small UI contexts
- Fails hard and loudly on misuse and exhausted capacities instead of throwing
  or silently dropping output (see the [guide](docs/GUIDE.md#errors-and-limits))

## Building

```bash
./buildy
./tests/run.sh   # headless tests, normal and with ASan/UBSan
```

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
