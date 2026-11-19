"""Conan dependencies of the ownCloud desktop client.

This file, conan.lock and conan/profiles/ are the single source of truth for
the client's third-party dependencies on this branch. Recipes come from
https://github.com/owncloud/conan-center-index, at the commit pinned in
conan/recipes-commit.

conan.lock covers all four platforms in conan/profiles/, and macos-x86_64
cross-built on an arm64 Mac. It is only ever changed with
conan/update-lock.sh; run it after any change to the requirements or options
below.

The module level must stay free of dynamic code: CI reads the Conan version
pin below statically, without importing this file.
"""

from conan import ConanFile

# Exact pin of the Conan client version (no whitespace after "=", Conan 2
# rejects "==" and parses "= 2.x.y" as a range).
required_conan_version = "=2.34.0"


class OwncloudClientConan(ConanFile):
    settings = "os", "compiler", "arch", "build_type"
    default_options = {
        "*:fPIC": True,
        # Qt does not use harfbuzz's glib integration, and with it macOS and
        # Windows got a static glib inside Qt's harfbuzz.
        "harfbuzz/*:with_glib": False,
        # One shared OpenSSL for QtNetwork, Qt's TLS plugin and everything else.
        # Static, QtNetwork embedded a copy and exported its symbols, the TLS
        # plugin linked a second one, and on macOS x86_64 the plugin did not
        # link at all ("fixup error ... '_OPENSSL_cpuid_setup' does not have
        # address": libcrypto.a's init section bound to QtNetwork's copy).
        # With a shared OpenSSL the qt recipe would have Qt load it by name at
        # runtime; the profiles' qt/*:...:extra_variables make Qt link it
        # (and qt/*:tools.info.package_id:confs puts that into Qt's package ID).
        "openssl/*:shared": True,
        "qt/*:gui": True,
        "qt/*:qtdeclarative": True,
        "qt/*:qtimageformats": True,
        "qt/*:qtquickcontrols2": True,
        "qt/*:qtshadertools": True,
        "qt/*:qtsvg": True,
        "qt/*:qttools": True,
        "qt/*:qttranslations": True,
        "qt/*:shared": True,
        "qt/*:widgets": True,
        "qt/*:with_egl": True,
        "qt/*:with_libjpeg": "libjpeg",
        "qt/*:with_odbc": False,
        "qt/*:with_pq": False,
    }

    def configure(self):
        if self.settings.os == "Linux":
            self.options["qt/*"].with_dbus = True
            self.options["qt/*"].qtwayland = True
            # Qt's event loop on glib (QEventDispatcherGlib), as in the Craft
            # build: qtkeychain's libsecret backend uses libsecret's async API,
            # whose callbacks only run in a glib main loop. Without it every
            # keychain job hangs and credentials are never stored or read.
            self.options["qt/*"].with_glib = True
            # With harfbuzz/*:with_glib off (default_options), Qt and qtkeychain's
            # libsecret are the ones that pull in glib.
            # Shared, so that the process has one glib, the shipped one, and not
            # private static copies inside Qt and libqt6keychain.so next to any
            # other glib that gets loaded. The AppImage has to bundle both libraries.
            self.options["glib/*"].shared = True
            self.options["libsecret/*"].shared = True

    def requirements(self):
        self.requires("extra-cmake-modules/6.8.0")
        self.requires("kdsingleapplication/1.2.0")
        self.requires("libregraphapi/1.0.4")
        self.requires("nlohmann_json/3.11.3")
        # 3.4.8: the 3.4 patch the pinned openssl recipe offers. (The recipes'
        # former root conanfile.py named 3.4.7, which the recipe no longer has.)
        self.requires("openssl/3.4.8")
        self.requires("qt/6.10.3")
        self.requires("qtkeychain/0.15.0")
        self.requires("sqlite3/3.51.3")
        self.requires("zlib/1.3.2")

        if self.settings.os == "Linux":
            # The qt recipe's requirements() pin glib/2.78.3 once with_glib is on
            # (configure() above); without this override the whole graph would go
            # down from 2.86.5. A range, so update-lock.sh 'glib/*' can still bump it.
            self.requires("glib/[>=2.86.5 <3]", override=True)

        if self.settings.os == "Macos":
            self.requires("sparkle/2.7.0")

    def build_requirements(self):
        # The client's own cmake must stay below 4, because extra-cmake-modules
        # pins cmake/[>=3.16 <4]. conan.lock still holds a cmake 4 entry: that one
        # is a tool_requires of other dependencies, resolved separately.
        self.tool_requires("cmake/3.31.12")
