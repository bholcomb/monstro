# Monstro

A C API immediate mode UI library with a C++ internal implementation.

## Features

- Immediate mode UI paradigm
- Graphics API agnostic - generates abstract draw commands
- Comprehensive widget set
- Flexible styling system
- Font rendering with TTF and bitmap font support
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

// Create context
MonstroContext* ctx = monstro_context_create();
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

    monstro_end_frame(ctx);

    // Get vertices, indices and draw commands, then render
    const MonstroDrawCommand* cmds;
    uint32_t count;
    monstro_context_get_draw_commands(ctx, &cmds, &count);
    // ... render using your graphics API
}

monstro_context_destroy(ctx);
```

## .NET bindings

The .NET 8 binding is in [`bindings/csharp/`](bindings/csharp/README.md), with a
one-to-one C API mirror, a disposable UI context wrapper, and native-backed tests.

## License

[MIT](LICENSE)
