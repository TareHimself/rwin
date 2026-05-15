//#import "DarwinWindowManager.h"
//#import <AppKit/AppKit.h>
//#import <QuartzCore/CAMetalLayer.h>
//
//namespace rwin {
//    struct WindowInfo {
//        __strong NSWindow*     window;
//        __strong CAMetalLayer* layer;
//    };
//
//    WindowInfo* DarwinWindowManager::GetWindow(const std::uint64_t& id) {
//        return _windows.find(id)->second.get();
//    }
//
//    vk::SurfaceKHR DarwinWindowManager::CreateSurface(const std::uint64_t &id, const vk::Instance &instance) {
//    }
//
//    std::uint64_t DarwinWindowManager::GetEvents(const std::span<WindowEvent> &events) {
//
//    }
//
//    std::uint64_t DarwinWindowManager::Create(const std::string_view &title, const Extent2D &size,
//        const Flags<WindowFlags> &flags) {
//
//    }
//
//    void DarwinWindowManager::Destroy(const std::uint64_t &id) {
//
//    }
//
//    Extent2D DarwinWindowManager::GetClientSize(const std::uint64_t &id) {
//    }
//
//    Point2D DarwinWindowManager::GetClientPosition(const std::uint64_t &id) {
//    }
//
//    Vector2 DarwinWindowManager::GetCursorPosition(const std::uint64_t &id) {
//    }
//
//    void DarwinWindowManager::Show(const std::uint64_t &id) {
//    }
//
//    void DarwinWindowManager::Hide(const std::uint64_t &id) {
//    }
//
//    void DarwinWindowManager::Minimize(const std::uint64_t &id) {
//    }
//
//    void DarwinWindowManager::Maximize(const std::uint64_t &id) {
//    }
//
//    float DarwinWindowManager::GetDpi(const std::uint64_t &id) {
//    }
//
//    float DarwinWindowManager::GetDefaultDpi() {
//    }
//
//    void DarwinWindowManager::PumpEvents() {
//    }
//
//    void DarwinWindowManager::GetRequiredExtensions(std::vector<const char *> &extensions) {
//    }
//
//    void DarwinWindowManager::SetHitTestCallback(const std::uint64_t &id,
//        const std::function<HitTestResult(const Vector2 &)> &callback) {
//    }
//
//    void DarwinWindowManager::ClearHitTestCallback(const std::uint64_t &id) {
//    }
//
//    void DarwinWindowManager::SetDropCallbacks(const std::uint64_t &id, const DropCallbacks &callbacks) {
//    }
//
//    void DarwinWindowManager::ClearDropCallbacks(const std::uint64_t &id) {
//    }
//}