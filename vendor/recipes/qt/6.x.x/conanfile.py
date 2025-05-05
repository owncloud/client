import configparser
import glob
import os
import platform
import textwrap

from conan import ConanFile
from conan.tools.apple import is_apple_os
from conan.tools.build import cross_building, check_min_cppstd, default_cppstd
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout
from conan.tools.env import VirtualBuildEnv, VirtualRunEnv, Environment
from conan.tools.files import copy, get, replace_in_file, apply_conandata_patches, save, rm, rmdir, export_conandata_patches
from conan.tools.gnu import PkgConfigDeps
from conan.tools.microsoft import msvc_runtime_flag, is_msvc
from conan.tools.scm import Version
from conan.errors import ConanException, ConanInvalidConfiguration

required_conan_version = ">=2.0"

class QtConan(ConanFile):
    _submodules = ["qtsvg", "qtdeclarative", "qttools", "qttranslations", "qtdoc",
                   "qtwayland", "qtquickcontrols2", "qtquicktimeline", "qtquick3d", "qtshadertools", "qt5compat",
                   "qtactiveqt", "qtcharts", "qtdatavis3d", "qtlottie", "qtscxml", "qtvirtualkeyboard",
                   "qt3d", "qtimageformats", "qtnetworkauth", "qtcoap", "qtmqtt", "qtopcua",
                   "qtmultimedia", "qtlocation", "qtsensors", "qtconnectivity", "qtserialbus",
                   "qtserialport", "qtwebsockets", "qtwebchannel", "qtwebengine", "qtwebview",
                   "qtremoteobjects", "qtpositioning", "qtlanguageserver",
                   "qtspeech", "qthttpserver", "qtquick3dphysics", "qtgrpc", "qtquickeffectmaker"]
    _submodules += ["qtgraphs"] # new modules for qt 6.6.0

    _module_statuses = ["essential", "addon", "deprecated", "preview"]

    name = "qt"
    description = "Qt is a cross-platform framework for graphical user interfaces."
    topics = ("framework", "ui")
    url = "https://github.com/conan-io/conan-center-index"
    homepage = "https://www.qt.io"
    license = "LGPL-3.0-only"
    settings = "os", "arch", "compiler", "build_type"

    options = {
        "shared": [True, False],
        "opengl": ["no", "desktop", "dynamic"],
        "with_vulkan": [True, False],
        "openssl": [True, False],
        "with_pcre2": [True, False],
        "with_glib": [True, False],
        "with_doubleconversion": [True, False],
        "with_freetype": [True, False],
        "with_fontconfig": [True, False],
        "with_icu": [True, False],
        "with_harfbuzz": [True, False],
        "with_libjpeg": ["libjpeg", "libjpeg-turbo", False],
        "with_libpng": [True, False],
        "with_sqlite3": [True, False],
        "with_mysql": [True, False],
        "with_pq": [True, False],
        "with_odbc": [True, False],
        "with_zstd": [True, False],
        "with_brotli": [True, False],
        "with_dbus": [True, False],
        "with_libalsa": [True, False],
        "with_openal": [True, False],
        "with_gstreamer": [True, False],
        "with_pulseaudio": [True, False],
        "with_gssapi": [True, False],
        "with_md4c": [True, False],
        "with_x11": [True, False],
        "with_egl": [True, False],

        "gui": [True, False],
        "widgets": [True, False],

        "device": [None, "ANY"],
        "cross_compile": [None, "ANY"],
        "sysroot": [None, "ANY"],
        "multiconfiguration": [True, False],
        "disabled_features": [None, "ANY"],
    }
    options.update({module: [True, False] for module in _submodules})
    options.update({f"{status}_modules": [True, False] for status in _module_statuses})

    # this significantly speeds up windows builds
    no_copy_source = True

    default_options = {
        "shared": False,
        "opengl": "desktop",
        "with_vulkan": False,
        "openssl": True,
        "with_pcre2": True,
        "with_glib": False,
        "with_doubleconversion": True,
        "with_freetype": True,
        "with_fontconfig": True,
        "with_icu": True,
        "with_harfbuzz": True,
        "with_libjpeg": False,
        "with_libpng": True,
        "with_sqlite3": True,
        "with_mysql": False,
        "with_pq": True,
        "with_odbc": True,
        "with_zstd": False,
        "with_brotli": True,
        "with_dbus": False,
        "with_libalsa": False,
        "with_openal": True,
        "with_gstreamer": False,
        "with_pulseaudio": False,
        "with_gssapi": False,
        "with_md4c": True,
        "with_x11": True,
        "with_egl": False,

        "gui": True,
        "widgets": True,

        "device": None,
        "cross_compile": None,
        "sysroot": None,
        "multiconfiguration": False,
        "disabled_features": "",
    }
    # essential_modules, addon_modules, deprecated_modules, preview_modules:
    #    these are only provided for convenience, set to False by default
    default_options.update({f"{status}_modules": False for status in _module_statuses})

    short_paths = True

    _submodules_tree = None

    @property
    def _get_module_tree(self):
        if self._submodules_tree:
            return self._submodules_tree
        config = configparser.ConfigParser()
        config.read(os.path.join(self.recipe_folder, f"qtmodules{self.version}.conf"))
        self._submodules_tree = {}
        assert config.sections(), f"no qtmodules.conf file for version {self.version}"
        for s in config.sections():
            section = str(s)
            assert section.startswith("submodule ")
            assert section.count('"') == 2
            modulename = section[section.find('"') + 1: section.rfind('"')]
            if modulename in ["qtbase", "qtqa", "qtrepotools"]:
                continue
            status = str(config.get(section, "status"))
            if status not in ["obsolete", "ignore", "additionalLibrary"]:
                if status not in self._module_statuses:
                    raise ConanException(f"module {modulename} has status {status} which is not in self._module_statuses {self._module_statuses}")
                assert modulename in self._submodules, f"module {modulename} not in self._submodules"
                self._submodules_tree[modulename] = {"status": status,
                                "path": str(config.get(section, "path")), "depends": []}
                if config.has_option(section, "depends"):
                    self._submodules_tree[modulename]["depends"] = [str(i) for i in config.get(section, "depends").split()]

        return self._submodules_tree

    def export_sources(self):
        export_conandata_patches(self)

    def export(self):
        copy(self, f"qtmodules{self.version}.conf", self.recipe_folder, self.export_folder)

    def config_options(self):
        if self.settings.os not in ["Linux", "FreeBSD"]:
            del self.options.with_icu
            del self.options.with_fontconfig
            self.options.with_glib = False
            del self.options.with_libalsa
            del self.options.with_x11
            del self.options.with_egl

        if self.settings.os == "Windows":
            self.options.opengl = "dynamic"
            del self.options.with_gssapi
        if self.settings.os != "Linux":
            self.options.qtwayland = False

        for submodule in self._submodules:
            if submodule not in self._get_module_tree:
                self.output.debug(f"Qt6: Removing {submodule} option as it is not in the module tree for this version, or is marked as obsolete or ignore")
                self.options.rm_safe(submodule)

    def configure(self):
        if not self.options.gui:
            del self.options.opengl
            del self.options.with_vulkan
            del self.options.with_freetype
            self.options.rm_safe("with_fontconfig")
            del self.options.with_harfbuzz
            del self.options.with_libjpeg
            del self.options.with_libpng
            del self.options.with_md4c
            self.options.rm_safe("with_x11")
            self.options.rm_safe("with_egl")

        if self.options.multiconfiguration:
            del self.settings.build_type

        # Requested modules:
        # - any module for non-removed options that have 'True' value
        # - any enabled via `xxx_modules` that does not have a 'False' value
        # Note that at this point, the submodule options dont have a value unless one is given externally
        # to the recipe (e.g. via the command line, a profile, or a consumer)
        requested_modules = set([module for module in self._submodules if self.options.get_safe(module)])
        for module in [m for m in self._submodules if m in self._get_module_tree]:
            status = self._get_module_tree[module]['status']
            is_disabled = self.options.get_safe(module) == False
            if self.options.get_safe(f"{status}_modules"):
                if not is_disabled:
                    requested_modules.add(module)
                else:
                    self.output.warning(f"qt6: {module} requested because {status}_modules=True"
                                        f" but it has been explicitly disabled with {module}=False")

        self.output.debug(f"qt6: requested modules {list(requested_modules)}")

        required_modules =  {}
        for module in requested_modules:
            deps = self._get_module_tree[module]["depends"]
            for dep in deps:
                required_modules.setdefault(dep,[]).append(module)

        required_but_disabled = [m for m in required_modules.keys() if self.options.get_safe(m) == False]
        if required_modules:
            self.output.debug(f"qt6: required_modules modules {list(required_modules.keys())}")
        if required_but_disabled:
            required_by = set()
            for m in required_but_disabled:
                required_by.update(required_modules[m])
            raise ConanInvalidConfiguration(f"Modules {required_but_disabled} are explicitly disabled, "
                                            f"but are required by {list(required_by)}, enabled by other options")

        enabled_modules = requested_modules.union(set(required_modules.keys()))
        enabled_modules.discard("qtbase")

        for module in list(enabled_modules):
            setattr(self.options, module, True)

        for module in self._submodules:
            if module in self.options and not self.options.get_safe(module):
                setattr(self.options, module, False)

        if not self.options.get_safe("qtmultimedia"):
            self.options.rm_safe("with_libalsa")
            del self.options.with_openal
            del self.options.with_gstreamer
            del self.options.with_pulseaudio

        if self.settings.os in ("FreeBSD", "Linux"):
            if self.options.get_safe("qtwebengine"):
                self.options.with_fontconfig = True

        for status in self._module_statuses:
            # These are convenience only, should not affect package_id
            option_name = f"{status}_modules"
            self.output.debug(f"qt6 removing convenience option: {option_name},"
                              f" see individual module options")
            self.options.rm_safe(option_name)

        for option in self.options.items():
            self.output.debug(f"qt6 option: {option}")

    def validate_build(self):
        check_min_cppstd(self, 17)

    def validate(self):
        skip_ci_reason = self.conf.get("user.conancenter:skip_ci_build", check_type=str)
        if skip_ci_reason:
            # Currently failing on CI for gcc11, see conan-io/conan-center-index#13472
            raise ConanInvalidConfiguration(skip_ci_reason)

        if self.settings_target is None:
            # Raise when consumed in the host context, but allow comaptible when
            # in the build context
            check_min_cppstd(self, 17)

        if self.settings.compiler == "apple-clang" and Version(self.settings.compiler.version) < "12":
            raise ConanInvalidConfiguration("apple-clang >= 12 required by qt >= 6.4.0")

        if Version(self.version) >= "6.6.1" and self.settings.compiler == "apple-clang" and Version(self.settings.compiler.version) < "13":
            # note: assuming that by now, any xcode 13 is updated to the latest 13.4.1
            raise ConanInvalidConfiguration("apple-clang >= 13.1 is required by qt >= 6.6.1 cf QTBUG-119490")

        if self.options.get_safe("qtwebengine"):
            if not self.options.shared:
                raise ConanInvalidConfiguration("Static builds of Qt WebEngine are not supported")

            if not (self.options.gui and self.options.qtdeclarative and self.options.qtwebchannel):
                raise ConanInvalidConfiguration("option qt:qtwebengine requires also qt:gui, qt:qtdeclarative and qt:qtwebchannel")
            if not self.options.with_dbus and self.settings.os == "Linux":
                raise ConanInvalidConfiguration("option qt:webengine requires also qt:with_dbus on Linux")

            if hasattr(self, "settings_build") and cross_building(self, skip_x64_x86=True):
                raise ConanInvalidConfiguration("Cross compiling Qt WebEngine is not supported")

        if self.options.widgets and not self.options.gui:
            raise ConanInvalidConfiguration("using option qt:widgets without option qt:gui is not possible. "
                                            "You can either disable qt:widgets or enable qt:gui")
        if self.settings.os == "Android" and self.options.get_safe("opengl", "no") == "desktop":
            raise ConanInvalidConfiguration("OpenGL desktop is not supported on Android.")

        if self.settings.os != "Windows" and self.options.get_safe("opengl", "no") == "dynamic":
            raise ConanInvalidConfiguration("Dynamic OpenGL is supported only on Windows.")

        if self.options.get_safe("with_fontconfig", False) and not self.options.get_safe("with_freetype", False):
            raise ConanInvalidConfiguration("with_fontconfig cannot be enabled if with_freetype is disabled.")

        if "MT" in self.settings.get_safe("compiler.runtime", default="") and self.options.shared:
            raise ConanInvalidConfiguration("Qt cannot be built as shared library with static runtime")

        if self.options.get_safe("with_pulseaudio", False) or self.options.get_safe("with_libalsa", False):
            raise ConanInvalidConfiguration("alsa and pulseaudio are not supported (QTBUG-95116), please disable them.")
        if not self.options.with_pcre2:
            raise ConanInvalidConfiguration("pcre2 is actually required by qt (QTBUG-92454). please use option qt:with_pcre2=True")

        if self.settings.os in ['Linux', 'FreeBSD'] and self.options.with_gssapi:
            raise ConanInvalidConfiguration("gssapi cannot be enabled until conan-io/conan-center-index#4102 is closed")

        if self.options.get_safe("with_x11", False) and not self.dependencies.direct_host["xkbcommon"].options.with_x11:
            raise ConanInvalidConfiguration("The 'with_x11' option for the 'xkbcommon' package must be enabled when the 'with_x11' option is enabled")
        if self.options.get_safe("qtwayland", False) and not self.dependencies.direct_host["xkbcommon"].options.with_wayland:
            raise ConanInvalidConfiguration("The 'with_wayland' option for the 'xkbcommon' package must be enabled when the 'qtwayland' option is enabled")

        if self.options.with_sqlite3 and not self.dependencies["sqlite3"].options.enable_column_metadata:
            raise ConanInvalidConfiguration("sqlite3 option enable_column_metadata must be enabled for qt")

        if self.options.get_safe("qtspeech") and not self.options.qtdeclarative:
            raise ConanInvalidConfiguration("qtspeech requires qtdeclarative, cf QTBUG-108381")

    def layout(self):
        cmake_layout(self, src_folder="src")

    def requirements(self):
        self.requires("zlib/[>=1.2.11 <2]")
        if self.options.openssl:
            self.requires("openssl/[>=1.1 <4]")
        if self.options.with_pcre2:
            self.requires("pcre2/10.42")
        if self.options.get_safe("with_vulkan"):
            # Note: the versions of vulkan-loader and moltenvk
            #       must be exactly part of the same Vulkan SDK version
            #       do not update either without checking both
            #       require exactly the same version of vulkan-headers
            self.requires("vulkan-loader/1.3.239.0")
            self.requires("vulkan-headers/1.3.239.0", transitive_headers=True)
            if is_apple_os(self):
                self.requires("moltenvk/1.2.2")
        if self.options.with_glib:
            self.requires("glib/2.78.3")
        if self.options.with_doubleconversion and not self.options.multiconfiguration:
            self.requires("double-conversion/3.3.0")
        if self.options.get_safe("with_freetype", False) and not self.options.multiconfiguration:
            self.requires("freetype/2.13.2")
        if self.options.get_safe("with_fontconfig", False):
            self.requires("fontconfig/2.15.0")
        if self.options.get_safe("with_icu", False):
            self.requires("icu/74.2")
        if self.options.get_safe("with_harfbuzz", False) and not self.options.multiconfiguration:
            self.requires("harfbuzz/8.3.0")
        if self.options.get_safe("with_libjpeg", False) and not self.options.multiconfiguration:
            if self.options.with_libjpeg == "libjpeg-turbo":
                self.requires("libjpeg-turbo/[>=3.0 <3.1]")
            else:
                self.requires("libjpeg/9e")
        if self.options.get_safe("with_libpng", False) and not self.options.multiconfiguration:
            self.requires("libpng/[>=1.6 <2]")
        if self.options.with_sqlite3 and not self.options.multiconfiguration:
            self.requires("sqlite3/[>=3.45.0 <4]")
        if self.options.get_safe("with_mysql", False):
            self.requires("libmysqlclient/8.1.0")
        if self.options.with_pq:
            self.requires("libpq/15.4")
        if self.options.with_odbc:
            if self.settings.os != "Windows":
                self.requires("odbc/2.3.11")
        if self.options.get_safe("with_openal", False):
            self.requires("openal-soft/1.22.2")
        if self.options.get_safe("with_libalsa", False):
            self.requires("libalsa/1.2.10")
        if self.options.get_safe("with_x11") or self.options.qtwayland:
            self.requires("xkbcommon/1.5.0")
        if self.options.get_safe("with_x11", False):
            self.requires("xorg/system")
        if self.options.get_safe("with_egl"):
            self.requires("egl/system")
        if self.settings.os != "Windows" and self.options.get_safe("opengl", "no") != "no":
            self.requires("opengl/system")
        if self.options.with_zstd:
            self.requires("zstd/1.5.5")
        if self.options.qtwayland:
            self.requires("wayland/1.22.0")
        if self.options.with_brotli:
            self.requires("brotli/1.1.0")
        if self.options.get_safe("qtwebengine") and self.settings.os == "Linux":
            self.requires("expat/[>=2.6.2 <3]")
            self.requires("opus/1.4")
            self.requires("xorg-proto/2022.2")
            self.requires("libxshmfence/1.3")
            self.requires("nss/3.93")
            self.requires("libdrm/2.4.119")
        if self.options.get_safe("with_gstreamer", False):
            self.requires("gstreamer/1.19.2")
            self.requires("gst-plugins-base/1.19.2")
        if self.options.get_safe("with_pulseaudio", False):
            self.requires("pulseaudio/14.2")
        if self.options.with_dbus:
            self.requires("dbus/1.15.8")
        if self.settings.os in ['Linux', 'FreeBSD'] and self.options.with_gssapi:
            self.requires("krb5/1.18.3") # conan-io/conan-center-index#4102
        if self.options.get_safe("with_md4c", False):
            self.requires("md4c/0.4.8")

    def build_requirements(self):
        self.tool_requires("cmake/[>=3.21.1 <4]")
        self.tool_requires("ninja/[>=1.12 <2]")
        if not self.conf.get("tools.gnu:pkg_config", check_type=str):
            self.tool_requires("pkgconf/[>=2.2 <3]")

        if self.options.get_safe("qtwebengine"):
            self.tool_requires("nodejs/18.15.0")
            self.tool_requires("gperf/3.1")
            # gperf, bison, flex, python >= 2.7.5 & < 3
            if self.settings_build.os == "Windows":
                self.tool_requires("winflexbison/2.5.25")
            else:
                self.tool_requires("bison/3.8.2")
                self.tool_requires("flex/2.6.4")

        if self.options.qtwayland:
            self.tool_requires("wayland/1.22.0")
        if cross_building(self):
            self.tool_requires(f"qt/{self.version}")

    def generate(self):
        ms = VirtualBuildEnv(self)
        ms.generate()

        tc = CMakeDeps(self)
        tc.set_property("libdrm", "cmake_file_name", "Libdrm")
        tc.set_property("libdrm::libdrm_libdrm", "cmake_target_name", "Libdrm::Libdrm")
        tc.set_property("wayland", "cmake_file_name", "Wayland")
        tc.set_property("wayland::wayland-client", "cmake_target_name", "Wayland::Client")
        tc.set_property("wayland::wayland-server", "cmake_target_name", "Wayland::Server")
        tc.set_property("wayland::wayland-cursor", "cmake_target_name", "Wayland::Cursor")
        tc.set_property("wayland::wayland-egl", "cmake_target_name", "Wayland::Egl")

        # override https://github.com/qt/qtbase/blob/dev/cmake/3rdparty/extra-cmake-modules/find-modules/FindEGL.cmake
        tc.set_property("egl", "cmake_file_name", "EGL")
        tc.set_property("egl", "cmake_find_mode", "module")
        tc.set_property("egl::egl", "cmake_target_name", "EGL::EGL")

        # don't override https://github.com/qt/qtmultimedia/blob/dev/cmake/FindGStreamer.cmake
        tc.set_property("gstreamer", "cmake_file_name", "gstreamer_conan")
        tc.set_property("gstreamer", "cmake_find_mode", "module")

        tc.generate()

        for f in glob.glob("*.cmake"):
            replace_in_file(self, f,
                " IMPORTED)\n",
                " IMPORTED GLOBAL)\n", strict=False)

        pc = PkgConfigDeps(self)
        pc.generate()

        vbe = VirtualBuildEnv(self)
        vbe.generate()
        if not cross_building(self):
            vre = VirtualRunEnv(self)
            vre.generate(scope="build")
        # TODO: to remove when properly handled by conan (see https://github.com/conan-io/conan/issues/11962)
        env = Environment()
        env.unset("VCPKG_ROOT")
        env.prepend_path("PKG_CONFIG_PATH", self.generators_folder)
        env.vars(self).save_script("conanbuildenv_pkg_config_path")
        if self.settings_build.os == "Macos":
            # On macOS, SIP resets DYLD_LIBRARY_PATH injected by VirtualBuildEnv & VirtualRunEnv
            dyld_library_path = "$DYLD_LIBRARY_PATH"
            dyld_library_path_build = vbe.vars().get("DYLD_LIBRARY_PATH")
            if dyld_library_path_build:
                dyld_library_path = f"{dyld_library_path_build}:{dyld_library_path}"
            if not cross_building(self):
                dyld_library_path_host = vre.vars().get("DYLD_LIBRARY_PATH")
                if dyld_library_path_host:
                    dyld_library_path = f"{dyld_library_path_host}:{dyld_library_path}"
            save(self, "bash_env", f'export DYLD_LIBRARY_PATH="{dyld_library_path}"')
            env.define_path("BASH_ENV", os.path.abspath("bash_env"))

        tc = CMakeToolchain(self, generator="Ninja")

        tc.absolute_paths = True

        tc.variables["QT_BUILD_TESTS"] = "OFF"
        tc.variables["QT_BUILD_EXAMPLES"] = "OFF"

        if is_msvc(self) and "MT" in msvc_runtime_flag(self):
            tc.variables["FEATURE_static_runtime"] = "ON"

        if self.options.multiconfiguration:
            tc.variables["CMAKE_CONFIGURATION_TYPES"] = "Release;Debug"
        tc.variables["FEATURE_optimize_size"] = ("ON" if self.settings.build_type == "MinSizeRel" else "OFF")

        for module in self._get_module_tree:
            tc.variables[f"BUILD_{module}"] = ("ON" if getattr(self.options, module) else "OFF")
        tc.variables["BUILD_qtqa"] = "OFF"
        tc.variables["BUILD_qtrepotools"] = "OFF"

        tc.variables["FEATURE_system_zlib"] = "ON"

        tc.variables["INPUT_opengl"] = self.options.get_safe("opengl", "no")

        # openSSL
        if not self.options.openssl:
            tc.variables["INPUT_openssl"] = "no"
        else:
            tc.variables["HAVE_openssl"] = "ON"
            if self.dependencies["openssl"].options.shared:
                tc.variables["INPUT_openssl"] = "runtime"
                tc.variables["QT_FEATURE_openssl_runtime"] = "ON"
            else:
                tc.variables["INPUT_openssl"] = "linked"
                tc.variables["QT_FEATURE_openssl_linked"] = "ON"

        # TODO: Remove after fixing https://github.com/conan-io/conan/issues/12012
        # Required for qt_config_compile_test() calls against CMakeDeps targets to work correctly.
        tc.cache_variables["CMAKE_TRY_COMPILE_CONFIGURATION"] = str(self.settings.build_type)

        if self.options.with_dbus:
            tc.variables["INPUT_dbus"] = "linked"
        else:
            tc.variables["FEATURE_dbus"] = "OFF"
        tc.variables["CMAKE_FIND_DEBUG_MODE"] = "FALSE"

        if not self.options.with_zstd:
            tc.variables["CMAKE_DISABLE_FIND_PACKAGE_WrapZSTD"] = "ON"

        if not self.options.get_safe("with_vulkan"):
            tc.variables["CMAKE_DISABLE_FIND_PACKAGE_WrapVulkanHeaders"] = "ON"

        # Prevent finding LibClang from the system
        # this is needed by the QDoc tool inside Qt Tools
        # See: https://github.com/conan-io/conan-center-index/issues/24729#issuecomment-2255291495
        tc.variables["CMAKE_DISABLE_FIND_PACKAGE_WrapLibClang"] = "ON"

        for opt, conf_arg in [("with_glib", "glib"),
                              ("with_icu", "icu"),
                              ("with_fontconfig", "fontconfig"),
                              ("with_mysql", "sql_mysql"),
                              ("with_pq", "sql_psql"),
                              ("with_odbc", "sql_odbc"),
                              ("gui", "gui"),
                              ("widgets", "widgets"),
                              ("with_zstd", "zstd"),
                              ("with_vulkan", "vulkan"),
                              ("with_brotli", "brotli"),
                              ("with_gssapi", "gssapi"),
                              ("with_egl", "egl"),
                              ("with_gstreamer", "gstreamer")]:
            tc.variables[f"FEATURE_{conf_arg}"] = ("ON" if self.options.get_safe(opt, False) else "OFF")


        for opt, conf_arg in [
                              ("with_doubleconversion", "doubleconversion"),
                              ("with_freetype", "freetype"),
                              ("with_harfbuzz", "harfbuzz"),
                              ("with_libjpeg", "jpeg"),
                              ("with_libpng", "png"),
                              ("with_sqlite3", "sqlite"),
                              ("with_pcre2", "pcre2"),]:
            if self.options.get_safe(opt, False):
                if self.options.multiconfiguration:
                    tc.variables[f"FEATURE_{conf_arg}"] = "ON"
                else:
                    tc.variables[f"FEATURE_system_{conf_arg}"] = "ON"
            else:
                tc.variables[f"FEATURE_{conf_arg}"] = "OFF"
                tc.variables[f"FEATURE_system_{conf_arg}"] = "OFF"

        for opt, conf_arg in [
                              ("with_doubleconversion", "doubleconversion"),
                              ("with_freetype", "freetype"),
                              ("with_harfbuzz", "harfbuzz"),
                              ("with_libjpeg", "libjpeg"),
                              ("with_libpng", "libpng"),
                              ("with_md4c", "libmd4c"),
                              ("with_pcre2", "pcre"),]:
            if self.options.get_safe(opt, False):
                if self.options.multiconfiguration:
                    tc.variables[f"INPUT_{conf_arg}"] = "qt"
                else:
                    tc.variables[f"INPUT_{conf_arg}"] = "system"
            else:
                tc.variables[f"INPUT_{conf_arg}"] = "no"

        for feature in str(self.options.disabled_features).split():
            tc.variables[f"FEATURE_{feature}"] = "OFF"

        if self.settings.os == "Macos":
            tc.variables["FEATURE_framework"] = "OFF"
        elif self.settings.os == "Android":
            tc.variables["CMAKE_ANDROID_NATIVE_API_LEVEL"] = self.settings.os.api_level
            tc.variables["ANDROID_ABI"] = {"armv7": "armeabi-v7a",
                                           "armv8": "arm64-v8a",
                                           "x86": "x86",
                                           "x86_64": "x86_64"}.get(str(self.settings.arch))

        if self.options.sysroot:
            tc.variables["CMAKE_SYSROOT"] = self.options.sysroot

        if self.options.device:
            tc.variables["QT_QMAKE_TARGET_MKSPEC"] = f"devices/{self.options.device}"
        else:
            xplatform_val = self._xplatform()
            if xplatform_val:
                tc.variables["QT_QMAKE_TARGET_MKSPEC"] = xplatform_val
            else:
                self.output.warning(f"host not supported: {self.settings.os} {self.settings.compiler} {self.settings.compiler.version} {self.settings.arch}")
        if self.options.cross_compile:
            tc.variables["QT_QMAKE_DEVICE_OPTIONS"] = f"CROSS_COMPILE={self.options.cross_compile}"
        if cross_building(self):
            # Mainly to locate Qt6HostInfoConfig.cmake
            tc.cache_variables["QT_HOST_PATH"] = self.dependencies.direct_build["qt"].package_folder
            # Stand-in for Qt6CoreTools - which is loaded for the executable targets
            tc.cache_variables["CMAKE_PROJECT_Qt_INCLUDE"] = os.path.join(self.dependencies.direct_build["qt"].package_folder, self._cmake_executables_file)
            # Ensure tools for host are always built
            tc.cache_variables["QT_FORCE_BUILD_TOOLS"] = True

        tc.variables["FEATURE_pkg_config"] = "ON"
        if self.settings.compiler == "gcc" and self.settings.build_type == "Debug" and not self.options.shared:
            tc.variables["BUILD_WITH_PCH"] = "OFF"  # disabling PCH to save disk space

                               #"set(QT_EXTRA_INCLUDEPATHS ${CONAN_INCLUDE_DIRS})\n"
                               #"set(QT_EXTRA_DEFINES ${CONAN_DEFINES})\n"
                               #"set(QT_EXTRA_LIBDIRS ${CONAN_LIB_DIRS})\n"

        current_cpp_std = self.settings.get_safe("compiler.cppstd", default_cppstd(self))
        current_cpp_std = str(current_cpp_std).replace("gnu", "")
        cpp_std_map = {
            11: "FEATURE_cxx11",
            14: "FEATURE_cxx14",
            17: "FEATURE_cxx17",
            20: "FEATURE_cxx20",
            23: "FEATURE_cxx2b"
        }

        for std, feature in cpp_std_map.items():
            tc.variables[feature] = "ON" if int(current_cpp_std) >= std else "OFF"

        tc.variables["QT_USE_VCPKG"] = False
        tc.cache_variables["QT_USE_VCPKG"] = False

        tc.generate()

    def package_id(self):
        del self.info.options.cross_compile
        del self.info.options.sysroot
        if self.info.options.multiconfiguration:
            if self.info.settings.compiler == "Visual Studio":
                if "MD" in self.info.settings.compiler.runtime:
                    self.info.settings.compiler.runtime = "MD/MDd"
                else:
                    self.info.settings.compiler.runtime = "MT/MTd"
            elif self.info.settings.compiler == "msvc":
                self.info.settings.compiler.runtime_type = "Release/Debug"

    def source(self):
        destination = self.source_folder
        if platform.system() == "Windows":
            # Don't use os.path.join, or it removes the \\?\ prefix, which enables long paths
            destination = rf"\\?\{self.source_folder}"
        get(self, **self.conan_data["sources"][self.version],
                  strip_root=True, destination=destination)

        # patching in source method because of no_copy_source attribute
        apply_conandata_patches(self)
        for f in ["renderer", os.path.join("renderer", "core"), os.path.join("renderer", "platform")]:
            replace_in_file(self, os.path.join(self.source_folder, "qtwebengine", "src", "3rdparty", "chromium", "third_party", "blink", f, "BUILD.gn"),
                                  "  if (enable_precompiled_headers) {\n    if (is_win) {",
                                  "  if (enable_precompiled_headers) {\n    if (false) {"
                                  )

        for f in ["FindPostgreSQL.cmake"]:
            file = os.path.join(self.source_folder, "qtbase", "cmake", f)
            if os.path.isfile(file):
                os.remove(file)

        # workaround QTBUG-94356
        replace_in_file(self, os.path.join(self.source_folder, "qtbase", "cmake", "FindWrapSystemZLIB.cmake"), '"-lz"', 'ZLIB::ZLIB')
        replace_in_file(self, os.path.join(self.source_folder, "qtbase", "configure.cmake"),
            "set_property(TARGET ZLIB::ZLIB PROPERTY IMPORTED_GLOBAL TRUE)",
            "")

        replace_in_file(self,
                        os.path.join(self.source_folder, "qtbase", "cmake", "QtAutoDetect.cmake" if Version(self.version) < "6.6.2" else "QtAutoDetectHelpers.cmake"),
                        "qt_auto_detect_vcpkg()",
                        "# qt_auto_detect_vcpkg()")

        # Handle locating moltenvk headers when vulkan is enabled on macOS
        replace_in_file(self, os.path.join(self.source_folder, "qtbase", "cmake", "FindWrapVulkanHeaders.cmake"),
        "if(APPLE)", "if(APPLE)\n"
                    " find_package(moltenvk REQUIRED QUIET)\n"
                    " target_include_directories(WrapVulkanHeaders::WrapVulkanHeaders INTERFACE ${moltenvk_INCLUDE_DIR})"
        )

    def _xplatform(self):
        if self.settings.os == "Linux":
            if self.settings.compiler == "gcc":
                return {"x86": "linux-g++-32",
                        "armv6": "linux-arm-gnueabi-g++",
                        "armv7": "linux-arm-gnueabi-g++",
                        "armv7hf": "linux-arm-gnueabi-g++",
                        "armv8": "linux-aarch64-gnu-g++"}.get(str(self.settings.arch), "linux-g++")
            if self.settings.compiler == "clang":
                if self.settings.arch == "x86":
                    return "linux-clang-libc++-32" if self.settings.compiler.libcxx == "libc++" else "linux-clang-32"
                if self.settings.arch == "x86_64":
                    return "linux-clang-libc++" if self.settings.compiler.libcxx == "libc++" else "linux-clang"

        elif self.settings.os == "Macos":
            return {"clang": "macx-clang",
                    "apple-clang": "macx-clang",
                    "gcc": "macx-g++"}.get(str(self.settings.compiler))

        elif self.settings.os == "iOS":
            if self.settings.compiler == "apple-clang":
                return "macx-ios-clang"

        elif self.settings.os == "watchOS":
            if self.settings.compiler == "apple-clang":
                return "macx-watchos-clang"

        elif self.settings.os == "tvOS":
            if self.settings.compiler == "apple-clang":
                return "macx-tvos-clang"

        elif self.settings.os == "Android":
            if self.settings.compiler == "clang":
                return "android-clang"

        elif self.settings.os == "Windows":
            return {
                "Visual Studio": "win32-msvc",
                "msvc": "win32-msvc",
                "gcc": "win32-g++",
                "clang": "win32-clang-g++",
            }.get(str(self.settings.compiler))

        elif self.settings.os == "WindowsStore":
            if is_msvc(self):
                if self.settings.compiler == "Visual Studio":
                    msvc_version = str(self.settings.compiler.version)
                else:
                    msvc_version = {
                        "190": "14",
                        "191": "15",
                        "192": "16",
                    }.get(str(self.settings.compiler.version))
                return {
                    "14": {
                        "armv7": "winrt-arm-msvc2015",
                        "x86": "winrt-x86-msvc2015",
                        "x86_64": "winrt-x64-msvc2015",
                    },
                    "15": {
                        "armv7": "winrt-arm-msvc2017",
                        "x86": "winrt-x86-msvc2017",
                        "x86_64": "winrt-x64-msvc2017",
                    },
                    "16": {
                        "armv7": "winrt-arm-msvc2019",
                        "x86": "winrt-x86-msvc2019",
                        "x86_64": "winrt-x64-msvc2019",
                    }
                }.get(msvc_version).get(str(self.settings.arch))

        elif self.settings.os == "FreeBSD":
            return {"clang": "freebsd-clang",
                    "gcc": "freebsd-g++"}.get(str(self.settings.compiler))

        elif self.settings.os == "SunOS":
            if self.settings.compiler == "sun-cc":
                if self.settings.arch == "sparc":
                    return "solaris-cc-stlport" if self.settings.compiler.libcxx == "libstlport" else "solaris-cc"
                if self.settings.arch == "sparcv9":
                    return "solaris-cc64-stlport" if self.settings.compiler.libcxx == "libstlport" else "solaris-cc64"
            elif self.settings.compiler == "gcc":
                return {"sparc": "solaris-g++",
                        "sparcv9": "solaris-g++-64"}.get(str(self.settings.arch))
        elif self.settings.os == "Neutrino" and self.settings.compiler == "qcc":
            return {"armv8": "qnx-aarch64le-qcc",
                    "armv8.3": "qnx-aarch64le-qcc",
                    "armv7": "qnx-armle-v7-qcc",
                    "armv7hf": "qnx-armle-v7-qcc",
                    "armv7s": "qnx-armle-v7-qcc",
                    "armv7k": "qnx-armle-v7-qcc",
                    "x86": "qnx-x86-qcc",
                    "x86_64": "qnx-x86-64-qcc"}.get(str(self.settings.arch))
        elif self.settings.os == "Emscripten" and self.settings.arch == "wasm":
            return "wasm-emscripten"

        return None

    def build(self):
        if self.settings.os == "Macos":
            save(self, ".qmake.stash", "")
            save(self, ".qmake.super", "")
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    @property
    def _cmake_executables_file(self):
        return os.path.join("lib", "cmake", "Qt6Core", "conan_qt_executables_variables.cmake")

    @property
    def _cmake_entry_point_file(self):
        return os.path.join("lib", "cmake", "Qt6Core", "conan_qt_entry_point.cmake")

    def _cmake_qt6_private_file(self, module):
        return os.path.join("lib", "cmake", f"Qt6{module}", f"conan_qt_qt6_{module.lower()}private.cmake")

    def package(self):
        if self.settings.os == "Macos":
            save(self, ".qmake.stash", "")
            save(self, ".qmake.super", "")
        cmake = CMake(self)
        cmake.install()
        copy(self, "*LICENSE*", self.source_folder, os.path.join(self.package_folder, "licenses"),
             excludes="qtbase/examples/*")
        for module in self._get_module_tree:
            if not getattr(self.options, module):
                rmdir(self, os.path.join(self.package_folder, "licenses", module))
        rmdir(self, os.path.join(self.package_folder, "lib", "pkgconfig"))
        for mask in ["Find*.cmake", "*Config.cmake", "*-config.cmake"]:
            rm(self, mask, self.package_folder, recursive=True, excludes="Qt6HostInfoConfig.cmake")
        rm(self, "*.la*", os.path.join(self.package_folder, "lib"), recursive=True)
        rm(self, "*.pdb*", self.package_folder, recursive=True)
        rm(self, "ensure_pro_file.cmake", self.package_folder, recursive=True)
        os.remove(os.path.join(self.package_folder, "libexec" if self.settings.os != "Windows" else "bin", "qt-cmake-private-install.cmake"))

        for m in os.listdir(os.path.join(self.package_folder, "lib", "cmake")):
            if os.path.isfile(os.path.join(self.package_folder, "lib", "cmake", m, f"{m}Macros.cmake")):
                continue
            if glob.glob(os.path.join(self.package_folder, "lib", "cmake", m, "QtPublic*Helpers.cmake")):
                continue
            if m.endswith("Tools"):
                if os.path.isfile(os.path.join(self.package_folder, "lib", "cmake", m, f"{m[:-5]}Macros.cmake")):
                    continue

            if m != "Qt6HostInfo":
                rmdir(self, os.path.join(self.package_folder, "lib", "cmake", m))

        extension = ""
        if self.settings.os == "Windows":
            extension = ".exe"
        filecontents = "set(QT_CMAKE_EXPORT_NAMESPACE Qt6)\n"
        ver = Version(self.version)
        filecontents += f"set(QT_VERSION_MAJOR {ver.major})\n"
        filecontents += f"set(QT_VERSION_MINOR {ver.minor})\n"
        filecontents += f"set(QT_VERSION_PATCH {ver.patch})\n"
        if self.settings.os == "Macos":
            filecontents += 'set(__qt_internal_cmake_apple_support_files_path "${CMAKE_CURRENT_LIST_DIR}/../../../lib/cmake/Qt6/macos")\n'
        targets = ["moc", "qlalr", "rcc", "tracegen", "cmake_automoc_parser", "qmake", "qtpaths", "syncqt", "tracepointgen"]
        if self.options.with_dbus:
            targets.extend(["qdbuscpp2xml", "qdbusxml2cpp"])
        if self.options.gui:
            targets.append("qvkgen")
        if self.options.widgets:
            targets.append("uic")
        if self.settings_build.os == "Macos" and self.settings.os != "iOS":
            targets.extend(["macdeployqt"])
        if self.settings.os == "Windows":
            targets.extend(["windeployqt"])
        if self.options.qttools:
            targets.extend(["qhelpgenerator", "qtattributionsscanner"])
            targets.extend(["lconvert", "lprodump", "lrelease", "lrelease-pro", "lupdate", "lupdate-pro"])
        if self.options.qtshadertools:
            targets.append("qsb")
        if self.options.qtdeclarative:
            targets.extend(["qmltyperegistrar", "qmlcachegen", "qmllint", "qmlimportscanner"])
            targets.extend(["qmlformat", "qml", "qmlprofiler", "qmlpreview"])
            # Note: consider "qmltestrunner", see https://github.com/conan-io/conan-center-index/issues/24276
        if self.options.get_safe("qtremoteobjects"):
            targets.append("repc")
        if self.options.get_safe("qtscxml"):
            targets.append("qscxmlc")
        for target in targets:
            exe_path = None
            for path_ in [f"bin/{target}{extension}",
                          f"lib/{target}{extension}",
                          f"libexec/{target}{extension}"]:
                if os.path.isfile(os.path.join(self.package_folder, path_)):
                    exe_path = path_
                    break
            else:
                assert False, f"Could not find executable {target}{extension} in {self.package_folder}"
            if not exe_path:
                self.output.warning(f"Could not find path to {target}{extension}")
            filecontents += textwrap.dedent(f"""\
                if(NOT TARGET ${{QT_CMAKE_EXPORT_NAMESPACE}}::{target})
                    add_executable(${{QT_CMAKE_EXPORT_NAMESPACE}}::{target} IMPORTED)
                    set_target_properties(${{QT_CMAKE_EXPORT_NAMESPACE}}::{target} PROPERTIES IMPORTED_LOCATION ${{CMAKE_CURRENT_LIST_DIR}}/../../../{exe_path})
                endif()
                """)

        filecontents += textwrap.dedent(f"""\
            if(NOT DEFINED QT_DEFAULT_MAJOR_VERSION)
                set(QT_DEFAULT_MAJOR_VERSION {ver.major})
            endif()
            """)
        filecontents += 'set(CMAKE_AUTOMOC_MACRO_NAMES "Q_OBJECT" "Q_GADGET" "Q_GADGET_EXPORT" "Q_NAMESPACE" "Q_NAMESPACE_EXPORT")\n'
        save(self, os.path.join(self.package_folder, self._cmake_executables_file), filecontents)

        def _create_private_module(module, dependencies):
            dependencies_string = ';'.join(f"Qt6::{dependency}" for dependency in dependencies)
            contents = textwrap.dedent(f"""\
            if(NOT TARGET Qt6::{module}Private)
                add_library(Qt6::{module}Private INTERFACE IMPORTED)

                set_target_properties(Qt6::{module}Private PROPERTIES
                    INTERFACE_INCLUDE_DIRECTORIES "${{CMAKE_CURRENT_LIST_DIR}}/../../../include/Qt{module}/{self.version};${{CMAKE_CURRENT_LIST_DIR}}/../../../include/Qt{module}/{self.version}/Qt{module}"
                    INTERFACE_LINK_LIBRARIES "{dependencies_string}"
                )

                add_library(Qt::{module}Private INTERFACE IMPORTED)
                set_target_properties(Qt::{module}Private PROPERTIES
                    INTERFACE_LINK_LIBRARIES "Qt6::{module}Private"
                    _qt_is_versionless_target "TRUE"
                )
            endif()""")

            save(self, os.path.join(self.package_folder, self._cmake_qt6_private_file(module)), contents)

        _create_private_module("Core", ["Core"])

        if self.options.gui:
            _create_private_module("Gui", ["CorePrivate", "Gui"])

        if self.options.widgets:
            _create_private_module("Widgets", ["CorePrivate", "GuiPrivate", "Widgets"])

        if self.options.qtdeclarative:
            _create_private_module("Qml", ["CorePrivate", "Qml"])
            save(self, os.path.join(self.package_folder, "lib", "cmake", "Qt6Qml", "conan_qt_qt6_policies.cmake"), textwrap.dedent("""\
                    set(QT_KNOWN_POLICY_QTP0001 TRUE)
                    """))
            if self.options.gui and self.options.qtshadertools:
                _create_private_module("Quick", ["CorePrivate", "GuiPrivate", "QmlPrivate", "Quick"])

        if self.settings.os in ["Windows", "iOS"]:
            contents = textwrap.dedent("""\
                set(entrypoint_conditions "$<NOT:$<BOOL:$<TARGET_PROPERTY:qt_no_entrypoint>>>")
                list(APPEND entrypoint_conditions "$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>")
                if(WIN32)
                    list(APPEND entrypoint_conditions "$<BOOL:$<TARGET_PROPERTY:WIN32_EXECUTABLE>>")
                endif()
                list(JOIN entrypoint_conditions "," entrypoint_conditions)
                set(entrypoint_conditions "$<AND:${entrypoint_conditions}>")
                set_property(
                    TARGET ${QT_CMAKE_EXPORT_NAMESPACE}::Core
                    APPEND PROPERTY INTERFACE_LINK_LIBRARIES "$<${entrypoint_conditions}:${QT_CMAKE_EXPORT_NAMESPACE}::EntryPointPrivate>"
                )""")
            save(self, os.path.join(self.package_folder, self._cmake_entry_point_file), contents)

    def package_info(self):
        self.cpp_info.set_property("cmake_file_name", "Qt6")
        self.cpp_info.set_property("pkg_config_name", "qt6")

        # consumers will need the QT_PLUGIN_PATH defined in runenv
        self.runenv_info.define("QT_PLUGIN_PATH", os.path.join(self.package_folder, "plugins"))
        self.buildenv_info.define("QT_PLUGIN_PATH", os.path.join(self.package_folder, "plugins"))

        self.buildenv_info.define("QT_HOST_PATH", self.package_folder)

        build_modules = {}
        def _add_build_module(component, module):
            if component not in build_modules:
                build_modules[component] = []
            build_modules[component].append(module)

        libsuffix = ""
        if self.settings.build_type == "Debug":
            if is_msvc(self):
                libsuffix = "d"
            if is_apple_os(self):
                libsuffix = "_debug"

        def _get_corrected_reqs(requires):
            reqs = []
            for r in requires:
                if "::" in r:
                    corrected_req = r
                else:
                    corrected_req = f"qt{r}"
                    assert corrected_req in self.cpp_info.components, f"{corrected_req} required but not yet present in self.cpp_info.components"
                reqs.append(corrected_req)
            return reqs

        def _create_module(module, requires, has_include_dir=True):
            componentname = f"qt{module}"
            assert componentname not in self.cpp_info.components, f"Module {module} already present in self.cpp_info.components"
            self.cpp_info.components[componentname].set_property("cmake_target_name", f"Qt6::{module}")
            self.cpp_info.components[componentname].set_property("pkg_config_name", f"Qt6{module}")
            if module.endswith("Private"):
                libname = module[:-7]
            else:
                libname = module
            self.cpp_info.components[componentname].libs = [f"Qt6{libname}{libsuffix}"]
            if has_include_dir:
                self.cpp_info.components[componentname].includedirs = ["include", os.path.join("include", f"Qt{module}")]
            self.cpp_info.components[componentname].defines = [f"QT_{module.upper()}_LIB"]
            if module != "Core" and "Core" not in requires:
                requires.append("Core")
            self.cpp_info.components[componentname].requires = _get_corrected_reqs(requires)

        def _create_plugin(pluginname, libname, plugintype, requires):
            componentname = f"qt{pluginname}"
            assert componentname not in self.cpp_info.components, f"Plugin {pluginname} already present in self.cpp_info.components"
            self.cpp_info.components[componentname].set_property("cmake_target_name", f"Qt6::{pluginname}")
            if not self.options.shared:
                self.cpp_info.components[componentname].libs = [libname + libsuffix]
            self.cpp_info.components[componentname].libdirs = [os.path.join("plugins", plugintype)]
            self.cpp_info.components[componentname].includedirs = []
            if "Core" not in requires:
                requires.append("Core")
            self.cpp_info.components[componentname].requires = _get_corrected_reqs(requires)

        core_reqs = ["zlib::zlib"]
        if self.options.with_pcre2:
            core_reqs.append("pcre2::pcre2")
        if self.options.with_doubleconversion:
            core_reqs.append("double-conversion::double-conversion")
        if self.options.get_safe("with_icu", False):
            core_reqs.append("icu::icu")
        if self.options.with_zstd:
            core_reqs.append("zstd::zstd")
        if self.options.with_glib:
            core_reqs.append("glib::glib")
        if self.options.openssl:
            core_reqs.append("openssl::openssl") # used by QCryptographicHash

        _create_module("Core", core_reqs)
        pkg_config_vars = [
            "bindir=${prefix}/bin",
            "libexecdir=${prefix}/libexec",
            "exec_prefix=${prefix}",
        ]
        self.cpp_info.components["qtCore"].set_property("pkg_config_custom_content", "\n".join(pkg_config_vars))

        if self.settings.build_type != "Debug":
            self.cpp_info.components['qtCore'].defines.append('QT_NO_DEBUG')
        if self.settings.os == "Windows":
            self.cpp_info.components["qtCore"].system_libs.append("authz")
        if is_msvc(self):
            self.cpp_info.components["qtCore"].cxxflags.append("-permissive-")
            self.cpp_info.components["qtCore"].cxxflags.append("-Zc:__cplusplus")
            self.cpp_info.components["qtCore"].system_libs.append("synchronization")
            self.cpp_info.components["qtCore"].system_libs.append("runtimeobject")
        self.cpp_info.components["qtPlatform"].set_property("cmake_target_name", "Qt6::Platform")
        self.cpp_info.components["qtPlatform"].includedirs = [os.path.join("mkspecs", self._xplatform())]
        if self.options.with_dbus:
            _create_module("DBus", ["dbus::dbus"])
            if self.settings.os == "Windows":
                # https://github.com/qt/qtbase/blob/v6.6.1/src/dbus/CMakeLists.txt#L71-L77
                self.cpp_info.components["qtDBus"].system_libs.append("advapi32")
                self.cpp_info.components["qtDBus"].system_libs.append("netapi32")
                self.cpp_info.components["qtDBus"].system_libs.append("user32")
                self.cpp_info.components["qtDBus"].system_libs.append("ws2_32")
        if self.options.gui:
            gui_reqs = []
            if self.options.with_dbus:
                gui_reqs.append("DBus")
            if self.options.with_freetype:
                gui_reqs.append("freetype::freetype")
            if self.options.with_libpng:
                gui_reqs.append("libpng::libpng")
            if self.options.get_safe("with_fontconfig", False):
                gui_reqs.append("fontconfig::fontconfig")
            if self.settings.os in ["Linux", "FreeBSD"]:
                if self.options.qtwayland or self.options.get_safe("with_x11", False):
                    gui_reqs.append("xkbcommon::xkbcommon")
                if self.options.get_safe("with_x11", False):
                    gui_reqs.append("xorg::xorg")
                if self.options.get_safe("with_egl"):
                    gui_reqs.append("egl::egl")
            if self.settings.os != "Windows" and self.options.get_safe("opengl", "no") != "no":
                gui_reqs.append("opengl::opengl")
            if self.options.get_safe("with_vulkan", False):
                gui_reqs.append("vulkan-loader::vulkan-loader")
                gui_reqs.append("vulkan-headers::vulkan-headers")
                if is_apple_os(self):
                    gui_reqs.append("moltenvk::moltenvk")
            if self.options.with_harfbuzz:
                gui_reqs.append("harfbuzz::harfbuzz")
            if self.options.with_glib:
                gui_reqs.append("glib::glib")
            if self.options.with_md4c:
                gui_reqs.append("md4c::md4c")
            _create_module("Gui", gui_reqs)

            _add_build_module("qtGui", self._cmake_qt6_private_file("Gui"))

            if self.settings.os == "Windows":
                # https://github.com/qt/qtbase/blob/v6.6.1/src/gui/CMakeLists.txt#L419-L429
                self.cpp_info.components["qtGui"].system_libs += [
                    "advapi32", "gdi32", "ole32", "shell32", "user32", "d3d11", "dxgi", "dxguid"
                ]
                # https://github.com/qt/qtbase/blob/v6.6.1/src/gui/CMakeLists.txt#L729
                self.cpp_info.components["qtGui"].system_libs.append("d2d1")
                # https://github.com/qt/qtbase/blob/v6.6.1/src/gui/CMakeLists.txt#L732-L742
                self.cpp_info.components["qtGui"].system_libs.append("dwrite")
                if self.settings.compiler == "gcc":
                    # https://github.com/qt/qtbase/blob/v6.6.1/src/gui/CMakeLists.txt#L746
                    self.cpp_info.components["qtGui"].system_libs.append("uuid")
                if Version(self.version) >= "6.6.0":
                    # https://github.com/qt/qtbase/blob/v6.6.0/src/gui/CMakeLists.txt#L428
                    self.cpp_info.components["qtGui"].system_libs.append("d3d12")
                if Version(self.version) >= "6.7.0":
                    # https://github.com/qt/qtbase/blob/v6.7.0-beta1/src/gui/CMakeLists.txt#L430
                    self.cpp_info.components["qtGui"].system_libs.append("uxtheme")
                if self.settings.compiler == "gcc":
                    self.cpp_info.components["qtGui"].system_libs.append("uuid")
                # https://github.com/qt/qtbase/blob/v6.6.1/src/plugins/platforms/direct2d/CMakeLists.txt#L60-L82
                self.cpp_info.components["qtGui"].system_libs += [
                    "advapi32", "d2d1", "d3d11", "dwmapi", "dwrite", "dxgi", "dxguid", "gdi32", "imm32", "ole32",
                    "oleaut32", "setupapi", "shell32", "shlwapi", "user32", "version", "winmm", "winspool",
                    "wtsapi32", "shcore", "comdlg32", "d3d9", "runtimeobject"
                ]
                _create_plugin("QWindowsIntegrationPlugin", "qwindows", "platforms", ["Core", "Gui"])
                # https://github.com/qt/qtbase/commit/65d58e6c41e3c549c89ea4f05a8e467466e79ca3
                if Version(self.version) >= "6.7.0":
                    _create_plugin("QModernWindowsStylePlugin", "qmodernwindowsstyle", "styles", ["Core", "Gui"])
                else:
                    _create_plugin("QWindowsVistaStylePlugin", "qwindowsvistastyle", "styles", ["Core", "Gui"])
                # https://github.com/qt/qtbase/blob/v6.6.1/src/plugins/platforms/windows/CMakeLists.txt#L53-L69
                self.cpp_info.components["qtQWindowsIntegrationPlugin"].system_libs += [
                    "advapi32", "dwmapi", "gdi32", "imm32", "ole32", "oleaut32", "setupapi", "shell32", "shlwapi",
                    "user32", "winmm", "winspool", "wtsapi32", "shcore", "comdlg32", "d3d9", "runtimeobject"
                ]
            elif self.settings.os == "Android":
                _create_plugin("QAndroidIntegrationPlugin", "qtforandroid", "platforms", ["Core", "Gui"])
                # https://github.com/qt/qtbase/blob/v6.6.1/src/plugins/platforms/android/CMakeLists.txt#L68-L70
                self.cpp_info.components["qtQAndroidIntegrationPlugin"].system_libs = ["android", "jnigraphics"]
            elif is_apple_os(self):
                # https://github.com/qt/qtbase/blob/v6.6.1/src/gui/CMakeLists.txt#L388-L394
                self.cpp_info.components["qtGui"].frameworks = ["CoreFoundation", "CoreGraphics", "CoreText", "Foundation", "ImageIO"]
                if self.options.get_safe("opengl", "no") != "no":
                    # https://github.com/qt/qtbase/commit/2ed63e587eefb246dba9e69aa01fdb2abb2def13
                    self.cpp_info.components["qtGui"].frameworks.append("AGL")
                if self.settings.os == "Macos":
                    # https://github.com/qt/qtbase/blob/v6.6.1/src/gui/CMakeLists.txt#L362-L370
                    self.cpp_info.components["qtGui"].frameworks += ["AppKit", "Carbon"]
                    _create_plugin("QCocoaIntegrationPlugin", "qcocoa", "platforms", ["Core", "Gui"])
                    # https://github.com/qt/qtbase/blob/v6.6.1/src/plugins/platforms/cocoa/CMakeLists.txt#L51-L58
                    self.cpp_info.components["QCocoaIntegrationPlugin"].frameworks = [
                        "AppKit", "Carbon", "CoreServices", "CoreVideo", "IOKit", "IOSurface", "Metal", "QuartzCore"
                    ]
                if self.settings.os in ["Macos", "iOS"]:
                    # https://github.com/qt/qtbase/blob/v6.5.3/src/gui/CMakeLists.txt#L963
                    self.cpp_info.components["qtGui"].frameworks.append("Metal")
                if self.settings.os in ["iOS", "tvOS"]:
                    _create_plugin("QIOSIntegrationPlugin", "qios", "platforms", [])
                    # https://github.com/qt/qtbase/blob/v6.6.1/src/plugins/platforms/ios/CMakeLists.txt#L32-L37
                    self.cpp_info.components["QIOSIntegrationPlugin"].frameworks = [
                        "AudioToolbox", "Foundation", "Metal", "QuartzCore", "UIKit", "CoreGraphics"
                    ]
                    if self.settings.os != "tvOS":
                        # https://github.com/qt/qtbase/blob/v6.6.1/src/plugins/platforms/ios/CMakeLists.txt#L66-L68
                        self.cpp_info.components["QIOSIntegrationPlugin"].frameworks += [
                            "AssetsLibrary", "UniformTypeIdentifiers", "Photos",
                        ]
                elif self.settings.os == "watchOS":
                    _create_plugin("QMinimalIntegrationPlugin", "qminimal", "platforms", [])
            elif self.settings.os == "Emscripten":
                _create_plugin("QWasmIntegrationPlugin", "qwasm", "platforms", ["Core", "Gui"])
            elif self.options.get_safe("with_x11", False):
                _create_module("XcbQpaPrivate", ["xkbcommon::libxkbcommon-x11", "xorg::xorg"], has_include_dir=False)
                _create_plugin("QXcbIntegrationPlugin", "qxcb", "platforms", ["Core", "Gui", "XcbQpaPrivate"])

            _create_plugin("QGifPlugin", "qgif", "imageformats", ["Gui"])
            _create_plugin("QIcoPlugin", "qico", "imageformats", ["Gui"])
            if self.options.get_safe("with_libjpeg"):
                jpeg_reqs = ["Gui"]
                if self.options.with_libjpeg == "libjpeg-turbo":
                    jpeg_reqs.append("libjpeg-turbo::libjpeg-turbo")
                if self.options.with_libjpeg == "libjpeg":
                    jpeg_reqs.append("libjpeg::libjpeg")
                _create_plugin("QJpegPlugin", "qjpeg", "imageformats", jpeg_reqs)

        if self.options.with_sqlite3:
            _create_plugin("QSQLiteDriverPlugin", "qsqlite", "sqldrivers", ["sqlite3::sqlite3"])
        if self.options.with_pq:
            _create_plugin("QPSQLDriverPlugin", "qsqlpsql", "sqldrivers", ["libpq::libpq"])
        if self.options.with_odbc:
            _create_plugin("QODBCDriverPlugin", "qsqlodbc", "sqldrivers", [])
            if self.settings.os != "Windows":
                self.cpp_info.components["QODBCDriverPlugin"].requires.append("odbc::odbc")
            else:
                self.cpp_info.components["QODBCDriverPlugin"].system_libs.append("odbc32")
        networkReqs = []
        if self.options.openssl:
            networkReqs.append("openssl::openssl")
        if self.options.with_brotli:
            networkReqs.append("brotli::brotli")
        if self.settings.os in ['Linux', 'FreeBSD'] and self.options.with_gssapi:
            networkReqs.append("krb5::krb5-gssapi")
        _create_module("Network", networkReqs)
        _create_module("Sql", [])
        _create_module("Test", [])
        if self.options.widgets:
            _create_module("Widgets", ["Gui"])
            _add_build_module("qtWidgets", self._cmake_qt6_private_file("Widgets"))
            if self.settings.os == "Windows":
                # https://github.com/qt/qtbase/blob/v6.6.1/src/widgets/CMakeLists.txt#L316-L321
                self.cpp_info.components["qtWidgets"].system_libs += [
                    "dwmapi", "shell32", "uxtheme",
                ]
        if self.options.gui and self.options.widgets:
            _create_module("PrintSupport", ["Gui", "Widgets"])
        if self.options.get_safe("opengl", "no") != "no" and self.options.gui:
            _create_module("OpenGL", ["Gui"])
        if self.options.widgets and self.options.get_safe("opengl", "no") != "no":
            _create_module("OpenGLWidgets", ["OpenGL", "Widgets"])
        _create_module("Concurrent", [])
        _create_module("Xml", [])

        if self.options.qt5compat:
            _create_module("Core5Compat", [])

        # since https://github.com/qt/qtdeclarative/commit/4fb84137f1c0a49d64b8bef66fef8a4384cc2a68
        qt_quick_enabled = self.options.gui and self.options.qtshadertools

        if self.options.qtdeclarative:
            _create_module("Qml", ["Network"])
            _add_build_module("qtQml", self._cmake_qt6_private_file("Qml"))
            _create_module("QmlModels", ["Qml"])
            self.cpp_info.components["qtQmlImportScanner"].set_property("cmake_target_name", "Qt6::QmlImportScanner")
            self.cpp_info.components["qtQmlImportScanner"].requires = _get_corrected_reqs(["Qml"])
            if qt_quick_enabled:
                _create_module("Quick", ["Gui", "Qml", "QmlModels"])
                _add_build_module("qtQuick", self._cmake_qt6_private_file("Quick"))
                if self.options.widgets:
                    _create_module("QuickWidgets", ["Gui", "Qml", "Quick", "Widgets"])
                _create_module("QuickShapes", ["Gui", "Qml", "Quick"])
                _create_module("QuickTest", ["Test", "Quick"])
            _create_module("QmlWorkerScript", ["Qml"])

        if self.options.qttools and self.options.gui and self.options.widgets:
            self.cpp_info.components["qtLinguistTools"].set_property("cmake_target_name", "Qt6::LinguistTools")
            _create_module("UiPlugin", ["Gui", "Widgets"])
            self.cpp_info.components["qtUiPlugin"].libs = [] # this is a collection of abstract classes, so this is header-only
            self.cpp_info.components["qtUiPlugin"].libdirs = []
            _create_module("UiTools", ["UiPlugin", "Gui", "Widgets"])
            _create_module("Designer", ["Gui", "UiPlugin", "Widgets", "Xml"])
            _create_module("Help", ["Gui", "Sql", "Widgets"])

        if self.options.qtshadertools and self.options.gui:
            _create_module("ShaderTools", ["Gui"])

        if self.options.qtquick3d and qt_quick_enabled:
            _create_module("Quick3DUtils", ["Gui"])
            _create_module("Quick3DAssetImport", ["Gui", "Qml", "Quick3DUtils"])
            _create_module("Quick3DRuntimeRender", ["Gui", "Quick", "Quick3DAssetImport", "Quick3DUtils", "ShaderTools"])
            _create_module("Quick3D", ["Gui", "Qml", "Quick", "Quick3DRuntimeRender"])

        if (self.options.get_safe("qtquickcontrols2") or self.options.qtdeclarative) and qt_quick_enabled:
            _create_module("QuickControls2", ["Gui", "Quick"])
            _create_module("QuickTemplates2", ["Gui", "Quick"])

        if self.options.qtsvg and self.options.gui:
            _create_module("Svg", ["Gui"])
            _create_plugin("QSvgIconPlugin", "qsvgicon", "iconengines", [])
            _create_plugin("QSvgPlugin", "qsvg", "imageformats", [])
            if self.options.widgets:
                _create_module("SvgWidgets", ["Gui", "Svg", "Widgets"])

        if self.options.qtwayland and self.options.gui:
            _create_module("WaylandClient", ["Gui", "wayland::wayland-client"])
            _create_module("WaylandCompositor", ["Gui", "wayland::wayland-server"])

        if self.options.get_safe("qtactiveqt") and self.settings.os == "Windows":
            _create_module("AxBase", ["Gui", "Widgets"])
            _create_module("AxServer", ["AxBase"])
            self.cpp_info.components["qtAxServer"].system_libs.append("shell32")
            self.cpp_info.components["qtAxServer"].defines.append("QAXSERVER")
            _create_module("AxContainer", ["AxBase"])
        if self.options.get_safe("qtcharts"):
            _create_module("Charts", ["Gui", "Widgets"])
        if self.options.get_safe("qtdatavis3d") and qt_quick_enabled:
            _create_module("DataVisualization", ["Gui", "OpenGL", "Qml", "Quick"])
        if self.options.get_safe("qtlottie"):
            _create_module("Bodymovin", ["Gui"])
        if self.options.get_safe("qtscxml"):
            _create_module("StateMachine", [])
            _create_module("StateMachineQml", ["StateMachine", "Qml"])
            _create_module("Scxml", [])
            _create_plugin("QScxmlEcmaScriptDataModelPlugin", "qscxmlecmascriptdatamodel", "scxmldatamodel", ["Scxml", "Qml"])
            _create_module("ScxmlQml", ["Scxml", "Qml"])
        if self.options.get_safe("qtvirtualkeyboard") and qt_quick_enabled:
            _create_module("VirtualKeyboard", ["Gui", "Qml", "Quick"])
            _create_plugin("QVirtualKeyboardPlugin", "qtvirtualkeyboardplugin", "platforminputcontexts", ["Gui", "Qml", "VirtualKeyboard"])
            _create_plugin("QtVirtualKeyboardHangulPlugin", "qtvirtualkeyboard_hangul", "virtualkeyboard", ["Gui", "Qml", "VirtualKeyboard"])
            _create_plugin("QtVirtualKeyboardMyScriptPlugin", "qtvirtualkeyboard_myscript", "virtualkeyboard", ["Gui", "Qml", "VirtualKeyboard"])
            _create_plugin("QtVirtualKeyboardThaiPlugin", "qtvirtualkeyboard_thai", "virtualkeyboard", ["Gui", "Qml", "VirtualKeyboard"])
        if self.options.get_safe("qt3d"):
            _create_module("3DCore", ["Gui", "Network"])
            _create_module("3DRender", ["3DCore", "OpenGL"])
            _create_module("3DAnimation", ["3DCore", "3DRender", "Gui"])
            _create_module("3DInput", ["3DCore", "Gui"])
            _create_module("3DLogic", ["3DCore", "Gui"])
            _create_module("3DExtras", ["Gui", "3DCore", "3DInput", "3DLogic", "3DRender"])
            _create_plugin("DefaultGeometryLoaderPlugin", "defaultgeometryloader", "geometryloaders", ["3DCore", "3DRender", "Gui"])
            _create_plugin("fbxGeometryLoaderPlugin", "fbxgeometryloader", "geometryloaders", ["3DCore", "3DRender", "Gui"])
            if qt_quick_enabled:
                _create_module("3DQuick", ["3DCore", "Gui", "Qml", "Quick"])
                _create_module("3DQuickAnimation", ["3DAnimation", "3DCore", "3DQuick", "3DRender", "Gui", "Qml"])
                _create_module("3DQuickExtras", ["3DCore", "3DExtras", "3DInput", "3DQuick", "3DRender", "Gui", "Qml"])
                _create_module("3DQuickInput", ["3DCore", "3DInput", "3DQuick", "Gui", "Qml"])
                _create_module("3DQuickRender", ["3DCore", "3DQuick", "3DRender", "Gui", "Qml"])
                _create_module("3DQuickScene2D", ["3DCore", "3DQuick", "3DRender", "Gui", "Qml"])
        if self.options.get_safe("qtimageformats"):
            _create_plugin("ICNSPlugin", "qicns", "imageformats", ["Gui"])
            _create_plugin("QJp2Plugin", "qjp2", "imageformats", ["Gui"])
            _create_plugin("QMacHeifPlugin", "qmacheif", "imageformats", ["Gui"])
            _create_plugin("QMacJp2Plugin", "qmacjp2", "imageformats", ["Gui"])
            _create_plugin("QMngPlugin", "qmng", "imageformats", ["Gui"])
            _create_plugin("QTgaPlugin", "qtga", "imageformats", ["Gui"])
            _create_plugin("QTiffPlugin", "qtiff", "imageformats", ["Gui"])
            _create_plugin("QWbmpPlugin", "qwbmp", "imageformats", ["Gui"])
            _create_plugin("QWebpPlugin", "qwebp", "imageformats", ["Gui"])
        if self.options.get_safe("qtnetworkauth"):
            _create_module("NetworkAuth", ["Network"])
        if self.options.get_safe("qtcoap"):
            _create_module("Coap", ["Network"])
        if self.options.get_safe("qtmqtt"):
            _create_module("Mqtt", ["Network"])
        if self.options.get_safe("qtopcua"):
            _create_module("OpcUa", ["Network"])
            _create_plugin("QOpen62541Plugin", "open62541_backend", "opcua", ["Network", "OpcUa"])
            _create_plugin("QUACppPlugin", "uacpp_backend", "opcua", ["Network", "OpcUa"])

        if self.options.get_safe("qtmultimedia"):
            multimedia_reqs = ["Network", "Gui"]
            if self.options.get_safe("with_libalsa", False):
                multimedia_reqs.append("libalsa::libalsa")
            if self.options.with_openal:
                multimedia_reqs.append("openal-soft::openal-soft")
            if self.options.get_safe("with_pulseaudio", False):
                multimedia_reqs.append("pulseaudio::pulse")
            _create_module("Multimedia", multimedia_reqs)
            _create_module("MultimediaWidgets", ["Multimedia", "Widgets", "Gui"])
            if self.options.qtdeclarative and qt_quick_enabled:
                _create_module("MultimediaQuick", ["Multimedia", "Quick"])
            if self.options.with_gstreamer:
                _create_plugin("QGstreamerMediaPlugin", "gstreamermediaplugin", "multimedia", [
                    "gstreamer::gstreamer",
                    "gst-plugins-base::gst-plugins-base"])

        if self.options.get_safe("qtpositioning"):
            _create_module("Positioning", [])
            _create_plugin("QGeoPositionInfoSourceFactoryGeoclue2", "qtposition_geoclue2", "position", [])
            _create_plugin("QGeoPositionInfoSourceFactoryPoll", "qtposition_positionpoll", "position", [])

        if self.options.get_safe("qtsensors"):
            _create_module("Sensors", [])
            _create_plugin("genericSensorPlugin", "qtsensors_generic", "sensors", [])
            _create_plugin("IIOSensorProxySensorPlugin", "qtsensors_iio-sensor-proxy", "sensors", [])
            if self.settings.os == "Linux":
                _create_plugin("LinuxSensorPlugin", "qtsensors_linuxsys", "sensors", [])
            _create_plugin("QtSensorGesturePlugin", "qtsensorgestures_plugin", "sensorgestures", [])
            _create_plugin("QShakeSensorGesturePlugin", "qtsensorgestures_shakeplugin", "sensorgestures", [])

        if self.options.get_safe("qtconnectivity"):
            _create_module("Bluetooth", ["Network"])
            _create_module("Nfc", [])

        if self.options.get_safe("qtserialport"):
            _create_module("SerialPort", [])

        if self.options.get_safe("qtserialbus"):
            _create_module("SerialBus", ["SerialPort"] if self.options.get_safe("qtserialport") else [])
            _create_plugin("PassThruCanBusPlugin", "qtpassthrucanbus", "canbus", [])
            _create_plugin("PeakCanBusPlugin", "qtpeakcanbus", "canbus", [])
            _create_plugin("SocketCanBusPlugin", "qtsocketcanbus", "canbus", [])
            _create_plugin("TinyCanBusPlugin", "qttinycanbus", "canbus", [])
            _create_plugin("VirtualCanBusPlugin", "qtvirtualcanbus", "canbus", [])

        if self.options.get_safe("qtwebsockets"):
            _create_module("WebSockets", ["Network"])

        if self.options.get_safe("qtwebchannel"):
            _create_module("WebChannel", ["Qml"])

        if self.options.get_safe("qtwebengine") and qt_quick_enabled:
            webenginereqs = ["Gui", "Quick", "WebChannel"]
            if self.options.get_safe("qtpositioning"):
                webenginereqs.append("Positioning")
            if self.settings.os == "Linux":
                webenginereqs.extend(["expat::expat", "opus::libopus", "xorg-proto::xorg-proto", "libxshmfence::libxshmfence", \
                                      "nss::nss", "libdrm::libdrm"])
            _create_module("WebEngineCore", webenginereqs)
            _create_module("WebEngineQuick", ["WebEngineCore"])
            _create_module("WebEngineWidgets", ["WebEngineCore", "Quick", "PrintSupport", "Widgets", "Gui", "Network"])

        if self.options.get_safe("qtremoteobjects"):
            _create_module("RemoteObjects", [])

        if self.options.get_safe("qtwebview"):
            _create_module("WebView", ["Core", "Gui"])

        if self.options.get_safe("qtspeech"):
            _create_module("TextToSpeech", [])

        if self.options.get_safe("qthttpserver"):
            http_server_deps = ["Core", "Network"]
            if self.options.get_safe("qtwebsockets"):
                http_server_deps.append("WebSockets")
            _create_module("HttpServer", http_server_deps)

        if self.options.get_safe("qtgrpc"):
            _create_module("Protobuf", [])
            _create_module("Grpc", ["Core", "Protobuf", "Network"])

        if self.settings.os in ["Windows", "iOS"]:
            if self.settings.os == "Windows":
                self.cpp_info.components["qtEntryPointImplementation"].set_property("cmake_target_name", "Qt6::EntryPointImplementation")
                self.cpp_info.components["qtEntryPointImplementation"].libs = [f"Qt6EntryPoint{libsuffix}"]
                self.cpp_info.components["qtEntryPointImplementation"].system_libs = ["shell32"]

                if self.settings.compiler == "gcc":
                    self.cpp_info.components["qtEntryPointMinGW32"].set_property("cmake_target_name", "Qt6::EntryPointMinGW32")
                    self.cpp_info.components["qtEntryPointMinGW32"].system_libs = ["mingw32"]
                    self.cpp_info.components["qtEntryPointMinGW32"].requires = ["qtEntryPointImplementation"]

            self.cpp_info.components["qtEntryPointPrivate"].set_property("cmake_target_name", "Qt6::EntryPointPrivate")
            if self.settings.os == "Windows":
                if self.settings.compiler == "gcc":
                    self.cpp_info.components["qtEntryPointPrivate"].defines.append("QT_NEEDS_QMAIN")
                    self.cpp_info.components["qtEntryPointPrivate"].requires.append("qtEntryPointMinGW32")
                else:
                    self.cpp_info.components["qtEntryPointPrivate"].requires.append("qtEntryPointImplementation")
            if self.settings.os == "iOS":
                self.cpp_info.components["qtEntryPointPrivate"].exelinkflags.append("-Wl,-e,_qt_main_wrapper")

        if self.settings.os != "Windows":
            self.cpp_info.components["qtCore"].cxxflags.append("-fPIC")

        if not self.options.shared:
            if self.settings.os == "Windows":
                # https://github.com/qt/qtbase/blob/v6.6.1/src/corelib/CMakeLists.txt#L527-L541
                self.cpp_info.components["qtCore"].system_libs.append("advapi32")
                self.cpp_info.components["qtCore"].system_libs.append("authz")
                self.cpp_info.components["qtCore"].system_libs.append("kernel32")
                self.cpp_info.components["qtCore"].system_libs.append("netapi32")
                self.cpp_info.components["qtCore"].system_libs.append("ole32")
                self.cpp_info.components["qtCore"].system_libs.append("shell32")
                self.cpp_info.components["qtCore"].system_libs.append("user32")
                self.cpp_info.components["qtCore"].system_libs.append("uuid")
                self.cpp_info.components["qtCore"].system_libs.append("version")
                self.cpp_info.components["qtCore"].system_libs.append("winmm")
                self.cpp_info.components["qtCore"].system_libs.append("ws2_32")
                self.cpp_info.components["qtCore"].system_libs.append("mpr")
                self.cpp_info.components["qtCore"].system_libs.append("userenv")
                # https://github.com/qt/qtbase/blob/v6.6.1/src/network/CMakeLists.txt#L196-L200
                self.cpp_info.components["qtNetwork"].system_libs.append("advapi32")
                self.cpp_info.components["qtNetwork"].system_libs.append("dnsapi")
                self.cpp_info.components["qtNetwork"].system_libs.append("iphlpapi")
                self.cpp_info.components["qtNetwork"].system_libs.append("secur32")
                self.cpp_info.components["qtNetwork"].system_libs.append("winhttp")
                # https://github.com/qt/qtbase/blob/v6.6.1/src/printsupport/CMakeLists.txt#L70-L75
                self.cpp_info.components["qtPrintSupport"].system_libs.append("gdi32")
                self.cpp_info.components["qtPrintSupport"].system_libs.append("user32")
                self.cpp_info.components["qtPrintSupport"].system_libs.append("comdlg32")
                self.cpp_info.components["qtPrintSupport"].system_libs.append("winspool")

            if is_apple_os(self):
                # https://github.com/qt/qtbase/blob/v6.6.1/src/corelib/CMakeLists.txt#L580-L584
                self.cpp_info.components["qtCore"].frameworks.append("CoreFoundation")
                self.cpp_info.components["qtCore"].frameworks.append("Foundation")
                self.cpp_info.components["qtCore"].frameworks.append("IOKit")
                # https://github.com/qt/qtbase/blob/v6.6.1/src/network/CMakeLists.txt#L205-L214
                self.cpp_info.components["qtNetwork"].frameworks.append("CFNetwork")
                # https://github.com/qt/qtbase/blob/v6.6.1/src/network/CMakeLists.txt#L216-L221
                # qtcore requires "_OBJC_CLASS_$_NSApplication" and more, which are in "Cocoa" framework
                self.cpp_info.components["qtCore"].frameworks.append("Cocoa")
                self.cpp_info.components["qtNetwork"].system_libs.append("resolv")
                if self.options.with_gssapi:
                    # https://github.com/qt/qtbase/blob/v6.6.1/src/network/CMakeLists.txt#L250C56-L253
                    self.cpp_info.components["qtNetwork"].frameworks.append("GSS")
                if self.options.gui and self.options.widgets:
                    # https://github.com/qt/qtbase/blob/v6.6.1/src/printsupport/CMakeLists.txt#L52-L63
                    self.cpp_info.components["qtPrintSupport"].system_libs.append("cups")
                    self.cpp_info.components["qtPrintSupport"].frameworks.append("ApplicationServices")
                if self.settings.os == "Macos":
                    # https://github.com/qt/qtbase/blob/v6.6.1/src/corelib/CMakeLists.txt#L598-L606
                    self.cpp_info.components["qtCore"].frameworks.append("AppKit")
                    self.cpp_info.components["qtCore"].frameworks.append("ApplicationServices")
                    self.cpp_info.components["qtCore"].frameworks.append("CoreServices")
                    self.cpp_info.components["qtCore"].frameworks.append("CoreServices")
                    self.cpp_info.components["qtCore"].frameworks.append("Security")
                    self.cpp_info.components["qtCore"].frameworks.append("DiskArbitration")
                else:
                    # https://github.com/qt/qtbase/blob/v6.6.1/src/corelib/CMakeLists.txt#L969-L972
                    self.cpp_info.components["qtCore"].frameworks.append("MobileCoreServices")
                if self.settings.os not in ["iOS", "tvOS"]:
                    self.cpp_info.components["qtNetwork"].frameworks.append("CoreServices")
                    self.cpp_info.components["qtNetwork"].frameworks.append("SystemConfiguration")
                else:
                    # https://github.com/qt/qtbase/blob/v6.6.1/src/corelib/CMakeLists.txt#L1074-L1077
                    self.cpp_info.components["qtCore"].frameworks.append("UIKit")
                if self.settings.os == "watchOS":
                    # https://github.com/qt/qtbase/blob/v6.6.1/src/corelib/CMakeLists.txt#L1079-L1082
                    self.cpp_info.components["qtCore"].frameworks.append("WatchKit")

        self.cpp_info.components["qtCore"].builddirs.append(os.path.join("bin"))
        _add_build_module("qtCore", self._cmake_executables_file)
        _add_build_module("qtCore", self._cmake_qt6_private_file("Core"))
        if self.settings.os in ["Windows", "iOS"]:
            _add_build_module("qtCore", self._cmake_entry_point_file)

        for m in os.listdir(os.path.join("lib", "cmake")):
            component_name = m.replace("Qt6", "qt")
            if component_name == "qt":
                component_name = "qtCore"

            if component_name in self.cpp_info.components:
                module = os.path.join("lib", "cmake", m, f"{m}Macros.cmake")
                if os.path.isfile(module):
                    _add_build_module(component_name, module)

                module = os.path.join("lib", "cmake", m, f"{m}ConfigExtras.cmake")
                if os.path.isfile(module):
                    _add_build_module(component_name, module)

                for helper_modules in glob.glob(os.path.join(self.package_folder, "lib", "cmake", m, "QtPublic*Helpers.cmake")):
                    _add_build_module(component_name, helper_modules)
                self.cpp_info.components[component_name].builddirs.append(os.path.join("lib", "cmake", m))

            elif component_name.endswith("Tools") and component_name[:-5] in self.cpp_info.components:
                module = os.path.join("lib", "cmake", f"{m}", f"{m[:-5]}Macros.cmake")
                if os.path.isfile(module):
                    _add_build_module(component_name[:-5], module)
                self.cpp_info.components[component_name[:-5]].builddirs.append(os.path.join("lib", "cmake", m))

        objects_dirs = glob.glob(os.path.join(self.package_folder, "lib", "objects-*/"))
        for object_dir in objects_dirs:
            for m in os.listdir(object_dir):
                component = "qt" + m[:m.find("_")]
                if component not in self.cpp_info.components:
                    continue
                for root, _, files in os.walk(os.path.join(object_dir, m)):
                    obj_files = [os.path.join(root, file) for file in files]
                    self.cpp_info.components[component].exelinkflags.extend(obj_files)
                    self.cpp_info.components[component].sharedlinkflags.extend(obj_files)

        build_modules_list = []

        if self.options.qtdeclarative:
            build_modules_list.append(os.path.join(self.package_folder, "lib", "cmake", "Qt6Qml", "conan_qt_qt6_policies.cmake"))

        def _add_build_modules_for_component(component):
            for req in self.cpp_info.components[component].requires:
                if "::" in req: # not a qt component
                    continue
                _add_build_modules_for_component(req)
            build_modules_list.extend(build_modules.pop(component, []))

        for c in self.cpp_info.components:
            _add_build_modules_for_component(c)

        self.cpp_info.set_property("cmake_build_modules", build_modules_list)

        self.conf_info.define("user.qt:tools_directory", os.path.join(self.package_folder, "bin" if self.settings.os == "Windows" else "libexec"))
