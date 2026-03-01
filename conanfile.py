from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps
from conan.tools.gnu import PkgConfig, PkgConfigDeps
import os 

class RWin(ConanFile):
    name = "rwin"
    version = "1.0.1"
    url = "https://github.com/TareHimself/rwin.git"
    license = "MIT"
    requires = [
    ]
    git_tag = "main"
    settings = "os", "compiler", "build_type", "arch"
    exports_sources = "CMakeLists.txt", "lib/*", "include/*"
    options = {
            "shared": [True, False],
        }
    default_options = {
        "shared": True,
    }
    
    def config_options(self):
        pass

    def requirements(self):
        if self.settings.os == "Linux":
            self.requires("wayland/1.22.0",options={"shared": True})
            self.requires("wayland-protocols/1.36")
            self.requires("xkbcommon/1.6.0")
            self.requires("libdecor/0.2.2")

    def build_requirements(self):
        if self.settings.os == "Linux":
            if not self.conf.get("tools.gnu:pkg_config", default=False, check_type=str):
                self.tool_requires("pkgconf/[2.2 <3]")
            # This is crucial: use wayland in the build context will make wayland-scanner available from CMake
            self.tool_requires("wayland/<host_version>")

    def layout(self):
        cmake_layout(self)

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        tc.generate()
        pkg_config_deps = PkgConfigDeps(self)
        pkg_config_deps.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure(variables={
            "RWIN_VERSION" : self.version
            })
        cmake.build()

        if self.settings.os == "Linux":
            pkg_config = PkgConfig(self, "wayland-scanner", self.generators_folder)

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.set_property("cmake_file_name", "rwin")
        self.cpp_info.set_property("cmake_target_name", "rwin::rwin")
        self.cpp_info.set_property("pkg_config_name", "rwin")
        self.cpp_info.libs = ["rwin"]
            
