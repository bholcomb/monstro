# Monstro

A C API immediate mode UI library with a C++ internal implementation.

## Features

- Immediate mode UI paradigm
- Graphics API agnostic - generates abstract draw commands
- Comprehensive widget set
- Flexible styling system
- Font rendering with TTF and bitmap font support

## Building

```bash
./buildy
```

## Example

```c
#include <monstro.h>

// Create context
MonstroContext* ctx = monstro_create_context();

// Main loop
while (running) {
    monstro_begin_frame(ctx, width, height, delta_time);
    
    if (monstro_begin_window(ctx, "Demo", NULL, MONSTRO_WINDOW_DEFAULT)) {
        if (monstro_button(ctx, "Click Me!", NULL)) {
            // Button was clicked
        }
    }
    monstro_end_window(ctx);
    
    monstro_end_frame(ctx);
    
    // Get draw commands and render
    uint32_t count;
    const MonstroDrawCommand* cmds = monstro_canvas_get_draw_commands(
        monstro_get_canvas(ctx), &count);
    // ... render using your graphics API
}

monstro_destroy_context(ctx);
```

## .NET bindings

The .NET 8 binding is in [`bindings/csharp/`](bindings/csharp/README.md), with a
one-to-one C API mirror, a disposable UI context wrapper, and native-backed tests.

## License

[MIT](LICENSE)
