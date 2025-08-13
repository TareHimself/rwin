#pragma once
#include "rwin/macros.h"

#ifdef RWIN_PLATFORM_WIN
#include "rwin/IdFactory.h"
#include "rwin/IWindowManager.h"
#include <list>
#include <ObjectArray.h>
#include <string>
#include <optional>
#include <unordered_map>
#include <vulkan/vulkan.hpp>

namespace rwin
{
    struct WindowInfo
    {
        std::uint64_t id{0};
        HWND hwnd{nullptr};
        bool trackingMouse{false};
        IDropTarget* dropTarget{nullptr};
        std::optional<DropCallbacks> dropCallbacks{};
        std::optional<std::function<HitTestResult(const Vector2&)>> hitTestFunction{};
    };

    class WindowsWindowManager final : public IWindowManager {
    public:
        WindowsWindowManager();
        ~WindowsWindowManager() override;
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

        void PumpEvents() override;
        std::list<WindowEvent> pendingEvents = {};
        WindowInfo * GetWindowInfo(const std::uint64_t& id);
        WindowInfo * GetWindowInfo(HWND hwnd);
        void GetRequiredExtensions(std::vector<const char*>& extensions) override;
        void SetHitTestCallback(const std::uint64_t& id, const std::function<HitTestResult(const Vector2&)>& callback) override;
        void ClearHitTestCallback(const std::uint64_t& id) override;
        void Minimize(const std::uint64_t& id) override;
        void Maximize(const std::uint64_t& id) override;
        float GetDpi(const std::uint64_t& id) override;
        float GetDefaultDpi() override;
        void SetDropCallbacks(const std::uint64_t& id, const DropCallbacks& callbacks) override;
        void ClearDropCallbacks(const std::uint64_t& id) override;

    private:
        std::unordered_map<std::uint64_t, WindowInfo> _windows;
        std::unordered_map<HWND,std::uint64_t> _hwndToWindowId;
        IdFactory _idFactory{};
    };
}
#endif