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
    // Borderless fullscreen on the window's monitor; no display mode change. The resulting size arrives as a Resize event.
    RWIN_API void setWindowFullscreen(const std::uint64_t& id, bool fullscreen);
    // Resizes the client area, in the same units getWindowClientSize reports. The platform may clamp or refuse it.
    RWIN_API void setWindowSize(const std::uint64_t& id, const Extent2D& size);
    RWIN_API float getWindowDpi(const std::uint64_t& id);
    RWIN_API float getDefaultDpi();
    RWIN_API void pumpEvents();
    RWIN_API void getRequiredExtensions(std::vector<const char*>& extensions);
    RWIN_API void setWindowHitTestCallback(const std::uint64_t& id, const std::function<HitTestResult(const Vector2&)>& callback);
    RWIN_API void clearWindowHitTestCallback(const std::uint64_t& id);
    RWIN_API void setWindowDropCallbacks(const std::uint64_t& id, const DropCallbacks& callbacks);
    RWIN_API void clearWindowDropCallbacks(const std::uint64_t& id);
    RWIN_API void startTextInput(const std::uint64_t& id, const Rect2D& caret);
    RWIN_API void stopTextInput(const std::uint64_t& id);
    RWIN_API void setTextInputRect(const std::uint64_t& id, const Rect2D& caret);
    RWIN_API std::u16string_view getEventText(const TextRef& ref);
    RWIN_API bool hasClipboardText();
    // Returns false and leaves `out` alone if the clipboard is empty or does not hold text.
    RWIN_API bool getClipboardText(std::u16string& out);
    RWIN_API bool setClipboardText(std::u16string_view text);
    RWIN_API void clearClipboard();
}
