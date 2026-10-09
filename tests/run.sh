#!/bin/bash
# Build and run the headless test suite: once normally and once under
# AddressSanitizer and UndefinedBehaviorSanitizer. Run from anywhere; pass a
# test-name filter as the first argument to run a subset.
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
out="$root/build/tests"
mkdir -p "$out"

sources=("$root"/src/*.cpp "$root"/src/widgets/*.cpp "$root"/tests/*.cpp)
flags=(-std=c++17 -Wall -Wextra -Wshadow -Werror -g -I"$root/include" -I"$root/externals" -I"$root/src")

build() {
   local name="$1"
   shift
   echo "== building $name"
   c++ "${flags[@]}" "$@" "${sources[@]}" -o "$out/$name"
}

build monstro_tests -O2
build monstro_tests_sanitized -O1 -fno-omit-frame-pointer -fsanitize=address,undefined -fno-sanitize-recover=undefined

cd "$root"
echo "== running monstro_tests"
"$out/monstro_tests" "$@"
echo "== running monstro_tests_sanitized"
ASAN_OPTIONS=detect_leaks=1 "$out/monstro_tests_sanitized" "$@"
