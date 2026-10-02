#pragma once
#include <cstdint>
#include <span>
#include <string_view>
#include <vulkan/vulkan.hpp>
#include "types.h"
#include "flags.h"
#include <functional>
#include "macros.h"
#include "DropCallbacks.h"

namespace rwin {
    class RWIN_API IWindowManager
    {
      public:
        virtual ~IWindowManager() = default;
        virtual vk::SurfaceKHR CreateSurface(const std::uint64_t& id,const vk::Instance& instance) = 0;
        virtual std::uint64_t GetEvents(const std::span<WindowEvent>& events) = 0;
        virtual std::uint64_t Create(const std::string_view& title,const Extent2D& size,const Flags<WindowFlags>& flags) = 0;
        virtual void Destroy(const std::uint64_t& id) = 0;
        virtual Extent2D GetClientSize(const std::uint64_t& id) = 0;
        virtual Point2D GetClientPosition(const std::uint64_t& id) = 0;
        virtual Vector2 GetCursorPosition(const std::uint64_t& id) = 0;
        virtual void Show(const std::uint64_t& id) = 0;
        virtual void Hide(const std::uint64_t& id) = 0;
        virtual void Minimize(const std::uint64_t& id) = 0;
        virtual void Maximize(const std::uint64_t& id) = 0;
        virtual float GetDpi(const std::uint64_t& id) = 0;
        virtual float GetDefaultDpi() = 0;
        virtual void PumpEvents() = 0;
        virtual void GetRequiredExtensions(std::vector<const char*>& extensions) = 0;
        virtual void SetHitTestCallback(const std::uint64_t& id, const std::function<HitTestResult(const Vector2&)>& callback) = 0;
        virtual void ClearHitTestCallback(const std::uint64_t& id) = 0;
        virtual void SetDropCallbacks(const std::uint64_t& id, const DropCallbacks& callbacks) = 0;
        virtual void ClearDropCallbacks(const std::uint64_t& id) = 0;

        // Text input is off until StartTextInput. While on, the window receives TextCommit and
        // TextPreedit events, and keys the IME consumes are not reported as KeyEvents. `caret` is
        // the text cursor in window-local backing pixels; platforms use it to place the IME's
        // candidate popup. Backends without text input support keep these no-ops.
        virtual void StartTextInput(const std::uint64_t&, const Rect2D&) {}
        virtual void StopTextInput(const std::uint64_t&) {}
        virtual void SetTextInputRect(const std::uint64_t&, const Rect2D&) {}

        // Text for a TextCommit/TextPreedit event. Valid until the next PumpEvents call, so read or
        // copy it while draining events.
        virtual std::u16string_view GetEventText(const TextRef&) { return {}; }
        static IWindowManager* Get();
    };
}
