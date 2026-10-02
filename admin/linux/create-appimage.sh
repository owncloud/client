#!/bin/bash
set -euo pipefail

usage() {
    echo "Usage: $0 -b BUILD_DIR [-o OUTPUT_DIR] [-v VERSION]"
    echo
    echo "Create an AppImage from an already-built ownCloud client."
    echo
    echo "Options:"
    echo "  -b BUILD_DIR   Path to the CMake build directory (required)"
    echo "  -o OUTPUT_DIR  Where to place the resulting AppImage (default: .)"
    echo "  -v VERSION     Version string for the AppImage filename (auto-detected from VERSION.cmake if omitted)"
    exit 1
}

BUILD_DIR=""
OUTPUT_DIR="$(pwd)"
VERSION=""

while getopts "b:o:v:h" opt; do
    case "$opt" in
        b) BUILD_DIR="$OPTARG" ;;
        o) OUTPUT_DIR="$OPTARG" ;;
        v) VERSION="$OPTARG" ;;
        h) usage ;;
        *) usage ;;
    esac
done

if [ -z "$BUILD_DIR" ]; then
    echo "Error: -b BUILD_DIR is required"
    usage
fi

BUILD_DIR="$(cd "$BUILD_DIR" && pwd)"
OUTPUT_DIR="$(mkdir -p "$OUTPUT_DIR" && cd "$OUTPUT_DIR" && pwd)"
SOURCE_DIR="$(cd "$(dirname "$0")/../.." && pwd)"

# Auto-detect version from VERSION.cmake if not provided
if [ -z "$VERSION" ]; then
    version_file="$SOURCE_DIR/VERSION.cmake"
    if [ ! -f "$version_file" ]; then
        echo "Error: VERSION.cmake not found at $version_file and no -v given"
        exit 1
    fi
    MAJOR=$(grep 'MIRALL_VERSION_MAJOR' "$version_file" | sed 's/.*MIRALL_VERSION_MAJOR[[:space:]]*\([0-9]*\).*/\1/')
    MINOR=$(grep 'MIRALL_VERSION_MINOR' "$version_file" | sed 's/.*MIRALL_VERSION_MINOR[[:space:]]*\([0-9]*\).*/\1/')
    PATCH=$(grep 'MIRALL_VERSION_PATCH' "$version_file" | sed 's/.*MIRALL_VERSION_PATCH[[:space:]]*\([0-9]*\).*/\1/')
    VERSION="${MAJOR}.${MINOR}.${PATCH}"
    echo "Auto-detected version: $VERSION"
fi

# Read APPLICATION_EXECUTABLE from the branding/OEM.cmake or OWNCLOUD.cmake
APP_EXECUTABLE=""
for cmake_file in "$SOURCE_DIR/branding/OEM.cmake" "$SOURCE_DIR/OWNCLOUD.cmake"; do
    if [ -f "$cmake_file" ]; then
        APP_EXECUTABLE=$(grep 'APPLICATION_EXECUTABLE' "$cmake_file" | head -1 | sed 's/.*APPLICATION_EXECUTABLE[[:space:]]*"\([^"]*\)".*/\1/')
        break
    fi
done
if [ -z "$APP_EXECUTABLE" ]; then
    echo "Error: could not determine APPLICATION_EXECUTABLE"
    exit 1
fi
echo "Application executable: $APP_EXECUTABLE"

# Check that linuxdeploy is available
LINUXDEPLOY="${LINUXDEPLOY:-linuxdeploy-x86_64.AppImage}"
if ! command -v "$LINUXDEPLOY" &>/dev/null; then
    if [ -x "$LINUXDEPLOY" ]; then
        : # path is usable as-is
    else
        echo "Error: linuxdeploy not found. Set LINUXDEPLOY env var or put linuxdeploy-x86_64.AppImage on PATH."
        exit 1
    fi
fi

APPDIR="$(mktemp -d)/AppDir"
trap 'rm -rf "$(dirname "$APPDIR")"' EXIT

echo "==> Installing into AppDir: $APPDIR"
cmake --install "$BUILD_DIR" --prefix "$APPDIR/usr"

# Symlink etc into the AppDir root (linuxdeploy expects it there)
if [ -d "$APPDIR/usr/etc" ]; then
    ln -sfn usr/etc "$APPDIR/etc"
fi

# Create the runenv hook (replicates Craft's AppImagePackager runenv)
mkdir -p "$APPDIR/apprun-hooks"
cat > "$APPDIR/apprun-hooks/runenv-hook.sh" << 'HOOK'
XDG_DATA_DIRS="$this_dir/usr/share/:$XDG_DATA_DIRS:/usr/local/share:/usr/share"
export XDG_DATA_DIRS
FONTCONFIG_PATH="$(if [ -d /etc/fonts ]; then echo "/etc/fonts"; else echo "$this_dir/etc/fonts"; fi)"
export FONTCONFIG_PATH
PATH="$this_dir/usr/bin:$this_dir/usr/lib:$PATH"
export PATH
HOOK

# Find the .desktop file
DESKTOP_FILE=$(find "$APPDIR/usr/share/applications" -name "*.desktop" | head -1)
if [ -z "$DESKTOP_FILE" ]; then
    echo "Error: no .desktop file found in $APPDIR/usr/share/applications/"
    exit 1
fi
echo "Using desktop file: $DESKTOP_FILE"

APPIMAGE_NAME="ownCloud-${VERSION}-x86_64.AppImage"

# In Docker, FUSE is typically not available
if [ -f /.dockerenv ] || grep -q docker /proc/1/cgroup 2>/dev/null; then
    export APPIMAGE_EXTRACT_AND_RUN=1
fi

export ARCH="${ARCH:-x86_64}"
export LD_LIBRARY_PATH="$APPDIR/usr/lib:$APPDIR/usr/lib/x86_64-linux-gnu:${LD_LIBRARY_PATH:-}"
export LINUXDEPLOY_OUTPUT_VERSION="$VERSION"
export NO_STRIP=1
if [ -z "${QMAKE:-}" ]; then
    QMAKE=$(command -v qmake6 2>/dev/null || command -v qmake 2>/dev/null || find ~/.conan2 -name qmake6 -path '*/bin/*' 2>/dev/null | head -1 || echo qmake)
fi
export QMAKE

QT_PREFIX="$("$QMAKE" -query QT_INSTALL_PREFIX 2>/dev/null || true)"
if [ -n "$QT_PREFIX" ]; then
    echo "Qt prefix: $QT_PREFIX"
    export QT_PLUGIN_PATH="${QT_PREFIX}/plugins:${QT_PLUGIN_PATH:-}"
    export QML2_IMPORT_PATH="${QT_PREFIX}/qml:${QML2_IMPORT_PATH:-}"
    export LD_LIBRARY_PATH="${QT_PREFIX}/lib:${LD_LIBRARY_PATH}"
    # linuxdeploy-plugin-qt crashes if expected plugin dirs don't exist
    for plugdir in printsupport; do
        mkdir -p "${QT_PREFIX}/plugins/${plugdir}"
    done
fi

echo "==> Running linuxdeploy"
"$LINUXDEPLOY" \
    --appdir "$APPDIR" \
    --desktop-file "$DESKTOP_FILE" \
    --output=appimage \
    --plugin=qt

# linuxdeploy creates the AppImage in the current directory
# Move it to the output directory with the desired name
GENERATED=$(ls -t *.AppImage 2>/dev/null | head -1)
if [ -n "$GENERATED" ] && [ -f "$GENERATED" ]; then
    mv "$GENERATED" "$OUTPUT_DIR/$APPIMAGE_NAME"
    echo "==> AppImage created: $OUTPUT_DIR/$APPIMAGE_NAME"
else
    echo "Error: linuxdeploy did not produce an AppImage"
    exit 1
fi
