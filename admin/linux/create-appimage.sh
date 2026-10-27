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
    echo "  -v VERSION     Version string for the AppImage filename (default: the version the"
    echo "                 client was configured with, from BUILD_DIR/packaging.env)"
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

# Name, version and executable as configured by CMake (see THEME.cmake), so the
# AppImage name matches the branding and the version the client reports.
PACKAGING_ENV="$BUILD_DIR/packaging.env"
if [ ! -f "$PACKAGING_ENV" ]; then
    echo "Error: $PACKAGING_ENV not found; configure the client with a CMake that writes it"
    exit 1
fi
# shellcheck source=/dev/null
. "$PACKAGING_ENV"
if [ -z "$VERSION" ]; then
    VERSION="$VERSION_STRING"
fi
APP_EXECUTABLE="$APPLICATION_EXECUTABLE"
echo "Artifact prefix: $ARTIFACT_PREFIX, version: $VERSION"
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

# Copy Conan-built shared libraries into AppDir so linuxdeploy bundles them
# instead of system libraries from the build container.
CONAN_LIB_PATHS=$(python3 "$SOURCE_DIR/admin/linux/extract-conan-lib-paths.py" \
    "$APPDIR/usr/bin/$APP_EXECUTABLE" || true)
if [ -n "$CONAN_LIB_PATHS" ]; then
    echo "==> Copying Conan libraries into AppDir"
    echo "$CONAN_LIB_PATHS" | tr ':' '\n' | while read -r libdir; do
        [ -d "$libdir" ] || continue
        case "$libdir" in
            */plugins/*)
                pluginsubdir="${libdir##*/plugins/}"
                mkdir -p "$APPDIR/usr/plugins/$pluginsubdir"
                cp -an "$libdir"/*.so* "$APPDIR/usr/plugins/$pluginsubdir/" 2>/dev/null || true
                ;;
            *)
                cp -an "$libdir"/lib*.so* "$APPDIR/usr/lib/" 2>/dev/null || true
                ;;
        esac
    done
fi

# Symlink etc into the AppDir root (linuxdeploy expects it there)
if [ -d "$APPDIR/usr/etc" ]; then
    ln -sfn usr/etc "$APPDIR/etc"
fi

# Create the runenv hook for runtime environment setup
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

APPIMAGE_NAME="${ARTIFACT_PREFIX}-${VERSION}-x86_64.AppImage"

# In Docker, FUSE is typically not available
if [ -f /.dockerenv ] || grep -q docker /proc/1/cgroup 2>/dev/null; then
    export APPIMAGE_EXTRACT_AND_RUN=1
fi

export ARCH="${ARCH:-x86_64}"
export LD_LIBRARY_PATH="$APPDIR/usr/lib:$APPDIR/usr/lib/x86_64-linux-gnu:${LD_LIBRARY_PATH:-}"
export LINUXDEPLOY_OUTPUT_VERSION="$VERSION"
if [ -z "${QMAKE:-}" ]; then
    QMAKE=$(command -v qmake6 2>/dev/null || command -v qmake 2>/dev/null || find ~/.conan2 -name qmake6 -path '*/bin/*' 2>/dev/null | head -1 || echo qmake)
fi
export QMAKE

# Derive the Qt Conan prefix from qmake's location.
QT_CONAN_DIR="$(cd "$(dirname "$QMAKE")/.." && pwd)"
echo "Qt Conan dir: $QT_CONAN_DIR"
export LD_LIBRARY_PATH="${QT_CONAN_DIR}/lib:${LD_LIBRARY_PATH}"
export PATH="${QT_CONAN_DIR}/bin:${QT_CONAN_DIR}/libexec:${PATH}"

# Copy Qt plugins into AppDir.
mkdir -p "$APPDIR/usr/plugins"
if [ -d "${QT_CONAN_DIR}/plugins" ]; then
    cp -a "${QT_CONAN_DIR}/plugins/"* "$APPDIR/usr/plugins/" 2>/dev/null || true
fi

# linuxdeploy-plugin-qt iterates plugin dirs reported by qmake -query and
# crashes if any expected subdirectory doesn't exist. Create all standard
# plugin dirs in both locations.
for plugdir in bearer egldeviceintegrations generic iconengines \
               imageformats platforminputcontexts platforms \
               platformthemes printsupport sqldrivers \
               tls wayland-decoration-client wayland-graphics-integration-client \
               wayland-graphics-integration-server wayland-shell-integration \
               xcbglintegrations; do
    mkdir -p "$APPDIR/usr/plugins/$plugdir"
    mkdir -p "${QT_CONAN_DIR}/plugins/$plugdir"
done

echo "==> Running linuxdeploy"
"$LINUXDEPLOY" \
    --appdir "$APPDIR" \
    --desktop-file "$DESKTOP_FILE" \
    --plugin=qt

echo "==> Cleaning up AppDir"
cd "$APPDIR"

# Dev files: headers, static libs, cmake, pkgconfig, build artifacts
find . -name '*.cmake' -delete
find . -name '*.pc' -delete
find . -name '*.la' -delete
find . -name '*.prl' -delete
find . -name '*.a' -delete
find . -name '*.h' -delete
find . -name '*.hpp' -delete
find . -name '*.cpp' -delete
find . -name '*.qmltypes' -delete
find . -name '*-qmlmodule.version' -delete
rm -rf usr/include usr/lib/cmake usr/lib/metatypes usr/lib/objects-*

# Qt modules not needed at runtime but kept because the Conan Qt recipe
# creates transitive link dependencies that pull them into the binary:
#   Designer, Help, UiTools, QuickTest, ShaderTools, PrintSupport,
#   WaylandCompositor
# TODO: fix Conan Qt recipe to not create these transitive deps, then
# remove these libraries here.
rm -f usr/lib/libQt6QuickControls2FluentWinUI3StyleImpl*
rm -f usr/lib/libQt6QuickControls2ImagineStyleImpl*
rm -f usr/lib/libQt6QuickParticles*
rm -f usr/lib/libQt6QuickVectorImage*

# Qt plugins not needed at runtime
rm -rf usr/plugins/designer usr/plugins/qmllint usr/plugins/qmltooling
rm -f usr/plugins/sqldrivers/libqsqlmysql*

# QML styles and modules not needed on Linux
rm -rf usr/qml/QtQuick/Controls/FluentWinUI3
rm -rf usr/qml/QtQuick/Controls/Imagine
rm -rf usr/qml/QtQuick/NativeStyle
rm -rf usr/qml/QtQuick/Particles
rm -rf usr/qml/QtQuick/VectorImage

# QML dev files, designer support, and test modules
rm -rf usr/qml/Qt/test
find . -path '*/designer/*' -delete
find . -path '*/objects-RelWithDebInfo/*' -delete

# X11 widget/extension libs pulled in from -devel packages.
# Review this list when upgrading Qt — new versions may need additional X11 libs.
rm -f usr/lib/libXt.so* usr/lib/libXaw.so* usr/lib/libXmu.so* usr/lib/libXmuu.so*
rm -f usr/lib/libXpm.so* usr/lib/libfontenc.so* usr/lib/libxkbfile.so*
rm -f usr/lib/libXRes.so* usr/lib/libXss.so* usr/lib/libXv.so* usr/lib/libXxf86vm.so*
rm -f usr/lib/libXinerama.so* usr/lib/libXdamage.so* usr/lib/libXcomposite.so*
rm -f usr/lib/libxcb-composite.so* usr/lib/libxcb-xinerama.so*
rm -f usr/lib/libxcb-res.so* usr/lib/libxcb-present.so* usr/lib/libxcb-ewmh.so*
rm -f usr/lib/libxcb-cursor.so*

# Translations for Qt tools we don't ship
rm -f usr/translations/assistant_* usr/translations/designer_*
rm -f usr/translations/linguist_* usr/translations/qt_help_*
rm -f usr/translations/qtconnectivity_* usr/translations/qtlocation_*
rm -f usr/translations/qtmultimedia_* usr/translations/qtscript_*
rm -f usr/translations/qtserialport_*
rm -f usr/translations/qtdeclarative_* usr/translations/qtquickcontrols_*

# Misc build/doc leftovers
rm -rf usr/share/man usr/share/doc usr/share/gtk-doc usr/share/gdb
rm -rf usr/share/info usr/share/aclocal usr/share/bash-completion
rm -rf usr/lib/gettext usr/share/gettext
rm -rf usr/share/pkgconfig
find . -name 'mkspecs' -type d -exec rm -rf {} + 2>/dev/null || true

# Strip debug symbols from shared libraries and executables
echo "==> Stripping binaries"
find . -type f \( -name '*.so' -o -name '*.so.*' \) -exec strip --strip-debug {} + 2>/dev/null || true
find . -type f -executable -exec sh -c 'file "$1" | grep -q "ELF" && strip --strip-debug "$1"' _ {} \; 2>/dev/null || true

# Remove empty directories
find . -type d -empty -delete 2>/dev/null || true

cd "$OLDPWD"

echo "==> AppDir contents:"
find "$APPDIR" -type f -printf '%10s %p\n' | sort -rn
echo "==> AppDir directory sizes:"
du -sh "$APPDIR"/usr/*
echo "==> AppDir total size:"
du -sh "$APPDIR"

echo "==> Creating AppImage"
"$LINUXDEPLOY" \
    --appdir "$APPDIR" \
    --output=appimage

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
