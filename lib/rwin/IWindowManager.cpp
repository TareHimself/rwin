#include "rwin/IWindowManager.h"
#ifdef RWIN_PLATFORM_WIN
#include "windows/WindowsWindowManager.h"

namespace rwin
{
    IWindowManager* IWindowManager::Get()
    {
        static auto instance = std::make_unique<WindowsWindowManager>();
        return instance.get();
    }

#elif RWIN_PLATFORM_LINUX && (RWIN_PLATFORM_LINUX_WAYLAND || RWIN_PLATFORM_LINUX_X11)
        IWindowManager* IWindowManager::Get()
        {
                static auto instance = std::make_unique<IWindowManager>();
                return instance.get();
        }
#elif RWIN_PLATFORM_MAC
        IWindowManager* IWindowManager::Get()
        {
                static auto instance = std::make_unique<IWindowManager>();
                return instance.get();
        }
#else
        IWindowManager* IWindowManager::Get()
        {
                throw std::runtime_error("No Window Manager");
        }
#endif
}
