#pragma once
#include "../macros_private.h"

#ifdef RWIN_PLATFORM_DARWIN
#include "rwin/IWindowManager.h"

#include <memory>

namespace rwin {
    // AppKit is main-thread only: every call on this class must be made from the main thread.
    class DarwinWindowManager final : public IWindowManager {
    public:
        DarwinWindowManager();
        ~DarwinWindowManager() override;
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
        void StartTextInput(const std::uint64_t& id, const Rect2D& caret) override;
        void StopTextInput(const std::uint64_t& id) override;
        void SetTextInputRect(const std::uint64_t& id, const Rect2D& caret) override;
        std::u16string_view GetEventText(const TextRef& ref) override;

    private:
        // Window table and event queue live per manager; defined in the .mm to keep AppKit out of the header.
        struct Impl;
        std::unique_ptr<Impl> _impl;
    };
}
#endif
