#include "../macros_private.h"

#ifdef RWIN_PLATFORM_WIN
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00 // Windows 10
#endif

#ifndef NTDDI_VERSION
#define NTDDI_VERSION 0x0A000000
#endif

#include "../ScrollUnits.h"
#include "WindowsWindowManager.h"
#include <windows.h>
#include <shlobj.h>
#include <combaseapi.h>
#include <stringapiset.h>
#include <dwmapi.h>
#include <windowsx.h>
#include <imm.h>
#include <algorithm>
#include <memory>
#include <string>
#include <vector>
#include <vulkan/vulkan_win32.h>
#include <atlbase.h>
#include <atlcom.h>

#include "../DropDispatch.h"
#pragma comment (lib, "Dwmapi")
#pragma comment (lib, "imm32")
namespace rwin
{
    LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static HINSTANCE INSTANCE;
    const LPCWSTR DEFAULT_WINDOW_CLASS_NAME = L"Default Window Class";
    WindowsWindowManager* MANAGER_INSTANCE = nullptr;
    constexpr int KEY_DOWN_MASK = 0x8000;


    struct DataObjectView final : IDataView
    {
        explicit DataObjectView(IDataObject* dataObject) : _dataObject(dataObject)
        {
        }

        bool HasFiles() const override
        {
            return Has(CF_HDROP);
        }
        bool HasText() const override
        {
            return Has(CF_UNICODETEXT) || Has(CF_TEXT);
        }

        bool GetFiles(std::vector<std::filesystem::path>& paths) const override
        {
            if (!_dataObject) return false;

            FORMATETC formatEtc = { CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
            STGMEDIUM stgMedium;

            if (SUCCEEDED(_dataObject->GetData(&formatEtc, &stgMedium)))
            {
                bool success = false;
                if (const auto hDrop = static_cast<HDROP>(GlobalLock(stgMedium.hGlobal)))
                {
                    const auto fileCount = DragQueryFileW(hDrop, 0xFFFFFFFF, nullptr, 0);
                    for (UINT i = 0; i < fileCount; ++i)
                    {
                        const auto length = DragQueryFileW(hDrop, i, nullptr, 0);
                        std::wstring filePath(length + 1, L'\0');
                        if (DragQueryFileW(hDrop, i, filePath.data(), length + 1))
                        {
                            filePath.resize(length);
                            paths.emplace_back(filePath);
                        }
                    }
                    GlobalUnlock(stgMedium.hGlobal);
                    success = true;
                }

                ReleaseStgMedium(&stgMedium);
                return success;
            }

            return false;
        }

        bool GetText(std::u16string& out) const override
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
                if (const void* data = GlobalLock(stg.hGlobal))
                {
                    const auto bytes = GlobalSize(stg.hGlobal);
                    if (fmt.cfFormat == CF_UNICODETEXT)
                    {
                        static_assert(sizeof(wchar_t) == sizeof(char16_t));
                        const auto chars = static_cast<const char16_t*>(data);
                        out.assign(chars, std::find(chars, chars + bytes / sizeof(char16_t), u'\0'));
                    }
                    else // CF_TEXT
                    {
                        const auto chars = static_cast<const char*>(data);
                        const auto length = static_cast<int>(std::find(chars, chars + bytes, '\0') - chars);
                        const auto wideLength = MultiByteToWideChar(CP_ACP, 0, chars, length, nullptr, 0);
                        out.assign(static_cast<std::size_t>((std::max)(wideLength, 0)), u'\0');
                        if (wideLength > 0)
                        {
                            MultiByteToWideChar(CP_ACP, 0, chars, length, reinterpret_cast<wchar_t*>(out.data()),
                                                wideLength);
                        }
                    }

                    GlobalUnlock(stg.hGlobal);
                    success = true;
                }
            }

            ReleaseStgMedium(&stg);
            return success;
        }

    private:
        bool Has(const CLIPFORMAT format) const
        {
            FORMATETC formatEtc = { format, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
            return _dataObject && _dataObject->QueryGetData(&formatEtc) == S_OK;
        }

        CComPtr<IDataObject> _dataObject{};
    };

    static_assert(DROPEFFECT_COPY == static_cast<DWORD>(DropEffect::Copy) &&
        DROPEFFECT_MOVE == static_cast<DWORD>(DropEffect::Move) &&
        DROPEFFECT_LINK == static_cast<DWORD>(DropEffect::Link));

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
            return MANAGER_INSTANCE ? MANAGER_INSTANCE->GetWindowInfo(_windowId) : nullptr;
        }

        static const DropCallbacks* GetCallbacks(const WindowInfo* info)
        {
            return info->dropCallbacks.has_value() ? &*info->dropCallbacks : nullptr;
        }

        static DropInfo MakeDropInfo(const WindowInfo* info, const DWORD keyState, const POINTL pt, const DWORD allowed)
        {
            POINT point{pt.x,pt.y};
            ScreenToClient(info->hwnd,&point);

            Flags<InputModifier> modifiers{};
            if (keyState & MK_SHIFT) modifiers.Add(InputModifier::Shift);
            if (keyState & MK_CONTROL) modifiers.Add(InputModifier::Control);
            if (keyState & MK_ALT) modifiers.Add(InputModifier::Alt);

            return DropInfo{
                .position = Vector2{static_cast<float>(point.x), static_cast<float>(point.y)},
                .allowed = Flags<DropEffect>(static_cast<std::uint32_t>(
                    allowed & (DROPEFFECT_COPY | DROPEFFECT_MOVE | DROPEFFECT_LINK))),
                .modifiers = modifiers,
            };
        }

        HRESULT DragEnter(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) override
        {
            _data = std::make_unique<DataObjectView>(pDataObj);
            _enterEffect = DropEffect::None;
            if (const auto info = GetWindowInfo())
            {
                _enterEffect = DispatchDropEnter(GetCallbacks(info), MakeDropInfo(info, grfKeyState, pt, *pdwEffect),
                                                 *_data);
            }
            *pdwEffect = static_cast<DWORD>(_enterEffect);
            return S_OK;
        }
        HRESULT DragOver(DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) override
        {
            auto effect = DropEffect::None;
            if (const auto info = GetWindowInfo(); info && _data)
            {
                effect = DispatchDropOver(GetCallbacks(info), MakeDropInfo(info, grfKeyState, pt, *pdwEffect), *_data,
                                          _enterEffect);
            }
            *pdwEffect = static_cast<DWORD>(effect);
            return S_OK;
        }
        HRESULT DragLeave() override
        {
            if (const auto info = GetWindowInfo(); info && _data)
            {
                DispatchDropLeave(GetCallbacks(info));
            }
            _data.reset();
            return S_OK;
        }
        HRESULT Drop(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) override
        {
            auto effect = DropEffect::None;
            if (const auto info = GetWindowInfo(); info && _data)
            {
                effect = DispatchDrop(GetCallbacks(info), MakeDropInfo(info, grfKeyState, pt, *pdwEffect), *_data);
            }
            _data.reset();
            *pdwEffect = static_cast<DWORD>(effect);
            return S_OK;
        }

    private:
        ULONG _refCount;
        std::uint64_t _windowId;
        std::unique_ptr<DataObjectView> _data;
        DropEffect _enterEffect{DropEffect::None};
    };

    bool OpenClipboardWithRetry(const HWND owner)
    {
        // Another process may hold the clipboard open for a moment.
        for (int attempt = 0; attempt < 5; ++attempt)
        {
            if (OpenClipboard(owner))
            {
                return true;
            }
            Sleep(5);
        }
        return false;
    }

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

        default: return InputKey::Unknown;
        }
    }

    void ApplyCandidateRect(HWND hwnd, const Rect2D& caret)
    {
        const auto himc = ImmGetContext(hwnd);
        if (!himc)
        {
            return;
        }

        const RECT area{
            caret.offset.x,
            caret.offset.y,
            caret.offset.x + static_cast<LONG>(caret.extent.width),
            caret.offset.y + static_cast<LONG>(caret.extent.height),
        };
        CANDIDATEFORM candidate{};
        candidate.dwStyle = CFS_EXCLUDE;
        candidate.ptCurrentPos = POINT{area.left, area.bottom};
        candidate.rcArea = area;
        ImmSetCandidateWindow(himc, &candidate);
        ImmReleaseContext(hwnd, himc);
    }

    std::u16string ReadCompositionString(const HIMC himc, const DWORD index)
    {
        const LONG bytes = ImmGetCompositionStringW(himc, index, nullptr, 0);
        if (bytes <= 0)
        {
            return {};
        }
        std::u16string text(static_cast<std::size_t>(bytes) / sizeof(char16_t), u'\0');
        ImmGetCompositionStringW(himc, index, text.data(), static_cast<DWORD>(bytes));
        return text;
    }

    void EmitPreedit(WindowInfo& info, const std::u16string_view text, const std::uint32_t caretStart,
                     const std::uint32_t caretEnd)
    {
        info.preeditActive = !text.empty();
        WindowEvent ev{};
        new(&ev.textPreedit) TextPreeditEvent{
            .type = WindowEventType::TextPreedit,
            .windowId = info.id,
            .text = MANAGER_INSTANCE->textArena.Append(text),
            .caretStart = caretStart,
            .caretEnd = caretEnd,
        };
        MANAGER_INSTANCE->pendingEvents.push_back(ev);
    }

    LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        auto windowInfo = MANAGER_INSTANCE ? MANAGER_INSTANCE->GetWindowInfo(hwnd) : nullptr;
        if (windowInfo == nullptr) return DefWindowProcW(hwnd, uMsg, wParam, lParam);

        switch (uMsg)
        {
        // case WM_KEYUP:
        //     break;
        case WM_NCHITTEST:
            {
                LRESULT hit = DefWindowProcW(hwnd, uMsg, wParam, lParam);
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
                        return DefWindowProcW(hwnd, uMsg, wParam, lParam);
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
        case WM_MOUSEWHEEL:
        case WM_MOUSEHWHEEL:
            {
                POINT point{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
                ScreenToClient(hwnd, &point);

                const bool horizontal = uMsg == WM_MOUSEHWHEEL;
                const UINT systemLines = horizontal ? SPI_GETWHEELSCROLLCHARS : SPI_GETWHEELSCROLLLINES;
                UINT lines = static_cast<UINT>(SCROLL_DEFAULT_LINES_PER_NOTCH);
                SystemParametersInfoW(systemLines, 0, &lines, 0);
                if (lines == WHEEL_PAGESCROLL)
                {
                    lines = static_cast<UINT>(SCROLL_DEFAULT_LINES_PER_NOTCH);
                }

                const float dpiScale = static_cast<float>(GetDpiForWindow(hwnd)) / USER_DEFAULT_SCREEN_DPI;
                const float notches = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) / WHEEL_DELTA;
                const float pixels = notches * static_cast<float>(lines) * SCROLL_LINE_PIXELS * dpiScale;

                WindowEvent ev{};
                new(&ev.scroll) ScrollEvent{
                    .type = WindowEventType::Scroll,
                    .windowId = windowInfo->id,
                    .position = Vector2{static_cast<float>(point.x), static_cast<float>(point.y)},
                    // Positive x scrolls left, matching the other backends.
                    .delta = Vector2{horizontal ? -pixels : 0.0f, horizontal ? 0.0f : pixels},
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

                // Keep receiving the mouse while any button is held, so a release outside the window is seen.
                const auto buttonBit = 1u << static_cast<unsigned>(button);
                if (isDown)
                {
                    windowInfo->heldButtons |= buttonBit;
                    SetCapture(hwnd);
                }
                else
                {
                    windowInfo->heldButtons &= ~buttonBit;
                    if (windowInfo->heldButtons == 0)
                    {
                        ReleaseCapture();
                    }
                }

                // Modifier keys
                Flags<InputModifier> modifiers{};
                if (GetKeyState(VK_SHIFT) & KEY_DOWN_MASK) modifiers.Add(InputModifier::Shift);
                if (GetKeyState(VK_CONTROL) & KEY_DOWN_MASK) modifiers.Add(InputModifier::Control);
                if (GetKeyState(VK_MENU) & KEY_DOWN_MASK) modifiers.Add(InputModifier::Alt);
                if ((GetKeyState(VK_LWIN) & KEY_DOWN_MASK) || (GetKeyState(VK_RWIN) & KEY_DOWN_MASK))
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
        case WM_CANCELMODE:
            {
                if (GetCapture() == hwnd)
                {
                    ReleaseCapture();
                }
                break;
            }
        case WM_CAPTURECHANGED:
            {
                if (reinterpret_cast<HWND>(lParam) == hwnd)
                {
                    break;
                }

                // Capture was taken away, so the releases will never arrive; report them now.
                for (unsigned bit = 0; windowInfo->heldButtons != 0; ++bit)
                {
                    if ((windowInfo->heldButtons & (1u << bit)) == 0)
                    {
                        continue;
                    }
                    windowInfo->heldButtons &= ~(1u << bit);

                    WindowEvent ev{};
                    new(&ev.cursorButton) CursorButtonEvent{
                        .type = WindowEventType::CursorButton,
                        .windowId = windowInfo->id,
                        .button = static_cast<CursorButton>(bit),
                        .state = InputState::Released,
                        .modifier = static_cast<InputModifier>(0),
                    };
                    MANAGER_INSTANCE->pendingEvents.push_back(ev);
                }
                break;
            }
        case WM_IME_SETCONTEXT:
            {
                if (windowInfo->textInputActive)
                {
                    lParam &= ~static_cast<LPARAM>(ISC_SHOWUICOMPOSITIONWINDOW);
                }
                return DefWindowProcW(hwnd, uMsg, wParam, lParam);
            }
        case WM_IME_STARTCOMPOSITION:
            {
                if (!windowInfo->textInputActive)
                {
                    break;
                }
                ApplyCandidateRect(hwnd, windowInfo->textInputRect);
                return 0;
            }
        case WM_IME_COMPOSITION:
            {
                if (!windowInfo->textInputActive)
                {
                    break;
                }

                const auto himc = ImmGetContext(hwnd);
                if (!himc)
                {
                    break;
                }

                if (lParam & GCS_RESULTSTR)
                {
                    const auto result = ReadCompositionString(himc, GCS_RESULTSTR);
                    if (!result.empty())
                    {
                        WindowEvent ev{};
                        new(&ev.textCommit) TextCommitEvent{
                            .type = WindowEventType::TextCommit,
                            .windowId = windowInfo->id,
                            .text = MANAGER_INSTANCE->textArena.Append(result),
                        };
                        MANAGER_INSTANCE->pendingEvents.push_back(ev);
                    }
                }

                if (lParam & GCS_COMPSTR)
                {
                    const auto preedit = ReadCompositionString(himc, GCS_COMPSTR);
                    const auto length = static_cast<std::uint32_t>(preedit.size());
                    auto caretStart = length;
                    auto caretEnd = length;

                    if (lParam & GCS_CURSORPOS)
                    {
                        caretStart = caretEnd = (std::min)(
                            static_cast<std::uint32_t>(LOWORD(ImmGetCompositionStringW(himc, GCS_CURSORPOS, nullptr, 0))),
                            length);
                    }

                    // The clause being converted is reported as the selected range.
                    std::vector<BYTE> attributes(length);
                    if ((lParam & GCS_COMPATTR) && length > 0 &&
                        ImmGetCompositionStringW(himc, GCS_COMPATTR, attributes.data(), length) == static_cast<LONG>(length))
                    {
                        const auto isTarget = [](const BYTE attribute)
                        {
                            return attribute == ATTR_TARGET_CONVERTED || attribute == ATTR_TARGET_NOTCONVERTED;
                        };
                        const auto first = std::find_if(attributes.begin(), attributes.end(), isTarget);
                        if (first != attributes.end())
                        {
                            const auto last = std::find_if_not(first, attributes.end(), isTarget);
                            caretStart = static_cast<std::uint32_t>(first - attributes.begin());
                            caretEnd = static_cast<std::uint32_t>(last - attributes.begin());
                        }
                    }

                    EmitPreedit(*windowInfo, preedit, caretStart, caretEnd);
                }

                ImmReleaseContext(hwnd, himc);
                return 0;
            }
        case WM_IME_ENDCOMPOSITION:
            {
                if (!windowInfo->textInputActive)
                {
                    break;
                }
                if (windowInfo->preeditActive)
                {
                    EmitPreedit(*windowInfo, {}, 0, 0);
                }
                return 0;
            }
        case WM_SYSCHAR:
            {
                // Alt+Space still opens the system menu; any other Alt+key beeps in DefWindowProc.
                if (wParam == VK_SPACE) return DefWindowProcW(hwnd, uMsg, wParam, lParam);
                return 0;
            }
        case WM_CHAR:
            {
                if (!windowInfo->textInputActive)
                {
                    return 0;
                }

                // Backspace, Enter, Tab and Escape arrive as WM_CHAR too; those are keys, not text.
                const auto unit = static_cast<char16_t>(wParam);
                if (unit < 0x20 || unit == 0x7F)
                {
                    return 0;
                }

                // Non-BMP characters arrive as two WM_CHAR messages; emit them as one commit.
                if (IS_HIGH_SURROGATE(unit))
                {
                    windowInfo->pendingHighSurrogate = unit;
                    return 0;
                }

                char16_t units[2]{unit, 0};
                std::size_t count = 1;
                if (IS_LOW_SURROGATE(unit) && windowInfo->pendingHighSurrogate != 0)
                {
                    units[0] = windowInfo->pendingHighSurrogate;
                    units[1] = unit;
                    count = 2;
                }
                windowInfo->pendingHighSurrogate = 0;

                WindowEvent ev{};
                new(&ev.textCommit) TextCommitEvent{
                    .type = WindowEventType::TextCommit,
                    .windowId = windowInfo->id,
                    .text = MANAGER_INSTANCE->textArena.Append({units, count}),
                };
                MANAGER_INSTANCE->pendingEvents.push_back(ev);
                return 0;
            }
        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
            {
                const bool isSystemKey = uMsg == WM_SYSKEYDOWN || uMsg == WM_SYSKEYUP;
                const bool isKeyUp = uMsg == WM_KEYUP || uMsg == WM_SYSKEYUP;

                if (windowInfo->textInputActive && wParam == VK_PROCESSKEY)
                {
                    return 0;
                }

                WindowEvent evt{};

                // Fill basic event info

                // Convert virtual key to your InputKey enum
                const auto key = MapVirtualKeyToInputKey(static_cast<UINT>(wParam), lParam);

                auto keyFlags = HIWORD(lParam);
                // State (pressed / released / repeat)
                bool isRepeat = (keyFlags & KF_REPEAT) == KF_REPEAT;
                const auto state = isKeyUp
                                       ? InputState::Released
                                       : (isRepeat ? InputState::Repeat : InputState::Pressed);
                // Modifier bitmask
                Flags<InputModifier> modifiers{};
                if (GetKeyState(VK_SHIFT) & KEY_DOWN_MASK) modifiers.Add(InputModifier::Shift);
                if (GetKeyState(VK_CONTROL) & KEY_DOWN_MASK) modifiers.Add(InputModifier::Control);
                if (GetKeyState(VK_MENU) & KEY_DOWN_MASK) modifiers.Add(InputModifier::Alt);
                if (GetKeyState(VK_LWIN) & KEY_DOWN_MASK || GetKeyState(VK_RWIN) & KEY_DOWN_MASK)
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
                // Alt+F4 only closes the window if the default handler sees it.
                if (isSystemKey && wParam == VK_F4)
                {
                    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
                }
                return 0;
            }
        case WM_DPICHANGED:
            {
                const auto suggested = reinterpret_cast<const RECT*>(lParam);
                SetWindowPos(hwnd, nullptr, suggested->left, suggested->top, suggested->right - suggested->left,
                             suggested->bottom - suggested->top, SWP_NOZORDER | SWP_NOACTIVATE);
                return 0;
            }
        case WM_SETFOCUS:
        case WM_KILLFOCUS:
            {
                WindowEvent ev{};
                new(&ev.keyboardFocus) FocusEvent{
                    .type = WindowEventType::KeyboardFocus,
                    .windowId = windowInfo->id,
                    .focused = uMsg == WM_SETFOCUS ? 1 : 0,
                };
                MANAGER_INSTANCE->pendingEvents.push_back(ev);
            }
            break;
        case WM_SIZE:
            {
                if (wParam == SIZE_MINIMIZED)
                {
                    WindowEvent minimizeEv{};
                    new(&minimizeEv.minimize) MinimizeEvent{
                        .type = WindowEventType::Minimize,
                        .windowId = windowInfo->id,
                    };
                    MANAGER_INSTANCE->pendingEvents.push_back(minimizeEv);
                    break;
                }

                if (wParam == SIZE_MAXIMIZED && !windowInfo->maximized)
                {
                    WindowEvent maximizeEv{};
                    new(&maximizeEv.maximize) MaximizeEvent{
                        .type = WindowEventType::Maximize,
                        .windowId = windowInfo->id,
                    };
                    MANAGER_INSTANCE->pendingEvents.push_back(maximizeEv);
                }
                windowInfo->maximized = wParam == SIZE_MAXIMIZED;

                WindowEvent ev{};
                new(&ev.resize) ResizeEvent{
                    .type = WindowEventType::Resize,
                    .windowId = windowInfo->id,
                    .size = MANAGER_INSTANCE->GetClientSize(windowInfo->id)
                };
                if (!MANAGER_INSTANCE->pendingEvents.empty() &&
                    MANAGER_INSTANCE->pendingEvents.back().info.type == WindowEventType::Resize &&
                    MANAGER_INSTANCE->pendingEvents.back().info.windowId == windowInfo->id)
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

        return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }


    WindowsWindowManager::WindowsWindowManager()
    {
        OleInitialize(nullptr);
        SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        INSTANCE = GetModuleHandle(nullptr);
        {
            WNDCLASSW wc = {
                .lpfnWndProc = WindowProc,
                .hInstance = INSTANCE,
                .lpszClassName = DEFAULT_WINDOW_CLASS_NAME
            };

            wc.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
            RegisterClassW(&wc);
        }
        _clipboardOwner = CreateWindowExW(0, DEFAULT_WINDOW_CLASS_NAME, L"", 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr,
                                          INSTANCE, nullptr);
        MANAGER_INSTANCE = this;
    }

    WindowsWindowManager::~WindowsWindowManager()
    {
        std::vector<std::uint64_t> ids;
        ids.reserve(_windows.size());
        for (const auto& window : _windows)
        {
            ids.push_back(window.first);
        }
        for (const auto id : ids)
        {
            Destroy(id);
        }

        if (_clipboardOwner)
        {
            DestroyWindow(_clipboardOwner);
        }
        UnregisterClassW(DEFAULT_WINDOW_CLASS_NAME, INSTANCE);
        MANAGER_INSTANCE = nullptr;
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

        RECT frame{0, 0, static_cast<LONG>(size.width), static_cast<LONG>(size.height)};
        AdjustWindowRectExForDpi(&frame, windowFlags, FALSE, 0, GetDpiForSystem());
        const auto outerWidth = frame.right - frame.left;
        const auto outerHeight = frame.bottom - frame.top;

        const auto screenWidth = GetSystemMetrics(SM_CXSCREEN);
        const auto screenHeight = GetSystemMetrics(SM_CYSCREEN);
        const auto x = (screenWidth - outerWidth) / 2;
        const auto y = (screenHeight - outerHeight) / 2;
        const auto titleLength = MultiByteToWideChar(CP_UTF8, 0, title.data(), static_cast<int>(title.size()), nullptr, 0);
        std::wstring wideTitle(static_cast<std::size_t>((std::max)(titleLength, 0)), L'\0');
        if (titleLength > 0)
        {
            MultiByteToWideChar(CP_UTF8, 0, title.data(), static_cast<int>(title.size()), wideTitle.data(), titleLength);
        }

        const auto hwnd = CreateWindowExW(
            flags.Has(WindowFlags::Floating) ? WS_EX_TOPMOST : 0,
            DEFAULT_WINDOW_CLASS_NAME,
            wideTitle.c_str(),
            windowFlags,
            x,
            y,
            outerWidth,
            outerHeight,
            nullptr,
            nullptr,
            INSTANCE,
            nullptr
        );

        if (!hwnd)
        {
            return 0;
        }

        const auto windowId = _idFactory.New();
        {
            auto pref = DWMWCP_ROUND;
            DwmSetWindowAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, &pref, sizeof(pref));
        }

        if (flags.Has(WindowFlags::Transparent))
        {
            // An empty blur region makes DWM composite the window using its alpha channel.
            const auto region = CreateRectRgn(0, 0, -1, -1);
            const DWM_BLURBEHIND blur{
                .dwFlags = DWM_BB_ENABLE | DWM_BB_BLURREGION,
                .fEnable = TRUE,
                .hRgnBlur = region,
            };
            DwmEnableBlurBehindWindow(hwnd, &blur);
            DeleteObject(region);
        }

        ImmAssociateContext(hwnd, nullptr);

        // Registered before showing so the first focus and resize events are not dropped.
        _windows.emplace(windowId, WindowInfo{.id = windowId, .hwnd = hwnd});
        _hwndToWindowId.emplace(hwnd, windowId);

        if (flags.Has(WindowFlags::Visible))
        {
            ShowWindow(hwnd, SW_SHOW);
            if (flags.Has(WindowFlags::Focused))
            {
                SetFocus(hwnd);
            }
        }
        return windowId;
    }


    void WindowsWindowManager::Destroy(const std::uint64_t& requestedId)
    {
        const auto id = requestedId;
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
            std::erase_if(pendingEvents, [&id](const WindowEvent& event) { return event.info.windowId == id; });
            _idFactory.Free(id);
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
            POINT origin{0, 0};

            if (ClientToScreen(info->hwnd, &origin))
            {
                return Point2D{
                    .x = static_cast<int>(origin.x),
                    .y = static_cast<int>(origin.y)
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
        // Texts of events still queued must stay readable, so only reset once the queue is empty.
        if (pendingEvents.empty())
        {
            textArena.Reset();
        }

        MSG msg;
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE) > 0)
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    void WindowsWindowManager::StartTextInput(const std::uint64_t& id, const Rect2D& caret)
    {
        if (const auto info = GetWindowInfo(id))
        {
            info->textInputActive = true;
            info->textInputRect = caret;
            ImmAssociateContextEx(info->hwnd, nullptr, IACE_DEFAULT);
            ApplyCandidateRect(info->hwnd, caret);
        }
    }

    void WindowsWindowManager::StopTextInput(const std::uint64_t& id)
    {
        if (const auto info = GetWindowInfo(id))
        {
            if (const auto himc = ImmGetContext(info->hwnd))
            {
                ImmNotifyIME(himc, NI_COMPOSITIONSTR, CPS_CANCEL, 0);
                ImmReleaseContext(info->hwnd, himc);
            }
            ImmAssociateContext(info->hwnd, nullptr);

            info->textInputActive = false;
            info->preeditActive = false;
            info->pendingHighSurrogate = 0;
        }
    }

    void WindowsWindowManager::SetTextInputRect(const std::uint64_t& id, const Rect2D& caret)
    {
        if (const auto info = GetWindowInfo(id))
        {
            info->textInputRect = caret;
            if (info->textInputActive)
            {
                ApplyCandidateRect(info->hwnd, caret);
            }
        }
    }

    std::u16string_view WindowsWindowManager::GetEventText(const TextRef& ref)
    {
        return textArena.View(ref);
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

    void WindowsWindowManager::SetFullscreen(const std::uint64_t& id, const bool fullscreen)
    {
        const auto info = GetWindowInfo(id);
        if (!info || info->fullscreen == fullscreen)
        {
            return;
        }

        if (fullscreen)
        {
            info->windowedStyle = GetWindowLongPtrW(info->hwnd, GWL_STYLE);
            GetWindowRect(info->hwnd, &info->windowedRect);

            MONITORINFO monitor{.cbSize = sizeof(MONITORINFO)};
            GetMonitorInfoW(MonitorFromWindow(info->hwnd, MONITOR_DEFAULTTONEAREST), &monitor);
            const auto& area = monitor.rcMonitor;

            SetWindowLongPtrW(info->hwnd, GWL_STYLE, (info->windowedStyle & ~WS_OVERLAPPEDWINDOW) | WS_POPUP);
            SetWindowPos(info->hwnd, HWND_TOP, area.left, area.top, area.right - area.left, area.bottom - area.top,
                         SWP_FRAMECHANGED | SWP_NOOWNERZORDER);
        }
        else
        {
            const auto& rect = info->windowedRect;
            SetWindowLongPtrW(info->hwnd, GWL_STYLE, info->windowedStyle);
            SetWindowPos(info->hwnd, nullptr, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
                         SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOOWNERZORDER);
        }

        info->fullscreen = fullscreen;
    }

    void WindowsWindowManager::SetSize(const std::uint64_t& id, const Extent2D& size)
    {
        if (const auto info = GetWindowInfo(id))
        {
            const auto style = static_cast<DWORD>(GetWindowLongPtrW(info->hwnd, GWL_STYLE));
            const auto exStyle = static_cast<DWORD>(GetWindowLongPtrW(info->hwnd, GWL_EXSTYLE));
            RECT frame{0, 0, static_cast<LONG>(size.width), static_cast<LONG>(size.height)};
            AdjustWindowRectExForDpi(&frame, style, FALSE, exStyle, GetDpiForWindow(info->hwnd));
            SetWindowPos(info->hwnd, nullptr, 0, 0, frame.right - frame.left, frame.bottom - frame.top,
                         SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
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
            if (info->dropTarget == nullptr)
            {
                info->dropTarget = new WindowDropTarget(id);
                if (FAILED(RegisterDragDrop(info->hwnd, info->dropTarget)))
                {
                    info->dropTarget->Release();
                    info->dropTarget = nullptr;
                }
            }
        }
    }

    void WindowsWindowManager::ClearDropCallbacks(const std::uint64_t& id)
    {
        if (const auto info = GetWindowInfo(id))
        {
            info->dropCallbacks = {};
            if (info->dropTarget != nullptr)
            {
                RevokeDragDrop(info->hwnd);
                info->dropTarget->Release();
                info->dropTarget = nullptr;
            }
        }
    }

    bool WindowsWindowManager::HasClipboardText()
    {
        return IsClipboardFormatAvailable(CF_UNICODETEXT) != 0;
    }

    bool WindowsWindowManager::GetClipboardText(std::u16string& out)
    {
        CComPtr<IDataObject> dataObject;
        for (int attempt = 0; attempt < 5 && !dataObject; ++attempt)
        {
            if (FAILED(OleGetClipboard(&dataObject)))
            {
                Sleep(5);
            }
        }
        return dataObject && DataObjectView(dataObject).GetText(out);
    }

    bool WindowsWindowManager::SetClipboardText(const std::u16string_view text)
    {
        if (!OpenClipboardWithRetry(_clipboardOwner))
        {
            return false;
        }

        bool success = false;
        EmptyClipboard();
        if (const auto handle = GlobalAlloc(GMEM_MOVEABLE, (text.size() + 1) * sizeof(char16_t)))
        {
            if (const auto memory = static_cast<char16_t*>(GlobalLock(handle)))
            {
                std::copy(text.begin(), text.end(), memory);
                memory[text.size()] = u'\0';
                GlobalUnlock(handle);
                // The clipboard owns the handle once this succeeds.
                success = SetClipboardData(CF_UNICODETEXT, handle) != nullptr;
            }
            if (!success)
            {
                GlobalFree(handle);
            }
        }
        CloseClipboard();
        return success;
    }

    void WindowsWindowManager::ClearClipboard()
    {
        if (OpenClipboardWithRetry(_clipboardOwner))
        {
            EmptyClipboard();
            CloseClipboard();
        }
    }
}
#endif
