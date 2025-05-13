import os
import textwrap

from conan import ConanFile
from conan.tools.build import can_run
from conan.tools.cmake import CMake, cmake_layout
from conan.tools.env import VirtualRunEnv
from conan.tools.files import copy, save


class TestPackageConan(ConanFile):
    settings = "os", "arch", "compiler", "build_type"
    generators = "CMakeDeps", "CMakeToolchain", "VirtualBuildEnv"
    test_type = "explicit"

    def layout(self):
        cmake_layout(self)

    def requirements(self):
        self.requires(self.tested_reference_str, run=can_run(self))

    def build_requirements(self):
        if not can_run(self):
            self.tool_requires(self.tested_reference_str)

    def generate(self):
        qt_install_prefix = self.dependencies["qt"].package_folder.replace("\\", "/")
        qt_conf = textwrap.dedent(f"""\
            [Paths]
            Prefix = {qt_install_prefix}
        """)
        save(self, "qt.conf", qt_conf)

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
