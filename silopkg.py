from silo import Component, Options, Package, SiloPackage
from silo.enums import Platform
from silo.tools.cmake import CMakeBuild, CMakeDeps, CMakeGenerator
from silo.tools.pkgconfig import PkgConfigDeps
from silo.tools.pkgconfig.pkg_config_deps import PC_DIR


class RwinOptions(Options):
    # GLFW does the windowing, in place of rwin's own code for the platform
    compat: bool = False


class Rwin(SiloPackage[RwinOptions]):
    name = "rwin"
    version = "1.0.0"
    supports_shared = True

    def source(self, src):
        src.local(".")

    def deps(self):
        self.dep("vulkan-headers@1.3.296")
        self.dep("vulkan-loader@1.3.296")
        if self.options.compat:
            self.dep("glfw@3.4")
        elif self.profile.settings.os is Platform.Linux:
            self.dep("wayland@1.22.0")
            self.dep("wayland-protocols@1.36")
            self.dep("xkbcommon@1.6.0")
            self.dep("libdecor@0.2.2")
            self.tool("wayland@1.22.0")
            self.tool("pkgconf@3")
        self.tool("cmake@4.3")
        self.tool("ninja@1.13")

    def generate(self):
        deps = CMakeDeps(self)
        deps.alias("Vulkan::Vulkan", "vulkan-loader::vulkan-loader")
        if self.options.compat:
            deps.alias("glfw3::glfw", "glfw::glfw")
        deps.generate()
        if self.native_linux:
            PkgConfigDeps(self).generate()
        toolchain = CMakeGenerator(self)
        toolchain.add_cache_bool("RWIN_PLATFORM_COMPAT", self.options.compat)
        toolchain.add_cache_bool("BUILD_SHARED_LIBS", self.options.shared)
        toolchain.generate()

    @property
    def native_linux(self) -> bool:
        return self.profile.settings.os is Platform.Linux and not self.options.compat

    def build(self):
        if self.native_linux:
            self.env["PKG_CONFIG_LIBDIR"] = str(self.context.generated_dir / PC_DIR)
        cmake = CMakeBuild(self)
        cmake.configure()
        cmake.build()
        cmake.install()

    def package(self):
        library = Component("rwin", headers=["include"], requires=["vulkan-loader"])
        if self.options.shared:
            library.shared_libs.append("rwin")
        else:
            library.static_libs.append("rwin")
            library.defines.append("RWIN_STATIC")
        if self.options.compat:
            library.requires.append("glfw")
        elif self.native_linux:
            library.requires += [
                "wayland::wayland-client",
                "wayland::wayland-cursor",
                "wayland-protocols",
                "xkbcommon",
                "libdecor",
            ]
            library.content.append("share/libdecor/plugins-1")
        elif self.profile.settings.os is Platform.Darwin and not self.options.shared:
            # What CMakeLists.txt links for the window manager there
            library.frameworks += ["AppKit", "Metal", "QuartzCore"]
        return Package(components=[library], licenses=["LICENSE"])
