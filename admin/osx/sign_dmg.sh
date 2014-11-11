#!/bin/bash

[ "$#" -lt 2 ] && echo "Usage: sign_dmg.sh <dmg> <identity>" && exit

src_dmg="$1"
tmp_dmg="writable_$1"
signed_dmg="signed_$1"
identity="$2"
mount="/Volumes/$(basename "$src_dmg"|sed 's,-\([0-9]\)\(.*\),,')"
test -e "$tmp_dmg" && rm -rf "$tmp_dmg"
hdiutil convert "$src_dmg" -format UDRW -o "$tmp_dmg"
hdiutil attach "$tmp_dmg"
pushd "$mount"

# Qt frameworks are not packaged according to Apple Spec with some files in the
# wrong locations. Because of this the build process doesn't copy all the right
# files when building the app bundle.
#
# codesign requires all frameworks, plugins, etc to be signed within an app
# bundle and won't sign these frameworks if they are not in the correct form. So
# these commands copy the needed files.
#
# Change the paths if using Qt 5 or if Qt 4 is installed elsewhere.
# Remove this section entirely when Qt fix this issue.

qtfwpath="/usr/local/Cellar/qt/4.8.5/lib"
fwarray=(QtCore QtGui QtNetwork QtSql QtWebKit QtXml QtXmlPatterns)

for curfw in "${fwarray[@]}"
do
    appfwrespath="${mount}/owncloud.app/Contents/Frameworks/${curfw}.framework/Resources"
    infoplistpath="${qtfwpath}/${curfw}.framework/Contents/Info.plist"

    if [ ! -f "${appfwrespath}/Info.plist" ]; then
        if [ ! -d "$appfwrespath" ]; then
            mkdir "$appfwrespath"
        fi
        cp "$infoplistpath" "$appfwrespath"
    fi
done

codesign -s "$identity" --deep "$mount"/*.app
bless --openfolder "$mount"
popd
diskutil eject "$mount"
test -e "$signed_dmg" && rm -rf "$signed_dmg"
hdiutil convert "$tmp_dmg" -format UDBZ -o "$signed_dmg"
