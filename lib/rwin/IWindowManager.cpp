#include "rwin/IWindowManager.h"
#include "macros_private.h"

#ifdef RWIN_PLATFORM_COMPAT
#include "glfw/GLFWWindowManager.h"
namespace rwin {
    IWindowManager *IWindowManager::Get() {
        static auto instance = std::make_unique<GLFWWindowManager>();
        return instance.get();
    }
}
#elif defined(RWIN_PLATFORM_WIN)
#include "windows/WindowsWindowManager.h"
namespace rwin {
    IWindowManager *IWindowManager::Get() {
        static auto instance = std::make_unique<WindowsWindowManager>();
        return instance.get();
    }
}
#elif defined(RWIN_PLATFORM_LINUX) && defined(RWIN_PLATFORM_LINUX_WAYLAND)
#include "linux/WaylandWindowManager.h"
namespace rwin {
    IWindowManager *IWindowManager::Get() {
        static auto instance = std::make_unique<WaylandWindowManager>();
        return instance.get();
    }
}
#elif defined(RWIN_PLATFORM_DARWIN)
namespace rwin {
    IWindowManager *IWindowManager::Get() {
        static auto instance = std::make_unique<IWindowManager>();
        return instance.get();
    }
}
#else
namespace rwin {
    IWindowManager *IWindowManager::Get() {
        throw std::runtime_error("No Window Manager");
    }
}
#endif
