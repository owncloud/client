from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.build import check_min_cppstd
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout
from conan.tools.files import copy, get, replace_in_file, rmdir
from conan.tools.system import package_manager
import os


required_conan_version = ">=2.0.9"
class PackageConan(ConanFile):
    name = "qtkeychain"
    description = "Platform-independent Qt API for storing passwords securely."
    topics = ("qt", "keychain")
    license = "BSD-3-Clause"
    homepage = "https://github.com/frankosterfeld/qtkeychain"
    url = "https://github.com/conan-io/conan-center-index"

    package_type = "library"
    settings = "os", "arch", "compiler", "build_type"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
    }
    default_options = {
        "shared": False,
        "fPIC": True,
    }

    implements = ["auto_shared_fpic"]

    def layout(self):
        cmake_layout(self, src_folder="src")

    def validate(self):
        check_min_cppstd(self, 17)
        qt_options = self.dependencies["qt"].options
        if self.settings.os in ("Linux", "FreeBSD") and not qt_options.with_dbus:
            raise ConanInvalidConfiguration(f'{self.ref} requires -o="qt/*:with_dbus=True" in {self.settings.os}')

    def requirements(self):
        self.requires("qt/[>=6.7 <7]", transitive_headers=True, transitive_libs=True)
        # self.requires("dbus/1.15.8")

    def build_requirements(self):
        self.tool_requires("qt/<host_version>")

    def system_requirements(self):
        apt = package_manager.Apt(self)
        apt.install(["libsecret-1-dev"], update=True, check=True)

    def source(self):
        get(self, **self.conan_data["sources"][self.version], strip_root=True)
        replace_in_file(self, os.path.join(self.source_folder, "CMakeLists.txt"),
                        "SET(CMAKE_CXX_STANDARD 11)", "")

    def generate(self):
        tc = CMakeToolchain(self)
        tc.cache_variables["BUILD_WITH_QT6"] = True
        # Settings this to True would need LinguistTools, which requires qttools, gui and widgets
        # qttools is not built by default in qt package, so we would need to add a check in validate()
        tc.cache_variables["BUILD_TRANSLATIONS"] = False
        tc.generate()

        deps = CMakeDeps(self)
        # CMakeLists expects us to have created Qt6Core specific variables, but we don't have them
        # So make the whole target generate them, this overlinks uneeded stuff though
        deps.set_property("qt", "cmake_additional_variables_prefixes", ["Qt6Core"])
        deps.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        copy(self, "COPYING", self.source_folder, os.path.join(self.package_folder, "licenses"))
        cmake = CMake(self)
        cmake.install()
        rmdir(self, os.path.join(self.package_folder, "lib", "cmake"))

    def package_info(self):
        self.cpp_info.libs = ["qt6keychain"]
