from silo import Component, InvalidConfiguration, Package, SiloPackage
from silo.enums import Platform
from silo.tools.cmake import CMakeBuild, CMakeDeps, CMakeGenerator
from silo.tools.pkgconfig import PkgConfigDeps
from silo.tools.pkgconfig.pkg_config_deps import PC_DIR


class Rwin(SiloPackage):
    name = "rwin"
    version = "1.0.0"

    def source(self, src):
        src.local(".")

    def deps(self):
        self.dep("vulkan-headers@1.3.296")
        self.dep("vulkan-loader@1.3.296")
        if self.profile.settings.os is Platform.Linux:
            self.dep("wayland@1.22.0")
            self.dep("wayland-protocols@1.36")
            self.dep("xkbcommon@1.6.0")
            self.dep("libdecor@0.2.2")
            self.tool("wayland@1.22.0")
            self.tool("pkgconf@3")
        self.tool("cmake@4.3")
        self.tool("ninja@1.13")

    def validate(self):
        if self.profile.settings.os is Platform.Darwin:
            raise InvalidConfiguration("rwin is not built with silo on macOS yet")

    def generate(self):
        deps = CMakeDeps(self)
        deps.alias("Vulkan::Vulkan", "vulkan-loader::vulkan-loader")
        deps.generate()
        if self.profile.settings.os is Platform.Linux:
            PkgConfigDeps(self).generate()
        CMakeGenerator(self).generate()

    def build(self):
        if self.profile.settings.os is Platform.Linux:
            self.env["PKG_CONFIG_LIBDIR"] = str(self.context.generated_dir / PC_DIR)
        cmake = CMakeBuild(self)
        cmake.configure()
        cmake.build()
        cmake.install()

    def package(self):
        library = Component(
            "rwin",
            headers=["include"],
            static_libs=["rwin"],
            requires=["vulkan-loader"],
        )
        if self.profile.settings.os is Platform.Linux:
            library.requires += [
                "wayland::wayland-client",
                "wayland::wayland-cursor",
                "wayland-protocols",
                "xkbcommon",
                "libdecor",
            ]
            library.content.append("share/libdecor/plugins-1")
        return Package(components=[library], licenses=["LICENSE"])
