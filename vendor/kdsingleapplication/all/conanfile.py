from conan import ConanFile
from conan.tools.build import check_min_cppstd
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout
from conan.tools.files import apply_conandata_patches, copy, export_conandata_patches, get, replace_in_file
import os

required_conan_version = ">=2.0.9"

class KDSingleApplicationConan(ConanFile):
    name = "kdsingleapplication"
    description = "KDAB's helper class for single-instance policy applications."
    topics = ("qt", "kdab")
    license = "MIT"
    homepage = "https://github.com/KDAB/KDSingleApplication"
    url = "https://github.com/conan-io/conan-center-index"
    package_type = "library"
    settings = "os", "arch", "compiler", "build_type"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
    }
    default_options = {
        "shared": True,
        "fPIC": True,
    }

    implements = ["auto_shared_fpic"]

    def export_sources(self):
        export_conandata_patches(self)

    def layout(self):
        cmake_layout(self, src_folder="src")

    def validate(self):
        check_min_cppstd(self, 17)

    def requirements(self):
        self.requires("qt/[>=6.7 <7]", transitive_headers=True, transitive_libs=True)

    def build_requirements(self):
        self.tool_requires("qt/<host_version>")

    def source(self):
        get(self, **self.conan_data["sources"][self.version], strip_root=True)
        replace_in_file(self, os.path.join(self.source_folder, "CMakeLists.txt"), "set(CMAKE_CXX_STANDARD 14)", "")

    def generate(self):
        tc = CMakeToolchain(self)
        tc.cache_variables["KDSingleApplication_QT6"] = True
        tc.cache_variables["BUILD_TRANSLATIONS"] = False
        tc.generate()

        deps = CMakeDeps(self)
        deps.generate()

    def build(self):
        apply_conandata_patches(self)
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        copy(self, "LICENSE.txt", self.source_folder, os.path.join(self.package_folder, "licenses"))
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        # self.cpp_info.set_property("cmake_file_name", "KDSingleApplication-qt6")
        self.cpp_info.set_property("cmake_target_name", "KDAB::kdsingleapplication")
        self.cpp_info.set_property("cmake_target_aliases", ["kdsingleapplication"])
        # self.cpp_info.filenames["cmake_find_package"] = "KDSingleApplication-qt6"

