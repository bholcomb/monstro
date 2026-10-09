#!/bin/bash
# Build and run the C# binding's tests against an already-built native library.
set -euo pipefail
here="$(cd "$(dirname "$0")" && pwd)"
native="${MONSTRO_NATIVE:-$here/../../build/linux-x86_64-release/bin/libmonstro.so}"
dotnet build "$here/Monstro.Tests" -c Release -v q -nologo
python3 "$here/generate.py" --check
probe="$here/Monstro.Tests/bin/Release/net8.0"
cc -std=c11 -shared -fPIC -Wall -Wextra -Werror \
    "$here/Monstro.Tests/abi.c" -I "$here/../../include" \
    -o "$probe/libmonstro-binding-test.so"
LD_LIBRARY_PATH="$(dirname "$native"):$probe${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" \
    dotnet "$probe/Monstro.Tests.dll" "$native" "$here/../../examples/basic/data/freeSans.ttf"
