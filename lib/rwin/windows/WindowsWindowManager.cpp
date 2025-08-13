#include "WindowsWindowManager.h"
#ifdef RWIN_PLATFORM_WIN
#include <windows.h>
#include <shlobj.h>
#include <combaseapi.h>
#include <stringapiset.h>
#include <dwmapi.h>
#include <windowsx.h>
#include <vulkan/vulkan_win32.h>
#include <atlbase.h>
#include <atlcom.h>

#include "rwin/IDropContext.h"
#pragma comment (lib, "Dwmapi")
namespace rwin
{
    LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static HINSTANCE INSTANCE;
    const LPCSTR DEFAULT_WINDOW_CLASS_NAME = "Default Window Class";
    WindowsWindowManager* MANAGER_INSTANCE = nullptr;


    struct DropContext : IDropContext
    {
        bool HasFiles() override
        {
            return _hasFiles;
        }
        bool HasText() override
        {
            return _hasText;
        }

        bool GetFiles(std::vector<std::filesystem::path>& paths) override
        {
            FORMATETC formatEtc = { CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
            STGMEDIUM stgMedium;

            if (SUCCEEDED(_dataObject->GetData(&formatEtc, &stgMedium)))
            {
                if (const auto hDrop = static_cast<HDROP>(GlobalLock(stgMedium.hGlobal)))
                {
                    const auto fileCount = DragQueryFileW(hDrop, 0xFFFFFFFF, nullptr, 0);
                    for (auto i = 0; i < fileCount; ++i)
                    {
                        wchar_t filePath[MAX_PATH];
                        if (DragQueryFileW(hDrop, i, filePath, MAX_PATH))
                        {
                            paths.emplace_back(filePath);
                        }
                    }
                    GlobalUnlock(stgMedium.hGlobal);

                    return true;
                }

                ReleaseStgMedium(&stgMedium);
            }

            return false;
        }

        bool GetText(std::vector<std::string>& text) override
        {
            if (!_dataObject) return false;

            FORMATETC fmt = { CF_UNICODETEXT, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
            STGMEDIUM stg;

            if (FAILED(_dataObject->GetData(&fmt, &stg)))
            {
                // Fallback to CF_TEXT (ANSI)
                fmt.cfFormat = CF_TEXT;
                if (FAILED(_dataObject->GetData(&fmt, &stg)))
                    return false;
            }

            bool success = false;
            if (stg.tymed == TYMED_HGLOBAL && stg.hGlobal)
            {
                const void* data = GlobalLock(stg.hGlobal);
                if (data)
                {
                    std::string output;

                    if (fmt.cfFormat == CF_UNICODETEXT)
                    {
                        const wchar_t* wtext = static_cast<const wchar_t*>(data);
                        int len = WideCharToMultiByte(CP_UTF8, 0, wtext, -1, nullptr, 0, nullptr, nullptr);
                        if (len > 0)
                        {
                            output.resize(len - 1); // exclude null terminator
                            WideCharToMultiByte(CP_UTF8, 0, wtext, -1, output.data(), len, nullptr, nullptr);
                        }
                    }
                    else // CF_TEXT
                    {
                        const char* ctext = static_cast<const char*>(data);
                        output = std::string(ctext);
                    }

                    GlobalUnlock(stg.hGlobal);

                    // Split lines and push
                    std::istringstream iss(output);
                    std::string line;
                    while (std::getline(iss, line))
                    {
                        if (!line.empty())
                        {
                            text.emplace_back(line);
                            success = true;
                        }
                    }
                }
            }

            ReleaseStgMedium(&stg);
            return success;
        }
        explicit DropContext(IDataObject* dataObject)
        {
            _dataObject = dataObject;
            CComPtr<IEnumFORMATETC> enumFormat;
            if (FAILED(dataObject->EnumFormatEtc(DATADIR_GET, &enumFormat))) return;

            FORMATETC fmt;
            while (enumFormat->Next(1, &fmt, nullptr) == S_OK) {

                if (fmt.cfFormat == CF_HDROP) {
                    _hasFiles = true;
                } else if (fmt.cfFormat == CF_UNICODETEXT || fmt.cfFormat == CF_TEXT) {
                    _hasText = true;
                }
                // else if (fmt.cfFormat == CF_BITMAP || fmt.cfFormat == CF_DIB || fmt.cfFormat == CF_DIBV5) {
                //     flags.Add(DndFormatFlags::Image);
                // }
            }
        }

    private:

        bool _hasFiles{false};
        bool _hasText{false};
        IDataObject* _dataObject{};
    };
    class WindowDropTarget final : public IDropTarget
    {
    public:
        explicit WindowDropTarget(const std::uint64_t& windowId)
        {
            _windowId = windowId;
            _refCount = 1;
        }

        HRESULT QueryInterface(const IID& riid, void** ppvObject) override
        {
            if (!ppvObject) return E_POINTER;
            *ppvObject = nullptr;

            if (riid == IID_IUnknown || riid == IID_IDropTarget) {
                *ppvObject = static_cast<IDropTarget*>(this);
                AddRef();
                return S_OK;
            }

            return E_NOINTERFACE;
        }
        ULONG AddRef() override
        {
            return InterlockedIncrement(&_refCount);
        }
        ULONG Release() override
        {
            auto count = InterlockedDecrement(&_refCount);
            if (count == 0)
            {
                delete this;
            }
            return count;
        }

        [[nodiscard]] WindowInfo * GetWindowInfo() const
        {
            return MANAGER_INSTANCE->GetWindowInfo(_windowId);
        }

        static Vector2 GetClientPosition(WindowInfo * info,POINTL pt)
        {
            POINT point{pt.x,pt.y};
            ScreenToClient(info->hwnd,&point);
            return Vector2(point.x,point.y);
        }

        HRESULT DragEnter(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) override
        {
            bool accept = false;
            if (const auto info = GetWindowInfo())
            {
                const auto ctx = std::make_shared<DropContext>(pDataObj);
                accept = info->dropCallbacks.has_value() && info->dropCallbacks->enter(GetClientPosition(info,pt),ctx.get());
                if (accept)
                {
                    _dropContext = ctx;
                }
            }
            *pdwEffect = accept ? DROPEFFECT_COPY : DROPEFFECT_NONE;
            return S_OK;
        }
        HRESULT DragOver(DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) override
        {
            bool accept = false;
            if (const auto info = GetWindowInfo(); info && _dropContext)
            {
                accept = info->dropCallbacks.has_value() && info->dropCallbacks->over(GetClientPosition(info,pt),_dropContext.get());
            }
            *pdwEffect = accept ? DROPEFFECT_COPY : DROPEFFECT_NONE;
            return S_OK;
        }
        HRESULT DragLeave() override
        {
            if (const auto info = GetWindowInfo(); info && _dropContext)
            {
                if (info->dropCallbacks.has_value())
                {
                    info->dropCallbacks->leave();
                }
            }
            return S_OK;
        }
        HRESULT Drop(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) override
        {
            if (const auto info = GetWindowInfo(); info && _dropContext)
            {
                if (info->dropCallbacks.has_value())
                {
                    info->dropCallbacks->drop(GetClientPosition(info,pt),_dropContext.get());
                }
            }
            return S_OK;
        }

    private:
        ULONG _refCount;
        std::uint64_t _windowId;
        std::shared_ptr<DropContext> _dropContext;
    };
    InputKey MapVirtualKeyToInputKey(UINT vk, LPARAM lParam)
    {
        // Extended key flag from lParam
        bool isExtended = (lParam >> 24) & 0x1;

        switch (vk)
        {
        // A-Z
        case 'A': return InputKey::A;
        case 'B': return InputKey::B;
        case 'C': return InputKey::C;
        case 'D': return InputKey::D;
        case 'E': return InputKey::E;
        case 'F': return InputKey::F;
        case 'G': return InputKey::G;
        case 'H': return InputKey::H;
        case 'I': return InputKey::I;
        case 'J': return InputKey::J;
        case 'K': return InputKey::K;
        case 'L': return InputKey::L;
        case 'M': return InputKey::M;
        case 'N': return InputKey::N;
        case 'O': return InputKey::O;
        case 'P': return InputKey::P;
        case 'Q': return InputKey::Q;
        case 'R': return InputKey::R;
        case 'S': return InputKey::S;
        case 'T': return InputKey::T;
        case 'U': return InputKey::U;
        case 'V': return InputKey::V;
        case 'W': return InputKey::W;
        case 'X': return InputKey::X;
        case 'Y': return InputKey::Y;
        case 'Z': return InputKey::Z;

        // 0-9
        case '0': return InputKey::Zero;
        case '1': return InputKey::One;
        case '2': return InputKey::Two;
        case '3': return InputKey::Three;
        case '4': return InputKey::Four;
        case '5': return InputKey::Five;
        case '6': return InputKey::Six;
        case '7': return InputKey::Seven;
        case '8': return InputKey::Eight;
        case '9': return InputKey::Nine;

        // Function keys
        case VK_F1: return InputKey::F1;
        case VK_F2: return InputKey::F2;
        case VK_F3: return InputKey::F3;
        case VK_F4: return InputKey::F4;
        case VK_F5: return InputKey::F5;
        case VK_F6: return InputKey::F6;
        case VK_F7: return InputKey::F7;
        case VK_F8: return InputKey::F8;
        case VK_F9: return InputKey::F9;
        case VK_F10: return InputKey::F10;
        case VK_F11: return InputKey::F11;
        case VK_F12: return InputKey::F12;
        case VK_F13: return InputKey::F13;
        case VK_F14: return InputKey::F14;
        case VK_F15: return InputKey::F15;
        case VK_F16: return InputKey::F16;
        case VK_F17: return InputKey::F17;
        case VK_F18: return InputKey::F18;
        case VK_F19: return InputKey::F19;
        case VK_F20: return InputKey::F20;
        case VK_F21: return InputKey::F21;
        case VK_F22: return InputKey::F22;
        case VK_F23: return InputKey::F23;
        case VK_F24: return InputKey::F24;

        // Punctuation and symbols
        case VK_SPACE: return InputKey::Space;
        case VK_OEM_7: return InputKey::Apostrophe;
        case VK_OEM_COMMA: return InputKey::Comma;
        case VK_OEM_MINUS: return InputKey::Minus;
        case VK_OEM_PERIOD: return InputKey::Period;
        case VK_OEM_2: return InputKey::Slash;
        case VK_OEM_1: return InputKey::Semicolon;
        case VK_OEM_PLUS: return InputKey::Equal;
        case VK_OEM_4: return InputKey::LeftBracket;
        case VK_OEM_5: return InputKey::Backslash;
        case VK_OEM_6: return InputKey::RightBracket;
        case VK_OEM_3: return InputKey::GraveAccent;

        // Control keys
        case VK_ESCAPE: return InputKey::Escape;
        case VK_RETURN: return InputKey::Enter;
        case VK_TAB: return InputKey::Tab;
        case VK_BACK: return InputKey::Backspace;
        case VK_INSERT: return InputKey::Insert;
        case VK_DELETE: return InputKey::Delete;
        case VK_HOME: return InputKey::Home;
        case VK_END: return InputKey::End;
        case VK_PRIOR: return InputKey::PageUp;
        case VK_NEXT: return InputKey::PageDown;

        // Arrows
        case VK_LEFT: return InputKey::Left;
        case VK_RIGHT: return InputKey::Right;
        case VK_UP: return InputKey::Up;
        case VK_DOWN: return InputKey::Down;

        // Locks and pause
        case VK_CAPITAL: return InputKey::CapsLock;
        case VK_SCROLL: return InputKey::ScrollLock;
        case VK_NUMLOCK: return InputKey::NumLock;
        case VK_SNAPSHOT: return InputKey::PrintScreen;
        case VK_PAUSE: return InputKey::Pause;

        // Left/Right modifiers
        case VK_SHIFT:
            {
                // Distinguish LSHIFT / RSHIFT using scan code
                UINT sc = (lParam >> 16) & 0xFF;
                return (sc == 0x36) ? InputKey::RightShift : InputKey::LeftShift;
            }
        case VK_CONTROL: return isExtended ? InputKey::RightControl : InputKey::LeftControl;
        case VK_MENU: return isExtended ? InputKey::RightAlt : InputKey::LeftAlt;
        case VK_LWIN: return InputKey::LeftSuper;
        case VK_RWIN: return InputKey::RightSuper;
        case VK_APPS: return InputKey::Menu;

        default: return InputKey::A; // Fallback or unknown
        }
    }

    LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        auto windowInfo = MANAGER_INSTANCE->GetWindowInfo(hwnd);
        if (windowInfo == nullptr) return DefWindowProc(hwnd, uMsg, wParam, lParam);

        switch (uMsg)
        {
        // case WM_KEYUP:
        //     break;
        case WM_NCHITTEST:
            {
                LRESULT hit = DefWindowProc(hwnd, uMsg, wParam, lParam);
                if (hit == HTCLIENT && windowInfo->hitTestFunction.has_value())
                {
                    int x = GET_X_LPARAM(lParam);
                    int y = GET_Y_LPARAM(lParam);
                    POINT point;
                    point.x = x;
                    point.y = y;
                    ScreenToClient(windowInfo->hwnd,&point);
                    switch ((*windowInfo->hitTestFunction)(Vector2(point.x, point.y)))
                    {
                    case HitTestResult::None:
                        break;
                    case HitTestResult::DragArea:
                        hit = HTCAPTION;
                        break;
                    case HitTestResult::TopResize:
                        hit = HTTOP;
                        break;
                    case HitTestResult::LeftResize:
                        hit = HTLEFT;
                        break;
                    case HitTestResult::RightResize:
                        hit = HTRIGHT;
                        break;
                    case HitTestResult::BottomResize:
                        hit = HTBOTTOM;
                        break;
                    case HitTestResult::CloseButton:
                        hit = HTCLOSE;
                        break;
                    case HitTestResult::MinimizeButton:
                        hit = HTMINBUTTON;
                        break;
                    case HitTestResult::MaximizeButton:
                        hit = HTMAXBUTTON;
                        break;
                    default: ;
                    }
                }
                return hit;
            }
        case WM_MOUSEHOVER:
            {
                return 0;
            }
        case WM_MOUSELEAVE:
            {
                windowInfo->trackingMouse = false;
                WindowEvent ev{};
                new(&ev.cursorFocus) FocusEvent{
                    .type = WindowEventType::CursorFocus,
                    .windowId = windowInfo->id,
                    .focused = 0,
                };
                MANAGER_INSTANCE->pendingEvents.push_back(ev);
                return 0;
            }
        case WM_MOUSEMOVE:
            {
                WindowEvent ev{};

                if (!windowInfo->trackingMouse)
                {
                    TRACKMOUSEEVENT event{};
                    event.dwFlags = TME_LEAVE;
                    event.dwHoverTime = 0;
                    event.hwndTrack = hwnd;
                    event.cbSize = sizeof(TRACKMOUSEEVENT);
                    if (TrackMouseEvent(&event))
                    {
                        windowInfo->trackingMouse = true;

                        new(&ev.cursorFocus) FocusEvent{
                            .type = WindowEventType::CursorFocus,
                            .windowId = windowInfo->id,
                            .focused = 1,
                        };
                        MANAGER_INSTANCE->pendingEvents.push_back(ev);
                    }
                    else
                    {
                        return DefWindowProc(hwnd, uMsg, wParam, lParam);
                    }
                }

                const float x = GET_X_LPARAM(lParam);
                const float y = GET_Y_LPARAM(lParam);

                new(&ev.cursorMove) CursorMoveEvent{
                    .type = WindowEventType::CursorMove,
                    .windowId = windowInfo->id,
                    .position = Vector2{x, y},
                };
                MANAGER_INSTANCE->pendingEvents.push_back(ev);
                return 0;
            }
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        case WM_XBUTTONDOWN:
        case WM_XBUTTONUP:
            {
                WindowEvent ev{};

                // Map message to InputState
                const bool isDown = (uMsg == WM_LBUTTONDOWN || uMsg == WM_RBUTTONDOWN ||
                    uMsg == WM_MBUTTONDOWN || uMsg == WM_XBUTTONDOWN);
                const auto state = isDown ? InputState::Pressed : InputState::Released;

                // Determine which button
                CursorButton button;
                switch (uMsg)
                {
                case WM_LBUTTONDOWN:
                case WM_LBUTTONUP:
                    button = CursorButton::One;
                    break;
                case WM_RBUTTONDOWN:
                case WM_RBUTTONUP:
                    button = CursorButton::Two;
                    break;
                case WM_MBUTTONDOWN:
                case WM_MBUTTONUP:
                    button = CursorButton::Three;
                    break;
                case WM_XBUTTONDOWN:
                case WM_XBUTTONUP:
                    {
                        WORD btn = GET_XBUTTON_WPARAM(wParam);
                        if (btn == XBUTTON1)
                            button = CursorButton::Four;
                        else if (btn == XBUTTON2)
                            button = CursorButton::Five;
                        else
                            button = CursorButton::Six; // fallback
                        break;
                    }
                default:
                    button = CursorButton::One;
                    break; // fallback
                }

                // Modifier keys
                Flags<InputModifier> modifiers{};
                if (GetKeyState(VK_SHIFT) & KF_UP) modifiers.Add(InputModifier::Shift);
                if (GetKeyState(VK_CONTROL) & KF_UP) modifiers.Add(InputModifier::Control);
                if (GetKeyState(VK_MENU) & KF_UP) modifiers.Add(InputModifier::Alt);
                if ((GetKeyState(VK_LWIN) & KF_UP) || (GetKeyState(VK_RWIN) & KF_UP))
                    modifiers.Add(InputModifier::Super);
                if (GetKeyState(VK_CAPITAL) & 0x0001) modifiers.Add(InputModifier::CapsLock);
                if (GetKeyState(VK_NUMLOCK) & 0x0001) modifiers.Add(InputModifier::NumLock);

                new(&ev.cursorButton) CursorButtonEvent{
                    .type = WindowEventType::CursorButton,
                    .windowId = windowInfo->id,
                    .button = button,
                    .state = state,
                    .modifier = static_cast<InputModifier>(modifiers),
                };
                MANAGER_INSTANCE->pendingEvents.push_back(ev);
                return 0;
            }
        case WM_CHAR:
            {
                WindowEvent ev{};
                new(&ev.text) TextEvent{
                    .type = WindowEventType::Text,
                    .windowId = windowInfo->id,
                    .text = static_cast<char16_t>(wParam)
                };
                MANAGER_INSTANCE->pendingEvents.push_back(ev);
                return 0;
            }
        case WM_KEYDOWN:
        case WM_KEYUP:
            {
                WindowEvent evt{};

                // Fill basic event info

                // Convert virtual key to your InputKey enum
                const auto key = MapVirtualKeyToInputKey(static_cast<UINT>(wParam), lParam);

                auto keyFlags = HIWORD(lParam);
                // State (pressed / released / repeat)
                bool isRepeat = (keyFlags & KF_REPEAT) == KF_REPEAT;
                const auto state = (uMsg == WM_KEYUP)
                                       ? InputState::Released
                                       : (isRepeat ? InputState::Repeat : InputState::Pressed);
                // Modifier bitmask
                Flags<InputModifier> modifiers{};
                if (GetKeyState(VK_SHIFT) & KF_UP) modifiers.Add(InputModifier::Shift);
                if (GetKeyState(VK_CONTROL) & KF_UP) modifiers.Add(InputModifier::Control);
                if (GetKeyState(VK_MENU) & KF_UP) modifiers.Add(InputModifier::Alt);
                if (GetKeyState(VK_LWIN) & KF_UP || GetKeyState(VK_RWIN) & KF_UP)
                    modifiers.Add(InputModifier::Super);
                if (GetKeyState(VK_CAPITAL) & 0x0001) modifiers.Add(InputModifier::CapsLock);
                if (GetKeyState(VK_NUMLOCK) & 0x0001) modifiers.Add(InputModifier::NumLock);

                evt.key.modifier = static_cast<InputModifier>(modifiers);

                WindowEvent ev{};
                new(&ev.key) KeyEvent{
                    .type = WindowEventType::Key,
                    .windowId = windowInfo->id,
                    .key = key,
                    .state = state,
                    .modifier = static_cast<InputModifier>(modifiers),
                };
                MANAGER_INSTANCE->pendingEvents.push_back(ev);
                // Now dispatch or store evt...
                // Example: processEvent(evt);
                return 0;
            }
        case WM_SIZE:
            {
                WindowEvent ev{};
                new(&ev.resize) ResizeEvent{
                    .type = WindowEventType::Resize,
                    .windowId = windowInfo->id,
                    .size = MANAGER_INSTANCE->GetClientSize(windowInfo->id)
                };
                if (!MANAGER_INSTANCE->pendingEvents.empty() && MANAGER_INSTANCE->pendingEvents.back().info.type ==
                    WindowEventType::Resize)
                {
                    MANAGER_INSTANCE->pendingEvents.back() = ev;
                }
                else
                {
                    MANAGER_INSTANCE->pendingEvents.push_back(ev);
                }
            }
            break;
        case WM_CLOSE:
            {
                WindowEvent ev{};
                new(&ev.close) CloseEvent{
                    .type = WindowEventType::Close,
                    .windowId = windowInfo->id,
                };
                MANAGER_INSTANCE->pendingEvents.push_back(ev);
                return 0;
            }
            break;
        }

        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }


    WindowsWindowManager::WindowsWindowManager()
    {
        OleInitialize(nullptr);
        INSTANCE = GetModuleHandle(nullptr);
        {
            WNDCLASS wc = {
                .lpfnWndProc = WindowProc,
                .hInstance = INSTANCE,
                .lpszClassName = DEFAULT_WINDOW_CLASS_NAME
            };

            wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
            RegisterClass(&wc);
        }
        MANAGER_INSTANCE = this;
    }

    WindowsWindowManager::~WindowsWindowManager()
    {
        OleUninitialize();
    }

    vk::SurfaceKHR WindowsWindowManager::CreateSurface(const std::uint64_t& id, const vk::Instance& instance)
    {
        if (const auto info = GetWindowInfo(id))
        {
            const auto createInfo = VkWin32SurfaceCreateInfoKHR{
                .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
                .pNext = nullptr,
                .flags = 0,
                .hinstance = INSTANCE,
                .hwnd = info->hwnd,
            };
            VkSurfaceKHR surface{};
            vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &surface);
            return surface;
        }
        return {};
    }

    std::uint64_t WindowsWindowManager::GetEvents(const std::span<WindowEvent>& events)
    {
        std::uint64_t gotten = 0;
        for (auto i = 0; i < events.size(); i++)
        {
            if (pendingEvents.empty())
            {
                break;
            }

            const auto event = pendingEvents.front();

            pendingEvents.pop_front();
            events[i] = event;
            gotten++;
        }
        return gotten;
    }

    std::uint64_t WindowsWindowManager::Create(const std::string_view& title, const Extent2D& size,
                                               const Flags<WindowFlags>& flags)
    {
        auto windowId = _idFactory.New();
        auto windowFlags = WS_SYSMENU;

        if (flags.Has(WindowFlags::Resizable))
        {
            windowFlags |= WS_SIZEBOX;
        }

        if (flags.Has(WindowFlags::Frameless))
        {
            windowFlags |= WS_POPUP;
        }
        else
        {
            windowFlags |= WS_OVERLAPPED | WS_CAPTION | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
        }

        const auto screenWidth = GetSystemMetrics(SM_CXSCREEN);
        const auto screenHeight = GetSystemMetrics(SM_CYSCREEN);
        const auto x = (screenWidth - size.width) / 2;
        const auto y = (screenHeight - size.height) / 2;
        const auto hwnd = CreateWindowEx(
            0,
            DEFAULT_WINDOW_CLASS_NAME,
            title.data(),
            windowFlags,
            x,
            y,
            size.width,
            size.height,
            nullptr,
            nullptr,
            INSTANCE,
            nullptr
        );

        if (flags.Has(WindowFlags::Focused))
        {
            SetFocus(hwnd);
        }
        {
            auto pref = DWMWCP_ROUND;
            DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &pref, sizeof(pref));
        }

        ShowWindow(hwnd, flags.Has(WindowFlags::Visible) ? SW_SHOW : SW_HIDE);

        IDropTarget * dropTarget = nullptr;
        if (flags.Has(WindowFlags::DragAndDrop))
        {
            dropTarget = new WindowDropTarget(windowId);
            RegisterDragDrop(hwnd, dropTarget);
        }

        _windows.emplace(windowId, WindowInfo{windowId, hwnd, false,dropTarget});
        _hwndToWindowId.emplace(hwnd, windowId);
        return windowId;
    }


    void WindowsWindowManager::Destroy(const std::uint64_t& id)
    {
        if (const auto info = _windows.find(id); info != _windows.end())
        {
            const auto hwnd = info->second.hwnd;
            if (info->second.dropTarget != nullptr)
            {
                RevokeDragDrop(hwnd);
                info->second.dropTarget->Release();
                info->second.dropTarget = nullptr;
            }
            DestroyWindow(hwnd);
            _hwndToWindowId.erase(hwnd);
            _windows.erase(info);
        }
    }

    Extent2D WindowsWindowManager::GetClientSize(const std::uint64_t& id)
    {
        if (const auto info = GetWindowInfo(id))
        {
            RECT rect;

            if (GetClientRect(info->hwnd, &rect))
            {
                const auto width = rect.right - rect.left;
                const auto height = rect.bottom - rect.top;
                return Extent2D{
                    .width = static_cast<uint32_t>(width),
                    .height = static_cast<uint32_t>(height)
                };
            }
        }

        return {};
    }

    Point2D WindowsWindowManager::GetClientPosition(const std::uint64_t& id)
    {
        if (const auto info = GetWindowInfo(id))
        {
            RECT rect;

            if (GetClientRect(info->hwnd, &rect))
            {
                return Point2D{
                    .x = static_cast<int>(rect.left),
                    .y = static_cast<int>(rect.top)
                };
            }
        }
        return Point2D{
            .x = 0,
            .y = 0
        };
    }

    Vector2 WindowsWindowManager::GetCursorPosition(const std::uint64_t& id)
    {
        if (const auto info = GetWindowInfo(id))
        {
            POINT pt;
            if (GetCursorPos(&pt))
            {
                // Convert from screen coordinates to client (drawable area) coordinates
                ScreenToClient(info->hwnd, &pt);
                return {
                    .x = static_cast<float>(pt.x),
                    .y = static_cast<float>(pt.y)
                };
            }
        }

        return {
            .x = 0,
            .y = 0
        };
    }

    void WindowsWindowManager::Show(const std::uint64_t& id)
    {
        if (const auto info = GetWindowInfo(id))
        {
            ShowWindow(info->hwnd, SW_SHOW);
        }
    }

    void WindowsWindowManager::Hide(const std::uint64_t& id)
    {
        if (const auto info = GetWindowInfo(id))
        {
            ShowWindow(info->hwnd, SW_HIDE);
        }
    }

    void WindowsWindowManager::PumpEvents()
    {
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE) > 0)
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    WindowInfo* WindowsWindowManager::GetWindowInfo(const std::uint64_t& id)
    {
        if (const auto found = _windows.find(id); found != _windows.end())
        {
            return &found->second;
        }
        return nullptr;
    }

    WindowInfo* WindowsWindowManager::GetWindowInfo(HWND hwnd)
    {
        if (const auto found = _hwndToWindowId.find(hwnd); found != _hwndToWindowId.end())
        {
            return GetWindowInfo(found->second);
        }

        return nullptr;
    }

    void WindowsWindowManager::GetRequiredExtensions(std::vector<const char*>& extensions)
    {
        extensions.emplace_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
        extensions.emplace_back(vk::KHRSurfaceExtensionName);
    }

    void WindowsWindowManager::SetHitTestCallback(const std::uint64_t& id,
                                                  const std::function<HitTestResult(const Vector2&)>& callback)
    {
        if (const auto info = GetWindowInfo(id))
        {
            info->hitTestFunction = callback;
        }
    }

    void WindowsWindowManager::ClearHitTestCallback(const std::uint64_t& id)
    {
        if (const auto info = GetWindowInfo(id))
        {
            info->hitTestFunction = {};
        }
    }

    void WindowsWindowManager::Minimize(const std::uint64_t& id)
    {
        if (const auto info = GetWindowInfo(id))
        {
            ShowWindow(info->hwnd, SW_MINIMIZE);
        }
    }

    void WindowsWindowManager::Maximize(const std::uint64_t& id)
    {
        if (const auto info = GetWindowInfo(id))
        {
            ShowWindow(info->hwnd, SW_MAXIMIZE);
        }
    }

    float WindowsWindowManager::GetDpi(const std::uint64_t& id)
    {
        if (const auto info = GetWindowInfo(id))
        {
            return static_cast<float>(GetDpiForWindow(info->hwnd));
        }
        return GetDefaultDpi();
    }

    float WindowsWindowManager::GetDefaultDpi()
    {
        return USER_DEFAULT_SCREEN_DPI;
    }

    void WindowsWindowManager::SetDropCallbacks(const std::uint64_t& id, const DropCallbacks& callbacks)
    {
        if (const auto info = GetWindowInfo(id))
        {
            info->dropCallbacks = callbacks;
        }
    }

    void WindowsWindowManager::ClearDropCallbacks(const std::uint64_t& id)
    {
        if (const auto info = GetWindowInfo(id))
        {
            info->dropCallbacks = {};
        }
    }
}
#endif
