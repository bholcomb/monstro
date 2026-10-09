#!/bin/bash
# Build and run the headless test suite: once normally and once under
# AddressSanitizer and UndefinedBehaviorSanitizer. Run from anywhere; pass a
# test-name filter as the first argument to run a subset. The normal build is
# also a buildy target: ./buildy --target monstro_tests (run it from the root).

set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
out="$root/build/tests"
mkdir -p "$out"

sources=("$root"/src/*.cpp "$root"/src/widgets/*.cpp "$root"/tests/*.cpp)
# The example widget libraries are smoke-tested too; they are not held to -Werror.
examples=(console curve node plot property shortcut timeline toast)
flags=(-std=c++17 -Wall -Wextra -Wshadow -Werror -g -I"$root/include" -I"$root/externals" -I"$root/src")

build() {
   local name="$1"
   shift
   echo "== building $name"
   local objects=()
   mkdir -p "$out/$name.examples"
   for example in "${examples[@]}"; do
      c++ -std=c++17 -g -I"$root/include" "$@" -c "$root/examples/$example/$example.cpp" \
         -o "$out/$name.examples/$example.o"
      objects+=("$out/$name.examples/$example.o")
   done
   c++ "${flags[@]}" "$@" "${sources[@]}" "${objects[@]}" -o "$out/$name"
}

build monstro_tests -O2
build monstro_tests_sanitized -O1 -fno-omit-frame-pointer -fsanitize=address,undefined -fno-sanitize-recover=undefined

# The library keeps no mutable global state: only toolchain and read-only
# third-party data may sit in writable sections.
echo "== checking for mutable global state"
c++ -std=c++17 -O2 -fPIC -shared -I"$root/include" -I"$root/externals" "$root"/src/*.cpp "$root"/src/widgets/*.cpp \
   -o "$out/libmonstro_globals.so"
globals=$(nm -C --defined-only "$out/libmonstro_globals.so" | awk '$2 ~ /^[bBdD]$/ {print $3}' |
   grep -v -x -E 'completed\.0|__dso_handle|DW\.ref\.__gxx_personality_v0|_DYNAMIC|_GLOBAL_OFFSET_TABLE_|__TMC_END__|__do_global_dtors_aux_fini_array_entry|__frame_dummy_init_array_entry|stbi__stdio_callbacks' ||
   true)
if [ -n "$globals" ]; then
   echo "mutable global state found:"
   echo "$globals"
   exit 1
fi

cd "$root"
echo "== running monstro_tests"
"$out/monstro_tests" "$@"
echo "== running monstro_tests_sanitized"
ASAN_OPTIONS=detect_leaks=1 "$out/monstro_tests_sanitized" "$@"
