#!/usr/bin/env python3
"""The CI build: monstro's shared library for this platform, built with buildy, then the tests that
run here, then the archive the release publishes for this platform, alone in build/ci/out/:
monstro-<version>-linux-x64.tar.gz, monstro-<version>-osx-arm64.tar.gz or
monstro-<version>-win-x64.zip. Its one top-level directory, monstro-<version>-<rid>/, holds
include/ (the public headers as in the repository), LICENSE and the library, libmonstro on every
platform: lib/libmonstro.so, lib/libmonstro.dylib, or bin/libmonstro.dll with its import library
lib/libmonstro.lib.

Run it from anywhere; Windows needs the MSVC developer environment (cl.exe, link.exe, dumpbin.exe)."""

import argparse
import ctypes
from pathlib import Path
import platform
import re
import shutil
import subprocess
import sys
import tarfile
import zipfile

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


def check_library_name(rid, built):
    """A program linked against the library records the library's own name (its SONAME on Linux,
    its install name on macOS), not the path it was linked from, so it loads from wherever the
    program ships it. src/buildy.yaml sets both; Windows records the DLL's file name."""
    if rid == "linux-x64":
        dynamic = subprocess.run(["readelf", "-d", str(built)], check=True, capture_output=True, text=True).stdout
        names = re.findall(r"\(SONAME\)\s+Library soname: \[(.*)\]", dynamic)
        expected = ["libmonstro.so"]
    elif rid == "osx-arm64":
        names = subprocess.run(["otool", "-D", str(built)], check=True, capture_output=True, text=True).stdout.splitlines()[1:]
        expected = ["@rpath/libmonstro.dylib"]
    else:
        return
    if names != expected:
        raise SystemExit(f"{built} is named {names} for the programs that link it; expected {expected}")
    print(f"{built.name}: programs that link it load {expected[0]}", flush=True)


def package(rid, version, built):
    """Writes this platform's release archive to build/ci/out/ and checks that it holds exactly the
    expected files, each identical to its source. Entries are sorted, owned by root, with
    normalized permissions."""
    top = f"monstro-{version}-{rid}"
    files = {f"{top}/LICENSE": ROOT / "LICENSE"}
    for header in sorted((ROOT / "include").rglob("*")):
        if header.is_file():
            files[f"{top}/{header.relative_to(ROOT).as_posix()}"] = header
    if rid == "win-x64":
        implib = built.parents[1] / "lib/libmonstro.lib"
        if not implib.is_file():
            raise SystemExit(f"buildy built {built} without its import library {implib}")
        files[f"{top}/bin/{built.name}"] = built
        files[f"{top}/lib/{implib.name}"] = implib
    else:
        files[f"{top}/lib/{built.name}"] = built
    directories = {f"{top}/"}
    for name in files:
        parts = name.split("/")[:-1]
        directories.update("/".join(parts[:end]) + "/" for end in range(1, len(parts) + 1))
    expected = sorted(directories | set(files))

    out = WORK / "out"
    out.mkdir()
    if rid == "win-x64":
        archive = out / f"{top}.zip"
        with zipfile.ZipFile(archive, "w", zipfile.ZIP_DEFLATED) as zipped:
            for name in expected:
                zipped.write(files[name] if name in files else ROOT, name)
        with zipfile.ZipFile(archive) as zipped:
            found = sorted(zipped.namelist())
            contents = {name: zipped.read(name) for name in found if not name.endswith("/")}
    else:
        archive = out / f"{top}.tar.gz"

        def normalize(info):
            info.uid = info.gid = 0
            info.uname = info.gname = ""
            info.mode = 0o755 if info.isdir() or info.name.endswith(built.name) else 0o644
            return info

        with tarfile.open(archive, "w:gz") as tarred:
            for name in expected:
                tarred.add(files[name] if name in files else ROOT, name.rstrip("/"), recursive=False, filter=normalize)
        with tarfile.open(archive) as tarred:
            members = tarred.getmembers()
            found = sorted(member.name + "/" if member.isdir() else member.name for member in members)
            contents = {member.name: tarred.extractfile(member).read() for member in members if member.isfile()}
    if found != expected:
        missing = sorted(set(expected) - set(found))
        extra = sorted(set(found) - set(expected))
        raise SystemExit(f"{archive} is missing {missing} and has extra (or repeated) entries {extra or 'none'}")
    different = [name for name, source in files.items() if contents.get(name) != source.read_bytes()]
    if different:
        raise SystemExit(f"{archive} holds {different} different from their sources")
    if sorted(out.iterdir()) != [archive]:
        raise SystemExit(f"{out} holds {sorted(path.name for path in out.iterdir())}; expected only {archive.name}")
    print(f"Built {archive}:", *expected, sep="\n  ", flush=True)


def short_commit():
    """The checkout's short commit: the version of a build that is not a release."""
    result = subprocess.run(["git", "rev-parse", "--short", "HEAD"], cwd=ROOT, capture_output=True, text=True)
    if result.returncode != 0:
        raise SystemExit(f"{ROOT} is not a git checkout ({result.stderr.strip()}); pass --version")
    return result.stdout.strip()


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
    parser.add_argument("--version",
                        help="The version in the archive's name: a release's tag without its v, as 0.1.0 (default: the short commit)")
    args = parser.parse_args()
    version = args.version or short_commit()
    if not re.fullmatch(r"[0-9a-zA-Z][0-9a-zA-Z.+-]*", version) or version.startswith("v"):
        parser.error(f"--version {version!r}: expected a tag without its v (0.1.0) or a commit")
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
    check_library_name(rid, built)
    load_check(built)
    binding_tests(rid, library, built)
    if rid == "win-x64":
        # A game ships this DLL to machines without the Visual C++ redistributable: the C runtime is
        # linked in (cl's default, /MT), never imported.
        dependents = subprocess.run(["dumpbin", "/dependents", str(built)], check=True, capture_output=True, text=True).stdout
        print(dependents, flush=True)
        if "vcruntime" in dependents.lower() or "msvcp" in dependents.lower():
            raise SystemExit(f"{built} imports the Visual C++ runtime; link it statically (/MT)")

    package(rid, version, built)


if __name__ == "__main__":
    main()
