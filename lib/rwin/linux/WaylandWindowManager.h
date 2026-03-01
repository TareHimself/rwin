#pragma once
#include "rwin/macros.h"
#if defined(RWIN_PLATFORM_LINUX) && defined(RWIN_PLATFORM_LINUX_WAYLAND)
#include "rwin/IWindowManager.h"
#include <span>
#include <wayland-client-protocol.h>
#include <libdecor.h>
#include <unordered_set>
#include <xdg-shell-client-protocol.h>
#include "rwin/IdFactory.h"
#include <xkbcommon/xkbcommon.h>

namespace rwin
{
    class WaylandWindowManager;

    struct WindowInfo
    {
        std::uint64_t windowId{};
        WaylandWindowManager* windowManager{ nullptr };
        wl_surface *surface = nullptr;
        Flags<WindowFlags> flags{};
        Extent2D size{};
        libdecor_frame *frame = nullptr;
        Vector2 cursorPosition{};
        std::optional<DropCallbacks> dropCallbacks{};
        std::optional<std::function<HitTestResult(const Vector2&)>> hitTestFunction{};
    };

    struct KeyboardInfo {
        xkb_keymap * keymap = nullptr;
        xkb_state * state = nullptr;
        std::unordered_set<InputKey> keysPressed{};
        KeyboardInfo(xkb_context* context,const char * keymapString,xkb_keymap_format format) ;

        ~KeyboardInfo() ;
    };

    class WaylandWindowManager final : public IWindowManager
    {
    public:
        WaylandWindowManager();
        ~WaylandWindowManager();
        vk::SurfaceKHR CreateSurface(const std::uint64_t& id, const vk::Instance& instance) override;
        std::uint64_t GetEvents(const std::span<WindowEvent>& events) override;
        std::uint64_t Create(const std::string_view& title, const Extent2D& size,
            const Flags<WindowFlags>& flags) override;
        void Destroy(const std::uint64_t& id) override;
        Extent2D GetClientSize(const std::uint64_t& id) override;
        Point2D GetClientPosition(const std::uint64_t& id) override;
        Vector2 GetCursorPosition(const std::uint64_t& id) override;
        void Show(const std::uint64_t& id) override;
        void Hide(const std::uint64_t& id) override;
        void Minimize(const std::uint64_t& id) override;
        void Maximize(const std::uint64_t& id) override;
        float GetDpi(const std::uint64_t& id) override;
        float GetDefaultDpi() override;
        void PumpEvents() override;
        void GetRequiredExtensions(std::vector<const char*>& extensions) override;
        void SetHitTestCallback(const std::uint64_t& id,
            const std::function<HitTestResult(const Vector2&)>& callback) override;
        void ClearHitTestCallback(const std::uint64_t& id) override;
        void SetDropCallbacks(const std::uint64_t& id, const DropCallbacks& callbacks) override;
        void ClearDropCallbacks(const std::uint64_t& id) override;
    private:
        std::unordered_map<std::uint64_t,std::shared_ptr<WindowInfo>> _windows{};
        std::unordered_map<wl_surface*,std::shared_ptr<WindowInfo>> _surfaceToWindows{};
        std::unordered_map<const wl_keyboard*,std::shared_ptr<KeyboardInfo>> _keyboards{};
        IdFactory _idFactory{};
        WindowInfo * GetWindowInfo(const std::uint64_t& id);
        WindowInfo * GetWindowInfo(wl_surface* surface);

        wl_registry * _registry = nullptr;
        wl_compositor * _compositor = nullptr;
        wl_display * _display = nullptr;
        xdg_wm_base * _shell = nullptr;
        libdecor * _decorContext = nullptr;
        wl_seat * _seat = nullptr;
        wl_keyboard * _keyboard = nullptr;
        wl_pointer * _pointer = nullptr;
        xkb_context* _xkbContext = nullptr;
        wl_display_listener _displayListener{};
        wl_registry_listener _registryListener{};
        wl_seat_listener _seatListener{};
        wl_keyboard_listener _keyboardListener{};
        wl_pointer_listener _pointerListener{};
        libdecor_interface _decorInterface{};
        libdecor_frame_interface _frameInterface{};
        std::list<WindowEvent> _pendingEvents = {};
        std::uint64_t _cursorFocusedHandle{};
        std::uint64_t _keyboardFocusedHandle{};
    };
}
#endif