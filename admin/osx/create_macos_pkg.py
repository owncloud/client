#!/usr/bin/env python3
"""macOS packaging script for the ownCloud/Kiteworks desktop client.

Produces a .pkg installer from a completed conan build.

Prerequisites:
  - A completed `conan build .` with the macOS profile
  - packagesbuild (from http://s.sudre.free.fr/Software/Packages/about.html)

Usage: build_macos_pkg.py --build-dir <path> --output-dir <path>
"""

import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path


def run(cmd, **kwargs):
    print(f"  $ {' '.join(str(c) for c in cmd)}")
    return subprocess.run(cmd, check=True, **kwargs)


def otool_deps(binary):
    """Return list of dylib references from otool -L."""
    result = subprocess.run(
        ["otool", "-L", str(binary)], capture_output=True, text=True
    )
    deps = []
    for line in result.stdout.splitlines()[1:]:
        match = re.match(r"\s+(\S+)", line)
        if match:
            deps.append(match.group(1))
    return deps


def otool_rpaths(binary):
    """Return list of LC_RPATH entries from a Mach-O binary."""
    result = subprocess.run(
        ["otool", "-l", str(binary)], capture_output=True, text=True
    )
    rpaths = []
    lines = result.stdout.splitlines()
    for i, line in enumerate(lines):
        if "LC_RPATH" in line and i + 2 < len(lines):
            match = re.match(r"\s+path\s+(\S+)", lines[i + 2])
            if match:
                rpaths.append(match.group(1))
    return rpaths


def resolve_rpath_ref(ref, source_binary):
    """Resolve an @rpath/ reference to an actual file path."""
    libname = ref.removeprefix("@rpath/")
    for rpath in otool_rpaths(source_binary):
        resolved = rpath.replace("@loader_path", str(source_binary.parent))
        candidate = Path(resolved) / libname
        if candidate.exists():
            return candidate
    return None


def is_qt_ref(ref):
    return "/Qt" in ref or ref.startswith("@rpath/libQt")


def is_system_ref(ref):
    return ref.startswith("/usr/lib/") or ref.startswith("/System/Library/")


def copy_lib(src, dest_dir):
    """Copy a dylib, preserving symlinks."""
    dest = dest_dir / src.name
    if dest.exists():
        return
    if src.is_symlink():
        linkto = os.readlink(src)
        os.symlink(linkto, dest)
    else:
        shutil.copy2(src, dest)


def copy_client_libs(install_dir, frameworks_dir):
    """Copy client dylibs (real files + symlinks) into Frameworks."""
    print("=== Copying client libraries ===")
    lib_dir = install_dir / "lib"
    if not lib_dir.exists():
        return
    real_files = sorted(p for p in lib_dir.glob("*.dylib") if not p.is_symlink())
    symlinks = sorted(p for p in lib_dir.glob("*.dylib") if p.is_symlink())
    for lib in real_files:
        print(f"  {lib.name}")
        copy_lib(lib, frameworks_dir)
    for lib in symlinks:
        copy_lib(lib, frameworks_dir)


def copy_qml_modules(install_dir, resources_dir):
    """Copy client QML modules into Resources/qml."""
    print("=== Copying QML modules ===")
    qml_src = install_dir / "lib" / "qml"
    if not qml_src.exists():
        return
    qml_dest = resources_dir / "qml"
    shutil.copytree(qml_src, qml_dest, dirs_exist_ok=True, symlinks=True)


def resolve_conan_deps(executable, frameworks_dir):
    """Find and copy non-Qt Conan dependencies into Frameworks."""
    print("=== Resolving Conan library dependencies ===")
    binaries_to_scan = [executable]
    scanned = set()

    while binaries_to_scan:
        binary = binaries_to_scan.pop(0)
        if binary in scanned:
            continue
        scanned.add(binary)

        for ref in otool_deps(binary):
            if not ref.startswith("@rpath/"):
                continue
            if is_qt_ref(ref):
                continue

            libname = ref.removeprefix("@rpath/")
            dest = frameworks_dir / libname
            if dest.exists():
                continue

            resolved = resolve_rpath_ref(ref, binary)
            if resolved:
                print(f"  {libname}")
                copy_lib(resolved, frameworks_dir)
                if not resolved.is_symlink():
                    binaries_to_scan.append(dest)
            else:
                print(f"  WARNING: could not resolve {ref}")


def fix_install_names(binary, frameworks_dir):
    """Rewrite dylib references to use @executable_path/../Frameworks/."""
    changes = []
    for ref in otool_deps(binary):
        if is_system_ref(ref):
            continue

        base = Path(ref).name

        # Absolute paths to libs we have in Frameworks
        if not ref.startswith("@") and (frameworks_dir / base).exists():
            new = f"@executable_path/../Frameworks/{base}"
            changes.append(("-change", ref, new))

        # @rpath references to non-Qt libs we copied
        elif ref.startswith("@rpath/") and not is_qt_ref(ref):
            libname = ref.removeprefix("@rpath/")
            if (frameworks_dir / libname).exists():
                new = f"@executable_path/../Frameworks/{libname}"
                changes.append(("-change", ref, new))

    if changes:
        args = []
        for change in changes:
            args.extend(change)
            print(f"  {change[1]} -> {Path(change[2]).name} (in {binary.name})")
        subprocess.run(
            ["install_name_tool"] + args + [str(binary)], check=True
        )

    # Fix the library's own id
    if binary.suffix == ".dylib":
        new_id = f"@executable_path/../Frameworks/{binary.name}"
        subprocess.run(
            ["install_name_tool", "-id", new_id, str(binary)], check=True
        )


def fix_all_install_names(executable, frameworks_dir, qml_dir):
    """Fix install names for the executable, all Frameworks dylibs, and QML plugins."""
    print("=== Fixing install names ===")

    fix_install_names(executable, frameworks_dir)

    for lib in sorted(frameworks_dir.glob("*.dylib")):
        if lib.is_symlink():
            continue
        fix_install_names(lib, frameworks_dir)

    if qml_dir.exists():
        for lib in sorted(qml_dir.rglob("*.dylib")):
            if lib.is_symlink():
                continue
            fix_install_names(lib, frameworks_dir)


def find_macdeployqt(build_dir):
    """Find macdeployqt by sourcing the Conan build environment."""
    generators = build_dir / "generators"
    env_scripts = sorted(generators.glob("conanbuildenv-*.sh"))
    run_scripts = sorted(generators.glob("conanrunenv-*.sh"))

    env = os.environ.copy()
    for script in env_scripts + run_scripts:
        result = subprocess.run(
            ["bash", "-c", f"source {script} && env"],
            capture_output=True, text=True
        )
        for line in result.stdout.splitlines():
            key, _, value = line.partition("=")
            if key:
                env[key] = value

    for path_dir in env.get("PATH", "").split(":"):
        candidate = Path(path_dir) / "macdeployqt"
        if candidate.is_file():
            return candidate, env

    return None, env


def run_macdeployqt(macdeployqt, app_bundle, frameworks_dir, qml_dir):
    """Run macdeployqt with -executable= for each bundled dylib."""
    print("=== Running macdeployqt ===")

    qt_lib_dir = macdeployqt.parent.parent / "lib"

    exec_args = []
    for lib in sorted(frameworks_dir.glob("*.dylib")):
        if lib.is_symlink():
            continue
        exec_args.append(f"-executable={lib}")

    cmd = [str(macdeployqt), str(app_bundle)] + exec_args
    cmd.append(f"-libpath={qt_lib_dir}")
    if qml_dir.exists():
        cmd.append(f"-qmldir={qml_dir}")

    result = subprocess.run(cmd, capture_output=True, text=True)
    # macdeployqt prints ERRORs for non-Mach-O files (plists, icons) — harmless
    for line in result.stderr.splitlines() + result.stdout.splitlines():
        if "ERROR" in line and "is not an object file" not in line:
            print(f"  macdeployqt: {line.strip()}")

    # Clean up unwanted plugins
    unwanted = [
        app_bundle / "Contents/PlugIns/sqldrivers/libqsqlpsql.dylib",
        app_bundle / "Contents/PlugIns/sqldrivers/libqsqlodbc.dylib",
    ]
    for path in unwanted:
        if path.exists():
            print(f"  Removing {path.name}")
            path.unlink()


def copy_translations(macdeployqt, resources_dir):
    """Copy Qt translations into the bundle."""
    print("=== Copying translations ===")
    qt_translations = macdeployqt.parent.parent / "translations"
    translations_dir = resources_dir / "Translations"
    translations_dir.mkdir(exist_ok=True)

    if qt_translations.is_dir():
        for qm in sorted(qt_translations.glob("*.qm")):
            shutil.copy2(qm, translations_dir)
        print(f"  Qt: {len(list(qt_translations.glob('*.qm')))} translation files")
    else:
        print("  WARNING: Qt translations not found")

    # TODO: qtkeychain translations are not included in the Conan package.
    # Fix the qtkeychain recipe to install them, then copy here.


def write_qt_conf(resources_dir):
    print("=== Writing qt.conf ===")
    qt_conf = resources_dir / "qt.conf"
    qt_conf.write_text(
        "[Paths]\n"
        "Prefix=..\n"
        "Binaries=MacOS\n"
        "Libraries=Frameworks\n"
        "Plugins=PlugIns\n"
        "Qml2Imports=Resources/qml\n"
        "Translations=Resources/Translations\n"
    )


def create_pkg(build_dir, app_bundle, output_dir):
    print("=== Creating .pkg installer ===")
    create_mac = build_dir / "admin" / "osx" / "create_mac.sh"
    if not create_mac.exists():
        print(f"Warning: create_mac.sh not found at {create_mac}")
        print("Run create_mac.sh manually after installing packagesbuild.")
        return False

    # create_mac.sh expects the .app to be directly inside the install path
    app_parent = app_bundle.parent
    create_mac.chmod(0o755)
    result = subprocess.run(
        [str(create_mac), str(app_parent), str(build_dir)],
        capture_output=True, text=True
    )
    print(result.stdout.strip())
    if result.returncode != 0:
        print(result.stderr.strip())
        return False

    # packagesbuild writes the .pkg to BUILD_PATH from the .pkgproj, which is
    # CMAKE_INSTALL_PREFIX. Parse it from the CMake cache.
    pkg_search_dirs = [app_parent]
    cmake_cache = build_dir / "CMakeCache.txt"
    if cmake_cache.exists():
        for line in cmake_cache.read_text().splitlines():
            if line.startswith("CMAKE_INSTALL_PREFIX:PATH="):
                prefix = Path(line.split("=", 1)[1])
                if prefix.is_dir():
                    pkg_search_dirs.append(prefix)

    for search_dir in pkg_search_dirs:
        for pkg in search_dir.glob("*.pkg"):
            dest = output_dir / pkg.name
            shutil.move(pkg, dest)
            print(f"  Package: {dest}")
            return True

    print("  WARNING: .pkg file not found after build")
    return False


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--build-dir", required=True, type=Path,
        help="Conan build output (e.g. client/build/RelWithDebInfo)",
    )
    parser.add_argument(
        "--output-dir", required=True, type=Path,
        help="Where to write the .pkg file",
    )
    args = parser.parse_args()

    build_dir = args.build_dir.resolve()
    output_dir = args.output_dir.resolve()
    output_dir.mkdir(parents=True, exist_ok=True)

    if not build_dir.is_dir():
        sys.exit(f"Error: build directory not found: {build_dir}")

    macdeployqt, env = find_macdeployqt(build_dir)
    if not macdeployqt:
        sys.exit("Error: macdeployqt not found via Conan build environment")
    print(f"Using macdeployqt: {macdeployqt}")
    os.environ.update(env)

    # 1. cmake --install
    print("=== Installing to staging directory ===")
    install_dir = Path(tempfile.mkdtemp()) / "install"
    run(["cmake", "--install", str(build_dir), "--prefix", str(install_dir)])

    app_bundles = list(install_dir.rglob("*.app"))
    if not app_bundles:
        sys.exit(f"Error: no .app bundle found in {install_dir}")
    app_bundle = app_bundles[0]
    print(f"Found: {app_bundle}")

    contents = app_bundle / "Contents"
    frameworks = contents / "Frameworks"
    resources = contents / "Resources"
    macos = contents / "MacOS"
    frameworks.mkdir(exist_ok=True)
    resources.mkdir(exist_ok=True)

    executables = [p for p in macos.iterdir() if p.is_file() and os.access(p, os.X_OK)]
    if not executables:
        sys.exit("Error: no executable found in Contents/MacOS")
    executable = executables[0]
    print(f"Main executable: {executable}")

    # 2-3. Copy libs and QML
    copy_client_libs(install_dir, frameworks)
    copy_qml_modules(install_dir, resources)

    # 4. Resolve Conan deps
    resolve_conan_deps(executable, frameworks)

    # 5. Fix install names
    fix_all_install_names(executable, frameworks, resources / "qml")

    # 6. macdeployqt
    run_macdeployqt(macdeployqt, app_bundle, frameworks, resources / "qml")

    # 7. Translations
    copy_translations(macdeployqt, resources)

    # 8. qt.conf
    write_qt_conf(resources)

    # 9. .pkg
    if not create_pkg(build_dir, app_bundle, output_dir):
        print(f"The .app bundle is ready at: {app_bundle}")

    print("=== Done ===")


if __name__ == "__main__":
    main()
