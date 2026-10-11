#!/usr/bin/env python3
"""The CI build: monstro's shared library for this platform, built with buildy, then the tests that
run here. The library is libmonstro on every platform; it lands in build/ci/out/ named for its
platform (linux-x64-libmonstro.so, win-x64-libmonstro.dll, osx-arm64-libmonstro.dylib), as the
release publishes it.

Run it from anywhere; Windows needs the MSVC developer environment (cl.exe, link.exe, dumpbin.exe)."""

import argparse
import ctypes
from pathlib import Path
import platform
import re
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
WORK = ROOT / "build/ci"
# Each platform CI builds: its .NET runtime identifier and the library's file name.
PLATFORMS = {
    ("linux", "x86_64"): ("linux-x64", "libmonstro.so"),
    ("win32", "AMD64"): ("win-x64", "libmonstro.dll"),
    ("darwin", "arm64"): ("osx-arm64", "libmonstro.dylib"),
}


def run(*command, cwd=ROOT):
    print("+ " + " ".join(map(str, command)), flush=True)
    subprocess.run([str(part) for part in command], cwd=cwd, check=True)


def build_library(buildy, library):
    """Builds the library alone. The root workspace also holds the Vulkan examples, whose GLFW
    dependency buildy fetches and builds whatever the target; this workspace keeps the root's
    project and compiler settings and only the library's module."""
    workspace = WORK / "workspace"
    workspace.mkdir(parents=True)
    config = (ROOT / "buildy.yaml").read_text()
    for marker in ("\nworkspace:", "\nenvironment:", "\nstaging:"):
        if config.count(marker) != 1:
            raise SystemExit(f"buildy.yaml has {config.count(marker)} '{marker.strip()}' sections; ci/build.py expects one")
    environment = config.split("\nenvironment:", 1)[1].split("\nstaging:", 1)[0]
    project = config.split("\nworkspace:", 1)[0]
    (workspace / "buildy.yaml").write_text(project + "\nworkspace:\n  modules:\n    - ../../../src\n\nenvironment:" + environment + "\n")
    run(buildy, "--target", "libmonstro", "--config", "release", cwd=workspace)
    built = sorted(workspace.glob(f"build/*-release/bin/{library}"))
    if len(built) != 1:
        raise SystemExit(f"Expected one {library} from buildy, found {[str(path) for path in built]}")
    return built[0]


def load_check(built):
    """The library loads in a plain process, exports every function the binding imports, and makes
    and destroys an environment."""
    native = (ROOT / "bindings/csharp/Monstro/Native.cs").read_text()
    names = re.findall(r"public static extern [^(]+ (monstro_\w+)\(", native)
    library = ctypes.CDLL(str(built))
    missing = [name for name in names if not hasattr(library, name)]
    if missing:
        raise SystemExit(f"{built} does not export {len(missing)} of the binding's {len(names)} functions: {', '.join(missing)}")
    library.monstro_init.restype = ctypes.c_void_p
    library.monstro_shutdown.argtypes = [ctypes.c_void_p]
    env = library.monstro_init(None)
    if not env:
        raise SystemExit("monstro_init(NULL) returned NULL")
    library.monstro_shutdown(env)
    print(f"{built.name}: exports all {len(names)} functions; an environment made and destroyed", flush=True)


def binding_tests(rid, library, built):
    """The .NET binding's checks against this library: every export, struct layouts against the
    headers (the ABI probe), input, fonts and draw output. .NET finds both libraries beside the app."""
    dotnet = WORK / "dotnet"
    run("dotnet", "build", ROOT / "bindings/csharp/Monstro.Tests/Monstro.Tests.csproj", "-c", "Release",
        "--artifacts-path", dotnet, "--nologo", "-nr:false")
    app = dotnet / "bin/Monstro.Tests/release"
    probe = ROOT / "bindings/csharp/Monstro.Tests/abi.c"
    if rid == "win-x64":
        # MSVC exports only what is named: the probe's three functions.
        exports = [f"/EXPORT:monstro_binding_{name}" for name in ("sizeof", "offsetof", "constant")]
        run("cl", "/nologo", "/LD", "/I", ROOT / "include", probe, f"/Fo{WORK}\\", f"/Fe{app / 'monstro-binding-test.dll'}",
            "/link", *exports)
    elif rid == "osx-arm64":
        run("cc", "-std=c11", "-dynamiclib", "-Wall", "-Wextra", "-Werror", probe, "-I", ROOT / "include",
            "-o", app / "libmonstro-binding-test.dylib")
    else:
        run("cc", "-std=c11", "-shared", "-fPIC", "-Wall", "-Wextra", "-Werror", probe, "-I", ROOT / "include",
            "-o", app / "libmonstro-binding-test.so")
    shutil.copy2(built, app / library)
    run("dotnet", app / "Monstro.Tests.dll", app / library, ROOT / "examples/basic/data/freeSans.ttf")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--buildy", type=Path,
                        help="The buildy executable (default: buildy on PATH; CI passes the pinned release's)")
    args = parser.parse_args()
    key = (sys.platform, platform.machine())
    if key not in PLATFORMS:
        parser.error(f"CI builds Linux x86_64, Windows x64 and macOS arm64, not {key}")
    buildy = args.buildy.resolve() if args.buildy else shutil.which("buildy")
    if buildy is None:
        parser.error("No buildy on PATH: install one from a buildy release (https://github.com/bholcomb/buildy/releases), or pass --buildy")
    rid, library = PLATFORMS[key]
    shutil.rmtree(WORK, ignore_errors=True)
    built = build_library(buildy, library)

    if rid == "linux-x64":
        run(sys.executable, ROOT / "bindings/csharp/generate.py", "--check")
    # The headless suite (also under ASan and UBSan) runs where fork and POSIX pipes exist.
    if rid != "win-x64":
        run("bash", ROOT / "tests/run.sh")
    load_check(built)
    binding_tests(rid, library, built)
    if rid == "win-x64":
        # A game ships this DLL to machines without the Visual C++ redistributable: the C runtime is
        # linked in (cl's default, /MT), never imported.
        dependents = subprocess.run(["dumpbin", "/dependents", str(built)], check=True, capture_output=True, text=True).stdout
        print(dependents, flush=True)
        if "vcruntime" in dependents.lower() or "msvcp" in dependents.lower():
            raise SystemExit(f"{built} imports the Visual C++ runtime; link it statically (/MT)")

    release = WORK / "out" / f"{rid}-{library}"
    release.parent.mkdir()
    shutil.copy2(built, release)
    print(f"Built {release}")


if __name__ == "__main__":
    main()
