#pragma once
#include <functional>
#include <vulkan/vulkan.hpp>
#include "flags.h"
#include "types.h"
#include "macros.h"
namespace rwin
{
    struct DropCallbacks;
    RWIN_API vk::SurfaceKHR createSurface(const std::uint64_t& id,const vk::Instance& instance);
    RWIN_API std::uint64_t getEvents(const std::span<WindowEvent>& events);
    RWIN_API std::uint64_t createWindow(const std::string_view& title,const Extent2D& size,const Flags<WindowFlags>& flags);
    RWIN_API void destroyWindow(const std::uint64_t& id);
    RWIN_API Extent2D getWindowClientSize(const std::uint64_t& id);
    RWIN_API Point2D getWindowClientPosition(const std::uint64_t& id);
    RWIN_API Vector2 getCursorPosition(const std::uint64_t& id);
    RWIN_API void showWindow(const std::uint64_t& id);
    RWIN_API void hideWindow(const std::uint64_t& id);
    RWIN_API void minimizeWindow(const std::uint64_t& id);
    RWIN_API void maximizeWindow(const std::uint64_t& id);
    RWIN_API float getWindowDpi(const std::uint64_t& id);
    RWIN_API float getDefaultDpi();
    RWIN_API void pumpEvents();
    RWIN_API void getRequiredExtensions(std::vector<const char*>& extensions);
    RWIN_API void setWindowHitTestCallback(const std::uint64_t& id, const std::function<HitTestResult(const Vector2&)>& callback);
    RWIN_API void clearWindowHitTestCallback(const std::uint64_t& id);
    RWIN_API void setWindowDropCallbacks(const std::uint64_t& id, const DropCallbacks& callbacks);
    RWIN_API void clearWindowDropCallbacks(const std::uint64_t& id);
}
