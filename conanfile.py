from conan import ConanFile
from conan.tools.cmake import cmake_layout, CMake

class ClientRecipe(ConanFile):
    settings = "os", "compiler", "arch", "build_type"
    generators = "CMakeToolchain", "CMakeDeps"
    options = {"shared": [True, False]}
    default_options = {
        "shared": True,
        "*:fPIC": True,
        'qt/*:shared': True,
        'qt/*:qtdeclarative': True,
        'qt/*:qtquickcontrols2': True,
        'qt/*:qtshadertools': True,
        'qt/*:qtsvg': True,
        'qt/*:qtimageformats': True,
        'qt/*:qttools': True,
        'qt/*:qttranslations': True,
        'qt/*:gui': True,
        'qt/*:widgets': True,
    }

    def configure(self):
        self.options['qt/*'].with_pq = False
        self.options['qt/*'].with_odbc = False
        if self.settings.os == "Linux":
            self.options['qt/*'].with_dbus = True

    def requirements(self):
        self.requires("extra-cmake-modules/6.8.0")
        self.requires("zlib/1.3.1")
        self.requires("sqlite3/3.49.1")
        self.requires("openssl/3.4.2")
        self.requires("nlohmann_json/3.11.3")
        self.requires("qt/6.8.3")
        self.requires("kdsingleapplication/1.2.0")
        self.requires("qtkeychain/0.15.0")
        self.requires("libregraphapi/1.0.4")
        if self.settings.os == "Macos":
            self.requires("sparkle/2.7.0")

    def build_requirements(self):
        self.tool_requires("cmake/3.30.0")

    def layout(self):
        cmake_layout(self)

    def build(self):
        cmake = CMake(self)
        #cmake.configure(None, None, ["--trace"])
        cmake.configure()
        #cmake.build(None, None, ["--verbose"])
        cmake.build()