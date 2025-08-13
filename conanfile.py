from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMake, cmake_layout, CMakeDeps
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

    def layout(self):
        cmake_layout(self)

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()
        tc = CMakeToolchain(self)
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure(variables={
            "RWIN_VERSION" : self.version
            })
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        self.cpp_info.set_property("cmake_file_name", "rwin")
        self.cpp_info.set_property("cmake_target_name", "rwin::rwin")
        self.cpp_info.set_property("pkg_config_name", "rwin")
        self.cpp_info.libs = ["rwin"]
            
