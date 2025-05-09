from conan import ConanFile
from conan.tools.apple import XcodeBuild
from conan.tools.files import get, copy
import os

class sparkleRecipe(ConanFile):
    name = "sparkle"
    package_type = "library"

    # Optional metadata
    license = "MIT"
    url = "https://github.com/sparkle-project/Sparkle"
    description = "A software update framework for macOS "
    topics = ("macos", "framework", "update", "software-update")

    # Binary configuration
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {"shared": False, "fPIC": True}

    def source(self):
        get(self, **self.conan_data["sources"][self.version], strip_root=True)

    def build(self):
        self.run("xcodebuild -project Sparkle.xcodeproj -scheme Sparkle -configuration Release build")

    def package(self):
        copy(self, "LICENSE", src=self.source_folder, dst=os.path.join(self.package_folder, "licenses"))
        copy(self, pattern="*", src=os.path.join(self.build_folder, "Sparkle.framework"), dst=os.path.join(self.package_folder, "lib"))
        copy(self, pattern="*", src=os.path.join(self.build_folder, "Sparkle.framework.dSym"), dst=os.path.join(self.package_folder, "lib"))

    def package_info(self):
        self.cpp_info.set_property("cmake_target_name", "sparkle::sparkle")
