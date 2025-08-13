#pragma once
#include <cstdint>
#include <span>
#include <vulkan/vulkan.hpp>
#include "macros.h"
#include "types.h"
#include "flags.h"
#include <functional>

#include "DropCallbacks.h"
// EXPORT int platformGet();
//
// EXPORT void platformInit();
//
// EXPORT void platformShutdown();
//
// using PathReceivedCallback = void(RIN_CALLBACK_CONVENTION *)(const char* text);
//
// EXPORT void platformSelectFile(const char * title,bool multiple, const char * filter,PathReceivedCallback callback);
//
// EXPORT void platformSelectPath(const char * title,bool multiple,PathReceivedCallback callback);
//
// EXPORT void platformWindowPump();
//
// enum class WindowFlags : uint32_t
// {
//     None = 0,
//     Frameless =  1 << 0,
//     Floating =  1 << 1,
//     Resizable =  1 << 2,
//     Visible =  1 << 3,
//     Transparent =  1 << 4,
//     Focused =  1 << 5,
// };
//
// EXPORT void* platformWindowCreate(const char * title,int width,int height,Flags<WindowFlags> flags);
//
// EXPORT void platformWindowDestroy(void * handle);
//
// EXPORT void platformWindowShow(void * handle);
//
// EXPORT void platformWindowHide(void * handle);
//
// EXPORT Vector2 platformWindowGetCursorPosition(void * handle);
//
// EXPORT void platformWindowSetCursorPosition(void * handle,Vector2 position);
//
// EXPORT Extent2D platformWindowGetSize(void *handle);
//
// EXPORT VkSurfaceKHR platformWindowCreateSurface(VkInstance instance,void * handle);
//
// EXPORT int platformWindowGetEvents(WindowEvent * output,int size);
//
// EXPORT void platformWindowStartTyping(void* handle);
//
// EXPORT void platformWindowStopTyping(void* handle);
//
// EXPORT void platformWindowSetSize(void* handle,Extent2D size);
//
// EXPORT void platformWindowSetPosition(void* handle,Point2D position);

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
        static IWindowManager* Get();
    };
}
