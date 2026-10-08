from conan import ConanFile
from conan.tools.cmake import cmake_layout, CMake

class ClientRecipe(ConanFile):
    settings = "os", "compiler", "arch", "build_type"
    generators = "CMakeToolchain", "CMakeDeps"
    options = {"shared": [True, False]}
    default_options = {
        "*:fPIC": True,
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
        "qt/*:with_md4c": False,
        "qt/*:disabled_features": "designer assistant",
        "shared": True,
    }

    def configure(self):
        if self.settings.os == "Linux":
            self.options['qt/*'].with_dbus = True
            self.options['qt/*'].qtwayland = True
        if self.settings.os == "Macos":
            self.options['harfbuzz/*'].with_glib = False

    def requirements(self):
        self.requires("extra-cmake-modules/6.8.0")
        self.requires("zlib/1.3.2")
        self.requires("sqlite3/3.51.3")
        self.requires("openssl/3.4.8")
        self.requires("nlohmann_json/3.11.3")
        self.requires("qt/6.11.1")
        self.requires("kdsingleapplication/1.2.0")
        self.requires("qtkeychain/0.15.0")
        self.requires("libregraphapi/1.0.4")
        if self.settings.os == "Macos":
            self.requires("sparkle/2.7.0")

    def build_requirements(self):
        self.tool_requires("cmake/3.31.12")

    def layout(self):
        cmake_layout(self)

    def build(self):
        cmake = CMake(self)
        #cmake.configure(None, None, ["--trace"])
        cmake.configure()
        #cmake.build(None, None, ["--verbose"])
        cmake.build()