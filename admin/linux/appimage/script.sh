#!/bin/bash -e
source /opt/qt510/bin/qt510-env.sh || true

mkdir /build/qtkeychain_build
ls /build/qtkeychain
cd /build/qtkeychain_build
cmake ../qtkeychain -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
make -j4
make install

mkdir /build/client_build
cd /build/client_build
cmake ../client -DCMAKE_BUILD_TYPE=Release -DNO_SHIBBOLETH=1 -DCMAKE_INSTALL_PREFIX=/usr  -DCMAKE_INSTALL_LIBDIR=lib
make -j4
make DESTDIR=/build/appdir install

cd /build

# version for the AppImage
export VERSION=`grep -ohP -e '(?<=MIRALL_VERSION_STRING ")[^"]*' client_build/version.h`

#hack: copy ocsync to a proper directory so it gets properly loaded
cp /build/appdir/usr/lib/owncloud/* /build/appdir/usr/lib

# We need the svg plugin for some icons. (others plugin are automatically taken)
mkdir -p /build/appdir/usr/plugins/imageformats/
cp $QTDIR/plugins/imageformats/libqsvg.so  /build/appdir/usr/plugins/imageformats/

linuxdeployqt ./appdir/usr/share/applications/*.desktop -appimage -bundle-non-qt-libs


cp ownCloud_*.AppImage results
