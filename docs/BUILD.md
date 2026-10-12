# Building Monstro

Prebuilt libraries are on the [releases](https://github.com/bholcomb/monstro/releases)
page. This page builds Monstro from source, runs its tests and builds the
examples.

## Prerequisites

- [buildy](https://github.com/bholcomb/buildy) on your `PATH`. CI builds with
  the release pinned in `ci/buildy.json`; `python3 ci/fetch_buildy.py DIR`
  downloads that release into `DIR` and checks its SHA-256.
- A C++17 compiler:
  - Linux: GCC.
  - macOS: Clang, from the Xcode Command Line Tools.
  - Windows: MSVC, from Visual Studio with the x64 C++ tools. Build from an x64
    developer prompt.
- Python 3, for `ci/build.py` and the binding generator.
- The .NET 8 SDK, for the C# binding and its checks.

The library and its tests need nothing else. The examples need more; see
[Examples](#examples).

## The library

From the repository root:

```sh
buildy --target libmonstro -c release
```

The library lands in `build/<platform>-<arch>-release/bin/`, for example
`build/linux-x86_64-release/bin/libmonstro.so`; on Windows, the import library
`libmonstro.lib` is in `lib/` beside `bin/`. Leave out `-c release` for a debug
build. From the root, buildy also fetches and builds GLFW for the examples,
even for this target, so the first build needs network access.

The library is named `libmonstro` on every platform (`libmonstro.so`,
`libmonstro.dll`, `libmonstro.dylib`): Windows file names ignore case, so
`monstro.dll` would clash with the C# binding's `Monstro.dll`.

Programs include `include/monstro.h` and link `libmonstro`.

## Tests

The headless suite, on Linux and macOS:

```sh
./tests/run.sh            # every test
./tests/run.sh text       # only tests whose names match a filter
```

It builds and runs the suite twice, normally and under AddressSanitizer and
UndefinedBehaviorSanitizer, with the `c++` on your `PATH`. `buildy --target
monstro_tests` builds the plain suite as well; run it from the repository root.

The C# binding's checks, after building the release library on Linux:

```sh
./bindings/csharp/test.sh
```

Set `MONSTRO_NATIVE` to the library's absolute path if it is not in
`build/linux-x86_64-release/bin/`. After changing a public header, regenerate
the binding's C API mirror and ABI probe:

```sh
python3 bindings/csharp/generate.py
python3 bindings/csharp/generate.py --check   # fails if the committed files are stale
```

### Everything CI runs

```sh
python3 ci/build.py [--buildy PATH]
```

This is what CI runs on each platform: it builds the release library alone,
runs the headless suite (not on Windows), checks that the library exports every
function the binding imports, runs the binding's checks, and on Windows checks
that the C runtime is linked in. It leaves the library in `build/ci/out/`, named
as a release names it. On Windows, run it from an x64 developer prompt.

## Examples

The examples render with [VulkEase](https://github.com/bholcomb/vulkease),
which needs Vulkan 1.4. They need:

- The [LunarG Vulkan SDK](https://vulkan.lunarg.com/), 1.4 or later, with its
  environment set (on Linux, source its `setup-env.sh`; distribution packages
  ship an older loader).
- A VulkEase checkout, built so its `dist/` holds the library and headers (run
  `buildy` there; a debug build turns on the validation layers). Monstro looks
  for it at `VULKEASE_ROOT`, by default `../vulkease` next to this repository.
- GLFW: buildy fetches and builds it on Linux and Windows; on macOS, install it
  with Homebrew.

Without the SDK or VulkEase, the examples stop at compile time and say which is
missing. Build everything, examples included:

```sh
source /path/to/vulkan/1.4.x/setup-env.sh
export VULKEASE_ROOT=/path/to/vulkease   # only if it is not ../vulkease
buildy
```

The examples, their shaders and fonts are staged in `bin/`. Run them from
there, with VulkEase on the library path:

```sh
export LD_LIBRARY_PATH="${VULKEASE_ROOT:-$PWD/../vulkease}/dist/bin:$LD_LIBRARY_PATH"
cd bin && ./monstro_basic
```

The others are `monstro_interaction`, `monstro_cockpit`, `monstro_console`,
`monstro_node`, `monstro_plot`, `monstro_curve`, `monstro_timeline`,
`monstro_property`, `monstro_shortcut` and `monstro_toast`; the
[guide](GUIDE.md#examples) describes each.

## Releases

`.github/workflows/build.yml` builds and tests the library on Linux x64,
Windows x64 and macOS arm64 for every push to `main` and every pull request. A
pushed version tag also publishes a GitHub release with each platform's
library and a `SHA256SUMS` file:

```sh
git tag v0.1.0
git push origin v0.1.0
```
