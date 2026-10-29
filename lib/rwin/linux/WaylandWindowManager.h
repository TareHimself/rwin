#pragma once
#include "../macros_private.h"
#if defined(RWIN_PLATFORM_LINUX) && defined(RWIN_PLATFORM_LINUX_WAYLAND)
#include "rwin/IWindowManager.h"
#include <span>
#include <wayland-client-protocol.h>
#include <wayland-cursor.h>
#include <chrono>
#include <list>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include <libdecor.h>
#include <unordered_set>
#include <xdg-shell-client-protocol.h>
#include "rwin/IdFactory.h"
#include "../TextArena.h"
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
        std::int32_t scale{1};
        bool maximized{false};
        bool textInputActive{false};
        std::unordered_set<wl_output*> outputs{};
        libdecor_frame *frame = nullptr;
        Vector2 cursorPosition{};
        std::optional<DropCallbacks> dropCallbacks{};
        std::optional<std::function<HitTestResult(const Vector2&)>> hitTestFunction{};
    };

    // What another client is offering, for the clipboard or for a drag.
    struct DataOffer
    {
        wl_data_offer* offer{nullptr};
        std::vector<std::string> mimeTypes{};
        std::uint32_t sourceActions{0};
        std::uint32_t action{0};
    };

    struct KeyboardInfo {
        xkb_keymap * keymap = nullptr;
        xkb_state * state = nullptr;
        std::unordered_set<std::uint32_t> keysDown{};
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
        void SetFullscreen(const std::uint64_t& id, bool fullscreen) override;
        void SetSize(const std::uint64_t& id, const Extent2D& size) override;
        float GetDpi(const std::uint64_t& id) override;
        float GetDefaultDpi() override;
        void PumpEvents() override;
        void GetRequiredExtensions(std::vector<const char*>& extensions) override;
        void SetHitTestCallback(const std::uint64_t& id,
            const std::function<HitTestResult(const Vector2&)>& callback) override;
        void ClearHitTestCallback(const std::uint64_t& id) override;
        void SetDropCallbacks(const std::uint64_t& id, const DropCallbacks& callbacks) override;
        void ClearDropCallbacks(const std::uint64_t& id) override;
        void StartTextInput(const std::uint64_t& id, const Rect2D& caret) override;
        void StopTextInput(const std::uint64_t& id) override;
        std::u16string_view GetEventText(const TextRef& ref) override;
        bool HasClipboardText() override;
        bool GetClipboardText(std::u16string& out) override;
        bool SetClipboardText(std::u16string_view text) override;
        void ClearClipboard() override;
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
        wl_shm * _shm = nullptr;
        wl_cursor_theme * _cursorTheme = nullptr;
        wl_cursor * _cursor = nullptr;
        wl_surface * _cursorSurface = nullptr;
        std::int32_t _cursorThemeScale = 0;
        libdecor * _decorContext = nullptr;
        wl_seat * _seat = nullptr;
        wl_keyboard * _keyboard = nullptr;
        wl_pointer * _pointer = nullptr;
        xkb_context* _xkbContext = nullptr;
        wl_registry_listener _registryListener{};
        wl_seat_listener _seatListener{};
        wl_keyboard_listener _keyboardListener{};
        wl_pointer_listener _pointerListener{};
        libdecor_interface _decorInterface{};
        libdecor_frame_interface _frameInterface{};
        wl_surface_listener _surfaceListener{};
        wl_output_listener _outputListener{};
        wl_data_device_manager * _dataDeviceManager = nullptr;
        wl_data_device * _dataDevice = nullptr;
        wl_data_device_listener _dataDeviceListener{};
        wl_data_offer_listener _dataOfferListener{};
        wl_data_source_listener _dataSourceListener{};
        std::unordered_map<wl_data_offer*, std::unique_ptr<DataOffer>> _dataOffers{};
        wl_data_offer * _selectionOffer = nullptr;
        wl_data_offer * _dragOffer = nullptr;
        std::uint64_t _dragWindow = std::numeric_limits<std::uint64_t>::max();
        std::uint32_t _dragSerial = 0;
        Vector2 _dragPosition{};
        DropEffect _dragEnterEffect{DropEffect::None};
        DropEffect _dragEffect{DropEffect::None};
        bool _dragAccepted = false;
        // Set while this process owns the clipboard; the text is served from here on request.
        wl_data_source * _clipboardSource = nullptr;
        std::string _clipboardText{};
        std::uint32_t _lastInputSerial = 0;
        void CreateDataDevice();
        DataOffer* FindDataOffer(wl_data_offer* offer);
        void DestroyDataOffer(wl_data_offer* offer);
        DropInfo MakeDropInfo(const DataOffer& offer);
        void ApplyDragEffect(DropEffect effect);
        void EndDrag();
        std::unordered_map<wl_output*, std::int32_t> _outputScales{};
        bool _hasPreferredBufferScale = false;
        bool _pointerHasFrame = false;
        std::int32_t _repeatRate = 25;
        std::int32_t _repeatDelayMs = 600;
        std::uint32_t _repeatKeycode = 0;
        std::chrono::steady_clock::time_point _repeatNext{};
        bool _connectionLost = false;
        TextArena _textArena{};
        Vector2 _scrollContinuous{};
        Vector2 _scrollDiscrete120{};
        void ApplyScale(WindowInfo* info, std::int32_t scale);
        void UpdateScaleFromOutputs(WindowInfo* info);
        void FlushScroll();
        void ApplyCursor(std::uint32_t serial, std::int32_t scale);
        void HandleKey(KeyboardInfo& keyboard, std::uint32_t keycode, InputState state);
        void ProcessKeyRepeat();
        Flags<InputModifier> CurrentModifiers();
        void DispatchEvents();
        Extent2D GetPhysicalSize(const WindowInfo* info) const;
        std::list<WindowEvent> _pendingEvents = {};
        std::uint64_t _cursorFocusedHandle = std::numeric_limits<std::uint64_t>::max();
        std::uint64_t _keyboardFocusedHandle = std::numeric_limits<std::uint64_t>::max();
    };
}
#endif