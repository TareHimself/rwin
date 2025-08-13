#include "rwin/rwin.h"

#include "rwin/IWindowManager.h"

namespace rwin
{
    vk::SurfaceKHR createSurface(const std::uint64_t& id,const vk::Instance& instance){
        return IWindowManager::Get()->CreateSurface(id,instance);
    }
    std::uint64_t getEvents(const std::span<WindowEvent>& events){
        return IWindowManager::Get()->GetEvents(events);
    }
    std::uint64_t createWindow(const std::string_view& title,const Extent2D& size,const Flags<WindowFlags>& flags){
        return IWindowManager::Get()->Create(title,size,flags);
    }
    void destroyWindow(const std::uint64_t& id){
        return IWindowManager::Get()->Destroy(id);
    }
    Extent2D getWindowClientSize(const std::uint64_t& id){
        return IWindowManager::Get()->GetClientSize(id);
    }
    Point2D getWindowClientPosition(const std::uint64_t& id){
        return IWindowManager::Get()->GetClientPosition(id);
    }
    Vector2 getCursorPosition(const std::uint64_t& id){
        return IWindowManager::Get()->GetCursorPosition(id);
    }
    void showWindow(const std::uint64_t& id){
        IWindowManager::Get()->Show(id);
    }
    void hideWindow(const std::uint64_t& id){
        IWindowManager::Get()->Hide(id);
    }
    void minimizeWindow(const std::uint64_t& id){
        IWindowManager::Get()->Minimize(id);
    }
    void maximizeWindow(const std::uint64_t& id){
        IWindowManager::Get()->Maximize(id);
    }
    float getWindowDpi(const std::uint64_t& id){
        return IWindowManager::Get()->GetDpi(id);
    }
    float getDefaultDpi(){
        return IWindowManager::Get()->GetDefaultDpi();
    }
    void pumpEvents(){
        IWindowManager::Get()->PumpEvents();
    }
    void getRequiredExtensions(std::vector<const char*>& extensions){
        IWindowManager::Get()->GetRequiredExtensions(extensions);
    }
    void setWindowHitTestCallback(const std::uint64_t& id, const std::function<HitTestResult(const Vector2&)>& callback){
        IWindowManager::Get()->SetHitTestCallback(id,callback);
    }

    void clearWindowHitTestCallback(const std::uint64_t& id){
        IWindowManager::Get()->ClearHitTestCallback(id);
    }

    void setWindowDropCallbacks(const std::uint64_t& id, const DropCallbacks& callbacks){
        IWindowManager::Get()->SetDropCallbacks(id,callbacks);
    }

    void clearWindowDropCallbacks(const std::uint64_t& id)
    {
        IWindowManager::Get()->ClearDropCallbacks(id);
    }


}
