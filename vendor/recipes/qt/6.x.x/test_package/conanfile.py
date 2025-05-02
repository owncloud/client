import os

from conan import ConanFile
from conan.tools.build import can_run
from conan.tools.cmake import CMake, CMakeToolchain, cmake_layout
from conan.tools.env import VirtualRunEnv
from conan.tools.files import copy, save


class TestPackageConan(ConanFile):
    settings = "os", "arch", "compiler", "build_type"
    generators = "CMakeDeps", "VirtualBuildEnv"
    test_type = "explicit"

    def layout(self):
        cmake_layout(self)

    def requirements(self):
        self.requires(self.tested_reference_str, run=can_run(self))

    def build_requirements(self):
        if not can_run(self):
            self.tool_requires(self.tested_reference_str)
            self.tool_requires("cmake/[>=3.27 <4]")

    def generate(self):
        path = self.dependencies["qt"].package_folder.replace("\\", "/")
        save(self, "qt.conf", f"""[Paths]
Prefix = {path}""")

        tc = CMakeToolchain(self)
        if 'qt' in self.dependencies.build:
            qt_tools_rootdir = self.conf.get("user.qt:tools_directory", None)
            for tool in ["moc", "rcc", "uic"]:
                tc.cache_variables[f"CMAKE_AUTO{tool.upper()}_EXECUTABLE"] = os.path.join(qt_tools_rootdir, f"{tool}.exe" if self.settings_build.os == "Windows" else tool)
        tc.generate()

        VirtualRunEnv(self).generate()
        if can_run(self):
            VirtualRunEnv(self).generate(scope="build")

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def test(self):
        if can_run(self):
            copy(self, "qt.conf", src=self.generators_folder, dst=os.path.join(self.cpp.build.bindirs[0]))
            bin_path = os.path.join(self.cpp.build.bindirs[0], "test_package")
            self.run(bin_path, env="conanrun")
            # Related to https://github.com/conan-io/conan-center-index/issues/20574
            if self.settings.os == "Macos":
                bin_macos_path = os.path.join(self.cpp.build.bindirs[0], "test_macos_bundle.app", "Contents", "MacOS", "test_macos_bundle")
                self.run(bin_macos_path, env="conanrun")

        # Check that the directory exposed in the configuration exists and includes moc
        qt_tools_dir = self.dependencies.host["qt"].conf_info.get("user.qt:tools_directory")
        assert os.path.isdir(qt_tools_dir)
        moc = os.path.join(qt_tools_dir, "moc.exe" if self.settings.os == "Windows" else "moc")
        assert os.path.exists(moc)
