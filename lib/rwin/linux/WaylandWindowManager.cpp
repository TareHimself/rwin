#include "../macros_private.h"
#if defined(RWIN_PLATFORM_LINUX) && defined(RWIN_PLATFORM_LINUX_WAYLAND)
#include "WaylandWindowManager.h"
#include "../ScrollUnits.h"
#include "../DropDispatch.h"
#include "../Utf.h"

#include <algorithm>
#include <cerrno>
#include <fcntl.h>
#include <iostream>
#include <poll.h>
#include <span>
#include <string_view>
#include <ranges>
#include <stdexcept>
#include <unistd.h>
#include <wayland-client-protocol.h>
#include <xdg-shell-client-protocol.h>
#include <xdg-decoration-unstable-v1-client-protocol.h>
#include <vulkan/vulkan_wayland.h>
#include <sys/mman.h>
#include <linux/input-event-codes.h>

namespace rwin
{
#define UINT64_NULL_HANDLE std::numeric_limits<std::uint64_t>::max()

    InputKey xkbKeyToInputKey(const xkb_keysym_t key)
    {
        switch (key)
        {
        case XKB_KEY_A:
        case XKB_KEY_a: return InputKey::A;
        case XKB_KEY_B:
        case XKB_KEY_b: return InputKey::B;
        case XKB_KEY_C:
        case XKB_KEY_c: return InputKey::C;
        case XKB_KEY_D:
        case XKB_KEY_d: return InputKey::D;
        case XKB_KEY_E:
        case XKB_KEY_e: return InputKey::E;
        case XKB_KEY_F:
        case XKB_KEY_f: return InputKey::F;
        case XKB_KEY_G:
        case XKB_KEY_g: return InputKey::G;
        case XKB_KEY_H:
        case XKB_KEY_h: return InputKey::H;
        case XKB_KEY_I:
        case XKB_KEY_i: return InputKey::I;
        case XKB_KEY_J:
        case XKB_KEY_j: return InputKey::J;
        case XKB_KEY_K:
        case XKB_KEY_k: return InputKey::K;
        case XKB_KEY_L:
        case XKB_KEY_l: return InputKey::L;
        case XKB_KEY_M:
        case XKB_KEY_m: return InputKey::M;
        case XKB_KEY_N:
        case XKB_KEY_n: return InputKey::N;
        case XKB_KEY_O:
        case XKB_KEY_o: return InputKey::O;
        case XKB_KEY_P:
        case XKB_KEY_p: return InputKey::P;
        case XKB_KEY_Q:
        case XKB_KEY_q: return InputKey::Q;
        case XKB_KEY_R:
        case XKB_KEY_r: return InputKey::R;
        case XKB_KEY_S:
        case XKB_KEY_s: return InputKey::S;
        case XKB_KEY_T:
        case XKB_KEY_t: return InputKey::T;
        case XKB_KEY_U:
        case XKB_KEY_u: return InputKey::U;
        case XKB_KEY_V:
        case XKB_KEY_v: return InputKey::V;
        case XKB_KEY_W:
        case XKB_KEY_w: return InputKey::W;
        case XKB_KEY_X:
        case XKB_KEY_x: return InputKey::X;
        case XKB_KEY_Y:
        case XKB_KEY_y: return InputKey::Y;
        case XKB_KEY_Z:
        case XKB_KEY_z: return InputKey::Z;

        case XKB_KEY_0: return InputKey::Zero;
        case XKB_KEY_1: return InputKey::One;
        case XKB_KEY_2: return InputKey::Two;
        case XKB_KEY_3: return InputKey::Three;
        case XKB_KEY_4: return InputKey::Four;
        case XKB_KEY_5: return InputKey::Five;
        case XKB_KEY_6: return InputKey::Six;
        case XKB_KEY_7: return InputKey::Seven;
        case XKB_KEY_8: return InputKey::Eight;
        case XKB_KEY_9: return InputKey::Nine;

        case XKB_KEY_F1: return InputKey::F1;
        case XKB_KEY_F2: return InputKey::F2;
        case XKB_KEY_F3: return InputKey::F3;
        case XKB_KEY_F4: return InputKey::F4;
        case XKB_KEY_F5: return InputKey::F5;
        case XKB_KEY_F6: return InputKey::F6;
        case XKB_KEY_F7: return InputKey::F7;
        case XKB_KEY_F8: return InputKey::F8;
        case XKB_KEY_F9: return InputKey::F9;
        case XKB_KEY_F10: return InputKey::F10;
        case XKB_KEY_F11: return InputKey::F11;
        case XKB_KEY_F12: return InputKey::F12;
        case XKB_KEY_F13: return InputKey::F13;
        case XKB_KEY_F14: return InputKey::F14;
        case XKB_KEY_F15: return InputKey::F15;
        case XKB_KEY_F16: return InputKey::F16;
        case XKB_KEY_F17: return InputKey::F17;
        case XKB_KEY_F18: return InputKey::F18;
        case XKB_KEY_F19: return InputKey::F19;
        case XKB_KEY_F20: return InputKey::F20;
        case XKB_KEY_F21: return InputKey::F21;
        case XKB_KEY_F22: return InputKey::F22;
        case XKB_KEY_F23: return InputKey::F23;
        case XKB_KEY_F24: return InputKey::F24;

        case XKB_KEY_space: return InputKey::Space;
        case XKB_KEY_apostrophe: return InputKey::Apostrophe;
        case XKB_KEY_comma: return InputKey::Comma;
        case XKB_KEY_minus: return InputKey::Minus;
        case XKB_KEY_period: return InputKey::Period;
        case XKB_KEY_slash: return InputKey::Slash;
        case XKB_KEY_semicolon: return InputKey::Semicolon;
        case XKB_KEY_equal: return InputKey::Equal;
        case XKB_KEY_bracketleft: return InputKey::LeftBracket;
        case XKB_KEY_backslash: return InputKey::Backslash;
        case XKB_KEY_bracketright: return InputKey::RightBracket;
        case XKB_KEY_grave: return InputKey::GraveAccent;

        case XKB_KEY_Escape: return InputKey::Escape;
        case XKB_KEY_Return: return InputKey::Enter;
        case XKB_KEY_Tab: return InputKey::Tab;
        case XKB_KEY_BackSpace: return InputKey::Backspace;
        case XKB_KEY_Insert: return InputKey::Insert;
        case XKB_KEY_Delete: return InputKey::Delete;
        case XKB_KEY_Right: return InputKey::Right;
        case XKB_KEY_Left: return InputKey::Left;
        case XKB_KEY_Down: return InputKey::Down;
        case XKB_KEY_Up: return InputKey::Up;
        case XKB_KEY_Page_Up: return InputKey::PageUp;
        case XKB_KEY_Page_Down: return InputKey::PageDown;
        case XKB_KEY_Home: return InputKey::Home;
        case XKB_KEY_End: return InputKey::End;

        case XKB_KEY_Caps_Lock: return InputKey::CapsLock;
        case XKB_KEY_Scroll_Lock: return InputKey::ScrollLock;
        case XKB_KEY_Num_Lock: return InputKey::NumLock;
        case XKB_KEY_Print: return InputKey::PrintScreen;
        case XKB_KEY_Pause: return InputKey::Pause;

        case XKB_KEY_Shift_L: return InputKey::LeftShift;
        case XKB_KEY_Shift_R: return InputKey::RightShift;
        case XKB_KEY_Control_L: return InputKey::LeftControl;
        case XKB_KEY_Control_R: return InputKey::RightControl;
        case XKB_KEY_Alt_L: return InputKey::LeftAlt;
        case XKB_KEY_Alt_R: return InputKey::RightAlt;
        case XKB_KEY_Super_L: return InputKey::LeftSuper;
        case XKB_KEY_Super_R: return InputKey::RightSuper;
        case XKB_KEY_Menu: return InputKey::Menu;

        default:
            return InputKey::Unknown;
        }
    }

    namespace
    {
        constexpr const char* TEXT_MIME_TYPES[] = {
            "text/plain;charset=utf-8", "UTF8_STRING", "text/plain", "TEXT", "STRING"
        };
        constexpr const char* URI_LIST_MIME_TYPE = "text/uri-list";

        const DropCallbacks* GetDropCallbacks(const WindowInfo* info)
        {
            return info->dropCallbacks.has_value() ? &*info->dropCallbacks : nullptr;
        }

        const char* FindMimeType(const DataOffer& offer, const std::span<const char* const> wanted)
        {
            for (const auto mimeType : wanted)
            {
                if (std::find(offer.mimeTypes.begin(), offer.mimeTypes.end(), std::string_view{mimeType}) !=
                    offer.mimeTypes.end())
                {
                    return mimeType;
                }
            }
            return nullptr;
        }

        // The offering client writes into a pipe; give up if it stalls for a second.
        bool ReadOffer(wl_display* display, wl_data_offer* offer, const char* mimeType, std::string& out)
        {
            int fds[2];
            if (pipe2(fds, O_CLOEXEC) != 0)
            {
                return false;
            }
            wl_data_offer_receive(offer, mimeType, fds[1]);
            close(fds[1]);
            wl_display_flush(display);

            bool success = false;
            std::string data;
            char buffer[4096];
            while (true)
            {
                pollfd descriptor{fds[0], POLLIN, 0};
                const auto ready = poll(&descriptor, 1, 1000);
                if (ready < 0 && errno == EINTR)
                {
                    continue;
                }
                if (ready <= 0)
                {
                    break;
                }

                const auto count = read(fds[0], buffer, sizeof(buffer));
                if (count < 0 && errno == EINTR)
                {
                    continue;
                }
                if (count < 0)
                {
                    break;
                }
                if (count == 0)
                {
                    success = true;
                    break;
                }
                data.append(buffer, static_cast<std::size_t>(count));
            }
            close(fds[0]);

            if (success)
            {
                out = std::move(data);
            }
            return success;
        }

        std::string PercentDecode(const std::string_view text)
        {
            const auto hex = [](const char c)
            {
                if (c >= '0' && c <= '9') return c - '0';
                if (c >= 'a' && c <= 'f') return c - 'a' + 10;
                if (c >= 'A' && c <= 'F') return c - 'A' + 10;
                return -1;
            };

            std::string out;
            for (std::size_t i = 0; i < text.size(); ++i)
            {
                if (text[i] == '%' && i + 2 < text.size() && hex(text[i + 1]) >= 0 && hex(text[i + 2]) >= 0)
                {
                    out += static_cast<char>(hex(text[i + 1]) * 16 + hex(text[i + 2]));
                    i += 2;
                }
                else
                {
                    out += text[i];
                }
            }
            return out;
        }

        struct WaylandDataView final : IDataView
        {
            WaylandDataView(wl_display* display, const DataOffer* offer) : _display(display), _offer(offer)
            {
            }

            bool HasText() const override
            {
                return FindMimeType(*_offer, TEXT_MIME_TYPES) != nullptr;
            }

            bool HasFiles() const override
            {
                return FindMimeType(*_offer, {&URI_LIST_MIME_TYPE, 1}) != nullptr;
            }

            bool GetText(std::u16string& out) const override
            {
                const auto mimeType = FindMimeType(*_offer, TEXT_MIME_TYPES);
                std::string data;
                if (!mimeType || !ReadOffer(_display, _offer->offer, mimeType, data))
                {
                    return false;
                }
                out = Utf8ToUtf16(data);
                return true;
            }

            bool GetFiles(std::vector<std::filesystem::path>& out) const override
            {
                std::string data;
                if (!HasFiles() || !ReadOffer(_display, _offer->offer, URI_LIST_MIME_TYPE, data))
                {
                    return false;
                }

                bool any = false;
                for (std::size_t start = 0; start < data.size();)
                {
                    auto end = data.find('\n', start);
                    if (end == std::string::npos)
                    {
                        end = data.size();
                    }
                    std::string_view line{data.data() + start, end - start};
                    start = end + 1;

                    if (!line.empty() && line.back() == '\r')
                    {
                        line.remove_suffix(1);
                    }
                    if (!line.starts_with("file://"))
                    {
                        continue;
                    }
                    line.remove_prefix(7);

                    // Skips the optional host part of file://host/path.
                    const auto pathStart = line.find('/');
                    if (pathStart == std::string_view::npos)
                    {
                        continue;
                    }
                    out.emplace_back(PercentDecode(line.substr(pathStart)));
                    any = true;
                }
                return any;
            }

        private:
            wl_display* _display;
            const DataOffer* _offer;
        };
    }

    Flags<InputModifier> getInputModifiers(xkb_state* state)
    {
        Flags<InputModifier> modifiers{};
        if (xkb_state_mod_name_is_active(state, XKB_MOD_NAME_SHIFT, XKB_STATE_MODS_EFFECTIVE))
            modifiers |= InputModifier::Shift;
        if (xkb_state_mod_name_is_active(state, XKB_MOD_NAME_CTRL, XKB_STATE_MODS_EFFECTIVE))
            modifiers |= InputModifier::Control;
        if (xkb_state_mod_name_is_active(state, XKB_MOD_NAME_ALT, XKB_STATE_MODS_EFFECTIVE))
            modifiers |= InputModifier::Alt;
        if (xkb_state_mod_name_is_active(state, XKB_MOD_NAME_LOGO, XKB_STATE_MODS_EFFECTIVE))
            modifiers |= InputModifier::Super;
        if (xkb_state_led_name_is_active(state, XKB_LED_NAME_CAPS))
            modifiers |= InputModifier::CapsLock;
        if (xkb_state_led_name_is_active(state, XKB_LED_NAME_NUM))
            modifiers |= InputModifier::NumLock;

        return modifiers;
    }

    KeyboardInfo::KeyboardInfo(xkb_context* context, const char* keymapString, xkb_keymap_format format)
    {
        keymap = xkb_keymap_new_from_string(
            context, keymapString, format,
            XKB_KEYMAP_COMPILE_NO_FLAGS);
        state = keymap ? xkb_state_new(keymap) : nullptr;
    }

    KeyboardInfo::~KeyboardInfo()
    {
        if (state) xkb_state_unref(state);
        if (keymap) xkb_keymap_unref(keymap);
    }

    WaylandWindowManager::WaylandWindowManager()
    {
        _keyboardListener = {
            .keymap = [](void* data,
                         struct wl_keyboard* wl_keyboard,
                         uint32_t format,
                         int32_t fd,
                         uint32_t size)
            {
                const auto self = static_cast<WaylandWindowManager*>(data);
                self->_keyboards.erase(wl_keyboard);

                if (format == WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1)
                {
                    const auto keymapString = static_cast<char*>(mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0));
                    if (keymapString != MAP_FAILED)
                    {
                        const auto keyboard = std::make_shared<KeyboardInfo>(
                            self->_xkbContext, keymapString, XKB_KEYMAP_FORMAT_TEXT_V1);
                        munmap(keymapString, size);
                        if (keyboard->state)
                        {
                            self->_keyboards.emplace(wl_keyboard, keyboard);
                        }
                    }
                }
                close(fd);
            },
            .enter = [](void* data,
                        struct wl_keyboard* wl_keyboard,
                        uint32_t serial,
                        struct wl_surface* surface,
                        struct wl_array* keys)
            {
                if (const auto self = static_cast<WaylandWindowManager*>(data))
                {
                    if (const auto info = self->GetWindowInfo(surface))
                    {
                        self->_keyboardFocusedHandle = info->windowId;
                        if (const auto keyboard = self->_keyboards.find(wl_keyboard); keyboard != self->_keyboards.end())
                        {
                            keyboard->second->keysDown.clear();
                        }
                        WindowEvent ev{};
                        new(&ev.keyboardFocus) FocusEvent{
                            .type = WindowEventType::KeyboardFocus,
                            .windowId = info->windowId,
                            .focused = 1,
                        };
                        self->_pendingEvents.push_back(ev);
                    }
                }
            },
            .leave = [](void* data,
                        struct wl_keyboard* wl_keyboard,
                        uint32_t serial,
                        struct wl_surface* surface)
            {
                if (const auto self = static_cast<WaylandWindowManager*>(data))
                {
                    if (const auto info = self->GetWindowInfo(surface))
                    {
                        self->_keyboardFocusedHandle = UINT64_NULL_HANDLE;
                        self->_repeatKeycode = 0;
                        if (const auto keyboard = self->_keyboards.find(wl_keyboard); keyboard != self->_keyboards.end())
                        {
                            keyboard->second->keysDown.clear();
                        }
                        WindowEvent ev{};
                        new(&ev.keyboardFocus) FocusEvent{
                            .type = WindowEventType::KeyboardFocus,
                            .windowId = info->windowId,
                            .focused = 0
                        };
                        self->_pendingEvents.push_back(ev);
                    }
                }
            },
            .key = [](void* data,
                      struct wl_keyboard* wl_keyboard,
                      uint32_t serial,
                      uint32_t time,
                      uint32_t key,
                      uint32_t state)
            {
                const auto self = static_cast<WaylandWindowManager*>(data);
                self->_lastInputSerial = serial;
                if (self->_keyboardFocusedHandle == UINT64_NULL_HANDLE) return;

                const auto found = self->_keyboards.find(wl_keyboard);
                if (found == self->_keyboards.end()) return;
                const auto keyboard = found->second;

                const auto keycode = key + 8;
                if (state == WL_KEYBOARD_KEY_STATE_PRESSED)
                {
                    const bool alreadyDown = !keyboard->keysDown.insert(keycode).second;
                    self->HandleKey(*keyboard, keycode, alreadyDown ? InputState::Repeat : InputState::Pressed);

                    if (xkb_keymap_key_repeats(keyboard->keymap, keycode) && self->_repeatRate > 0)
                    {
                        self->_repeatKeycode = keycode;
                        self->_repeatNext = std::chrono::steady_clock::now() +
                            std::chrono::milliseconds(self->_repeatDelayMs);
                    }
                }
                else if (state == WL_KEYBOARD_KEY_STATE_RELEASED)
                {
                    keyboard->keysDown.erase(keycode);
                    if (self->_repeatKeycode == keycode)
                    {
                        self->_repeatKeycode = 0;
                    }
                    self->HandleKey(*keyboard, keycode, InputState::Released);
                }
            },
            .modifiers = [](void* data,
                            struct wl_keyboard* wl_keyboard,
                            uint32_t serial,
                            uint32_t mods_depressed,
                            uint32_t mods_latched,
                            uint32_t mods_locked,
                            uint32_t group)
            {
                const auto self = static_cast<WaylandWindowManager*>(data);
                if (const auto found = self->_keyboards.find(wl_keyboard); found != self->_keyboards.end())
                {
                    xkb_state_update_mask(found->second->state,
                                          mods_depressed, mods_latched, mods_locked, 0, 0, group);
                }
            },
            .repeat_info = [](void* data,
                              struct wl_keyboard* wl_keyboard,
                              int32_t rate,
                              int32_t delay)
            {
                const auto self = static_cast<WaylandWindowManager*>(data);
                self->_repeatRate = rate;
                self->_repeatDelayMs = delay;
            },
        };

        _pointerListener = {
            .enter = [](void* data,
                        struct wl_pointer* wl_pointer,
                        uint32_t serial,
                        struct wl_surface* surface,
                        wl_fixed_t surface_x,
                        wl_fixed_t surface_y)
            {
                if (const auto self = static_cast<WaylandWindowManager*>(data))
                {
                    if (const auto info = self->GetWindowInfo(surface))
                    {
                        self->_cursorFocusedHandle = info->windowId;
                        info->cursorPosition = {
                            static_cast<float>(wl_fixed_to_double(surface_x)) * static_cast<float>(info->scale),
                            static_cast<float>(wl_fixed_to_double(surface_y)) * static_cast<float>(info->scale),
                        };
                        self->ApplyCursor(serial, info->scale);

                        WindowEvent ev{};
                        new(&ev.cursorFocus) FocusEvent{
                            .type = WindowEventType::CursorFocus,
                            .windowId = info->windowId,
                            .focused = 1,
                        };
                        self->_pendingEvents.push_back(ev);

                        WindowEvent moveEv{};
                        new(&moveEv.cursorMove) CursorMoveEvent{
                            .type = WindowEventType::CursorMove,
                            .windowId = info->windowId,
                            .position = info->cursorPosition,
                        };
                        self->_pendingEvents.push_back(moveEv);
                    }
                }
            },
            .leave = [](void* data,
                        struct wl_pointer* wl_pointer,
                        uint32_t serial,
                        struct wl_surface* surface)
            {
                if (const auto self = static_cast<WaylandWindowManager*>(data))
                {
                    if (const auto info = self->GetWindowInfo(surface))
                    {
                        self->_cursorFocusedHandle = UINT64_NULL_HANDLE;
                        WindowEvent ev{};
                        new(&ev.cursorFocus) FocusEvent{
                            .type = WindowEventType::CursorFocus,
                            .windowId = info->windowId,
                            .focused = 0,
                        };
                        self->_pendingEvents.push_back(ev);
                    }
                }
            },
            .motion = [](void* data,
                         struct wl_pointer* wl_pointer,
                         uint32_t time,
                         wl_fixed_t surface_x,
                         wl_fixed_t surface_y)
            {
                if (const auto self = static_cast<WaylandWindowManager*>(data))
                {
                    if (const auto info = self->GetWindowInfo(self->_cursorFocusedHandle))
                    {
                        const auto x = static_cast<float>(wl_fixed_to_double(surface_x)) * static_cast<float>(info->scale);
                        const auto y = static_cast<float>(wl_fixed_to_double(surface_y)) * static_cast<float>(info->scale);
                        WindowEvent ev{};
                        info->cursorPosition = {x, y};

                        new(&ev.cursorMove) CursorMoveEvent{
                            .type = WindowEventType::CursorMove,
                            .windowId = info->windowId,
                            .position = info->cursorPosition,
                        };
                        self->_pendingEvents.push_back(ev);
                    }
                }
            },
            .button = [](void* data,
                         struct wl_pointer* wl_pointer,
                         uint32_t serial,
                         uint32_t time,
                         uint32_t button,
                         uint32_t state)
            {
                if (const auto self = static_cast<WaylandWindowManager*>(data))
                {
                    self->_lastInputSerial = serial;
                    if (const auto info = self->GetWindowInfo(self->_cursorFocusedHandle))
                    {
                        WindowEvent ev{};
                        CursorButton btn;
                        switch (button)
                        {
                        case BTN_LEFT: btn = CursorButton::One;
                            break;
                        case BTN_RIGHT: btn = CursorButton::Two;
                            break;
                        case BTN_MIDDLE: btn = CursorButton::Three;
                            break;
                        case BTN_SIDE: btn = CursorButton::Four;
                            break;
                        case BTN_EXTRA: btn = CursorButton::Five;
                            break;
                        case BTN_FORWARD: btn = CursorButton::Six;
                            break;
                        case BTN_BACK: btn = CursorButton::Seven;
                            break;
                        default: return; // unknown button
                        }

                        const InputState btnState = (state == WL_POINTER_BUTTON_STATE_PRESSED)
                                                        ? InputState::Pressed
                                                        : InputState::Released;
                        new(&ev.cursorButton) CursorButtonEvent{
                            .type = WindowEventType::CursorButton,
                            .windowId = info->windowId,
                            .button = btn,
                            .state = btnState,
                            .modifier = static_cast<InputModifier>(self->CurrentModifiers()),
                        };
                        self->_pendingEvents.push_back(ev);
                    }
                }
            },
            .axis = [](void* data,
                       struct wl_pointer* wl_pointer,
                       uint32_t time,
                       uint32_t axis,
                       wl_fixed_t value)
            {
                if (const auto self = static_cast<WaylandWindowManager*>(data))
                {
                    const auto amount = static_cast<float>(wl_fixed_to_double(value));
                    if (axis == WL_POINTER_AXIS_HORIZONTAL_SCROLL)
                    {
                        self->_scrollContinuous.x += amount;
                    }
                    else
                    {
                        self->_scrollContinuous.y += amount;
                    }

                    if (!self->_pointerHasFrame)
                    {
                        self->FlushScroll();
                    }
                }
            },
            .frame = [](void* data,
                        struct wl_pointer* wl_pointer)
            {
                if (const auto self = static_cast<WaylandWindowManager*>(data))
                {
                    self->FlushScroll();
                }
            },
            .axis_source = [](void* data,
                              struct wl_pointer* wl_pointer,
                              uint32_t axis_source)
            {
            },
            .axis_stop = [](void* data,
                            struct wl_pointer* wl_pointer,
                            uint32_t time,
                            uint32_t axis)
            {
            },
            .axis_discrete = [](void* data,
                                struct wl_pointer* wl_pointer,
                                uint32_t axis,
                                int32_t discrete)
            {
                if (const auto self = static_cast<WaylandWindowManager*>(data))
                {
                    if (wl_pointer_get_version(wl_pointer) >= WL_POINTER_AXIS_VALUE120_SINCE_VERSION)
                    {
                        return;
                    }
                    auto& target = axis == WL_POINTER_AXIS_HORIZONTAL_SCROLL
                                       ? self->_scrollDiscrete120.x
                                       : self->_scrollDiscrete120.y;
                    target += static_cast<float>(discrete) * 120.0f;
                }
            },
            .axis_value120 = [](void* data,
                                struct wl_pointer* wl_pointer,
                                uint32_t axis,
                                int32_t value120)
            {
                if (const auto self = static_cast<WaylandWindowManager*>(data))
                {
                    auto& target = axis == WL_POINTER_AXIS_HORIZONTAL_SCROLL
                                       ? self->_scrollDiscrete120.x
                                       : self->_scrollDiscrete120.y;
                    target += static_cast<float>(value120);
                }
            },
            .axis_relative_direction = [](void* data,
                                          struct wl_pointer* wl_pointer,
                                          uint32_t axis,
                                          uint32_t direction)
            {
            },
        };

        _seatListener = {
            .capabilities = [](void* data,
                               struct wl_seat* wl_seat,
                               uint32_t capabilities)
            {
                auto self = static_cast<WaylandWindowManager*>(data);

                const bool hasKeyboard = (capabilities & WL_SEAT_CAPABILITY_KEYBOARD) != 0;
                if (hasKeyboard && !self->_keyboard)
                {
                    self->_keyboard = wl_seat_get_keyboard(self->_seat);
                    wl_keyboard_add_listener(self->_keyboard, &self->_keyboardListener, self);
                }
                else if (!hasKeyboard && self->_keyboard)
                {
                    self->_keyboards.erase(self->_keyboard);
                    wl_keyboard_release(self->_keyboard);
                    self->_keyboard = nullptr;
                    self->_keyboardFocusedHandle = UINT64_NULL_HANDLE;
                    self->_repeatKeycode = 0;
                }

                const bool hasPointer = (capabilities & WL_SEAT_CAPABILITY_POINTER) != 0;
                if (hasPointer && !self->_pointer)
                {
                    self->_pointer = wl_seat_get_pointer(self->_seat);
                    self->_pointerHasFrame = wl_pointer_get_version(self->_pointer) >= WL_POINTER_FRAME_SINCE_VERSION;
                    wl_pointer_add_listener(self->_pointer, &self->_pointerListener, self);
                }
                else if (!hasPointer && self->_pointer)
                {
                    wl_pointer_release(self->_pointer);
                    self->_pointer = nullptr;
                    self->_cursorFocusedHandle = UINT64_NULL_HANDLE;
                }
            },
            .name = [](void* data,
                       struct wl_seat* wl_seat,
                       const char* name)
            {
            }
        };

        _registryListener = {
            .global = [](
            void* data,
            struct wl_registry* registry,
            uint32_t name,
            const char* interface,
            uint32_t version
        )
            {
                auto self = static_cast<WaylandWindowManager*>(data);
                auto interfaceName = std::string{interface};

                if (interfaceName == wl_compositor_interface.name)
                {
                    const auto bindVersion = std::min<uint32_t>(version, wl_compositor_interface.version);
                    self->_compositor = static_cast<wl_compositor*>(wl_registry_bind(
                        registry, name, &wl_compositor_interface,
                        bindVersion));
                    self->_hasPreferredBufferScale = bindVersion >= WL_SURFACE_PREFERRED_BUFFER_SCALE_SINCE_VERSION;
                }
                else if (interfaceName == wl_output_interface.name)
                {
                    const auto bindVersion = std::min<uint32_t>(version, 3);
                    const auto output = static_cast<wl_output*>(wl_registry_bind(
                        registry, name, &wl_output_interface, bindVersion));
                    self->_outputScales[output] = 1;
                    wl_output_add_listener(output, &self->_outputListener, self);
                }
                else if (interfaceName == wl_shm_interface.name)
                {
                    self->_shm = static_cast<wl_shm*>(wl_registry_bind(registry, name, &wl_shm_interface, 1));
                }
                else if (interfaceName == wl_seat_interface.name)
                {
                    const auto bindVersion = std::min<uint32_t>(version, wl_seat_interface.version);
                    self->_seat = static_cast<wl_seat*>(wl_registry_bind(
                        registry, name, &wl_seat_interface, bindVersion));
                    wl_seat_add_listener(self->_seat, &self->_seatListener, self);
                    self->CreateDataDevice();
                }
                else if (interfaceName == wl_data_device_manager_interface.name)
                {
                    self->_dataDeviceManager = static_cast<wl_data_device_manager*>(wl_registry_bind(
                        registry, name, &wl_data_device_manager_interface, std::min<uint32_t>(version, 3)));
                    self->CreateDataDevice();
                }
            },
            .global_remove = [](void* data,
                                struct wl_registry* registry,
                                uint32_t name)
            {
            },
        };

        _outputListener = {
            .geometry = [](void* data, struct wl_output* output, int32_t x, int32_t y, int32_t physicalWidth,
                           int32_t physicalHeight, int32_t subpixel, const char* make, const char* model,
                           int32_t transform)
            {
            },
            .mode = [](void* data, struct wl_output* output, uint32_t flags, int32_t width, int32_t height,
                       int32_t refresh)
            {
            },
            .done = [](void* data, struct wl_output* output)
            {
            },
            .scale = [](void* data, struct wl_output* output, int32_t factor)
            {
                if (const auto self = static_cast<WaylandWindowManager*>(data))
                {
                    self->_outputScales[output] = std::max(factor, 1);
                    for (const auto& window : self->_windows)
                    {
                        if (window.second->outputs.contains(output))
                        {
                            self->UpdateScaleFromOutputs(window.second.get());
                        }
                    }
                }
            },
            .name = [](void* data, struct wl_output* output, const char* name)
            {
            },
            .description = [](void* data, struct wl_output* output, const char* description)
            {
            },
        };

        _surfaceListener = {
            .enter = [](void* data, struct wl_surface* surface, struct wl_output* output)
            {
                if (const auto self = static_cast<WaylandWindowManager*>(data))
                {
                    if (const auto info = self->GetWindowInfo(surface))
                    {
                        info->outputs.insert(output);
                        self->UpdateScaleFromOutputs(info);
                    }
                }
            },
            .leave = [](void* data, struct wl_surface* surface, struct wl_output* output)
            {
                if (const auto self = static_cast<WaylandWindowManager*>(data))
                {
                    if (const auto info = self->GetWindowInfo(surface))
                    {
                        info->outputs.erase(output);
                        self->UpdateScaleFromOutputs(info);
                    }
                }
            },
            .preferred_buffer_scale = [](void* data, struct wl_surface* surface, int32_t factor)
            {
                if (const auto self = static_cast<WaylandWindowManager*>(data))
                {
                    if (const auto info = self->GetWindowInfo(surface))
                    {
                        self->ApplyScale(info, factor);
                    }
                }
            },
            .preferred_buffer_transform = [](void* data, struct wl_surface* surface, uint32_t transform)
            {
            },
        };

        _frameInterface = {
            .configure = [](struct libdecor_frame* frame,
                            struct libdecor_configuration* configuration,
                            void* user_data)
            {
                if (auto info = static_cast<WindowInfo*>(user_data))
                {
                    int width, height;
                    if (!(libdecor_configuration_get_content_size(configuration, frame, &width, &height) && width != 0 && height != 0))
                    {
                        width = static_cast<int>(info->size.width);
                        height = static_cast<int>(info->size.height);
                    }

                    auto newExtent = Extent2D{static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

                    libdecor_window_state windowState{};
                    if (libdecor_configuration_get_window_state(configuration, &windowState))
                    {
                        const bool maximized = (windowState & LIBDECOR_WINDOW_STATE_MAXIMIZED) != 0;
                        if (maximized && !info->maximized)
                        {
                            WindowEvent maximizeEv{};
                            new(&maximizeEv.maximize) MaximizeEvent{
                                .type = WindowEventType::Maximize,
                                .windowId = info->windowId,
                            };
                            info->windowManager->_pendingEvents.push_back(maximizeEv);
                        }
                        info->maximized = maximized;
                    }

                    auto state = libdecor_state_new(width, height);
                    libdecor_frame_commit(frame, state, configuration);
                    libdecor_state_free(state);

                    if (newExtent != info->size)
                    {
                        info->size = newExtent;
                        WindowEvent ev{};
                        new(&ev.resize) ResizeEvent{
                            .type = WindowEventType::Resize,
                            .windowId = info->windowId,
                            .size = info->windowManager->GetPhysicalSize(info),
                        };
                        info->windowManager->_pendingEvents.push_back(ev);
                    }
                }
            },
            .close = [](struct libdecor_frame* frame, void* user_data)
            {
                if (const auto info = static_cast<WindowInfo*>(user_data))
                {
                    WindowEvent ev{};
                    new(&ev.close) CloseEvent{
                        .type = WindowEventType::Close,
                        .windowId = info->windowId,
                    };
                    info->windowManager->_pendingEvents.push_back(ev);
                }
            },
            .commit = [](struct libdecor_frame* frame, void* user_data)
            {
                if (const auto info = static_cast<WindowInfo*>(user_data))
                {
                    wl_surface_commit(info->surface);
                }
            }
        };


        _decorInterface = {
            .error = [](struct libdecor* context,
                        enum libdecor_error error,
                        const char* message)
            {
                std::cerr << "Caught libdecor error (" << error << "): " << message << std::endl;
                exit(EXIT_FAILURE);
            }
        };

        _dataOfferListener = {
            .offer = [](void* data, struct wl_data_offer* offer, const char* mimeType)
            {
                const auto self = static_cast<WaylandWindowManager*>(data);
                if (const auto found = self->FindDataOffer(offer))
                {
                    found->mimeTypes.emplace_back(mimeType);
                }
            },
            .source_actions = [](void* data, struct wl_data_offer* offer, uint32_t sourceActions)
            {
                const auto self = static_cast<WaylandWindowManager*>(data);
                if (const auto found = self->FindDataOffer(offer))
                {
                    found->sourceActions = sourceActions;
                }
            },
            .action = [](void* data, struct wl_data_offer* offer, uint32_t dndAction)
            {
                const auto self = static_cast<WaylandWindowManager*>(data);
                if (const auto found = self->FindDataOffer(offer))
                {
                    found->action = dndAction;
                }
            },
        };

        _dataDeviceListener = {
            .data_offer = [](void* data, struct wl_data_device* device, struct wl_data_offer* offer)
            {
                const auto self = static_cast<WaylandWindowManager*>(data);
                auto dataOffer = std::make_unique<DataOffer>();
                dataOffer->offer = offer;
                self->_dataOffers.insert_or_assign(offer, std::move(dataOffer));
                wl_data_offer_add_listener(offer, &self->_dataOfferListener, self);
            },
            .enter = [](void* data, struct wl_data_device* device, uint32_t serial, struct wl_surface* surface,
                        wl_fixed_t x, wl_fixed_t y, struct wl_data_offer* offer)
            {
                const auto self = static_cast<WaylandWindowManager*>(data);
                self->_dragOffer = offer;
                self->_dragSerial = serial;
                self->_dragWindow = UINT64_NULL_HANDLE;
                self->_dragEnterEffect = DropEffect::None;

                const auto info = self->GetWindowInfo(surface);
                const auto dataOffer = self->FindDataOffer(offer);
                if (!info || !dataOffer)
                {
                    return;
                }

                self->_dragWindow = info->windowId;
                self->_dragPosition = {
                    static_cast<float>(wl_fixed_to_double(x)) * static_cast<float>(info->scale),
                    static_cast<float>(wl_fixed_to_double(y)) * static_cast<float>(info->scale),
                };
                self->_dragEnterEffect = DispatchDropEnter(GetDropCallbacks(info), self->MakeDropInfo(*dataOffer),
                                                           WaylandDataView{self->_display, dataOffer});
                self->ApplyDragEffect(self->_dragEnterEffect);
            },
            .leave = [](void* data, struct wl_data_device* device)
            {
                const auto self = static_cast<WaylandWindowManager*>(data);
                if (const auto info = self->GetWindowInfo(self->_dragWindow))
                {
                    DispatchDropLeave(GetDropCallbacks(info));
                }
                self->EndDrag();
            },
            .motion = [](void* data, struct wl_data_device* device, uint32_t time, wl_fixed_t x, wl_fixed_t y)
            {
                const auto self = static_cast<WaylandWindowManager*>(data);
                const auto info = self->GetWindowInfo(self->_dragWindow);
                const auto dataOffer = self->FindDataOffer(self->_dragOffer);
                if (!info || !dataOffer)
                {
                    return;
                }

                self->_dragPosition = {
                    static_cast<float>(wl_fixed_to_double(x)) * static_cast<float>(info->scale),
                    static_cast<float>(wl_fixed_to_double(y)) * static_cast<float>(info->scale),
                };
                const auto effect = DispatchDropOver(GetDropCallbacks(info), self->MakeDropInfo(*dataOffer),
                                                     WaylandDataView{self->_display, dataOffer},
                                                     self->_dragEnterEffect);
                if (effect != self->_dragEffect)
                {
                    self->ApplyDragEffect(effect);
                }
            },
            .drop = [](void* data, struct wl_data_device* device)
            {
                const auto self = static_cast<WaylandWindowManager*>(data);
                const auto info = self->GetWindowInfo(self->_dragWindow);
                const auto dataOffer = self->FindDataOffer(self->_dragOffer);
                auto effect = DropEffect::None;
                if (info && dataOffer)
                {
                    effect = DispatchDrop(GetDropCallbacks(info), self->MakeDropInfo(*dataOffer),
                                          WaylandDataView{self->_display, dataOffer});
                }

                // finish is a protocol error unless a type was accepted and an action was agreed.
                if (dataOffer && effect != DropEffect::None && self->_dragAccepted && dataOffer->action != 0 &&
                    wl_data_offer_get_version(dataOffer->offer) >= WL_DATA_OFFER_FINISH_SINCE_VERSION)
                {
                    wl_data_offer_finish(dataOffer->offer);
                }
                self->EndDrag();
            },
            .selection = [](void* data, struct wl_data_device* device, struct wl_data_offer* offer)
            {
                const auto self = static_cast<WaylandWindowManager*>(data);
                if (self->_selectionOffer && self->_selectionOffer != offer)
                {
                    self->DestroyDataOffer(self->_selectionOffer);
                }
                self->_selectionOffer = offer;
            },
        };

        _dataSourceListener = {
            .target = [](void* data, struct wl_data_source* source, const char* mimeType)
            {
            },
            .send = [](void* data, struct wl_data_source* source, const char* mimeType, int32_t fd)
            {
                const auto self = static_cast<WaylandWindowManager*>(data);
                if (source == self->_clipboardSource)
                {
                    std::string_view remaining{self->_clipboardText};
                    while (!remaining.empty())
                    {
                        const auto written = write(fd, remaining.data(), remaining.size());
                        if (written < 0 && errno == EINTR)
                        {
                            continue;
                        }
                        if (written <= 0)
                        {
                            break;
                        }
                        remaining.remove_prefix(static_cast<std::size_t>(written));
                    }
                }
                close(fd);
            },
            .cancelled = [](void* data, struct wl_data_source* source)
            {
                const auto self = static_cast<WaylandWindowManager*>(data);
                wl_data_source_destroy(source);
                if (source == self->_clipboardSource)
                {
                    self->_clipboardSource = nullptr;
                    self->_clipboardText.clear();
                }
            },
            .dnd_drop_performed = [](void* data, struct wl_data_source* source)
            {
            },
            .dnd_finished = [](void* data, struct wl_data_source* source)
            {
            },
            .action = [](void* data, struct wl_data_source* source, uint32_t dndAction)
            {
            },
        };

        _xkbContext = xkb_context_new(static_cast<xkb_context_flags>(0));
        _display = wl_display_connect(nullptr);
        if (!_display)
        {
            throw std::runtime_error("rwin: could not connect to the Wayland display");
        }
        _registry = wl_display_get_registry(_display);
        wl_registry_add_listener(_registry, &_registryListener, this);
        wl_display_roundtrip(_display);
        wl_display_roundtrip(_display);
        if (!_compositor)
        {
            throw std::runtime_error("rwin: the Wayland compositor does not provide wl_compositor");
        }
        _decorContext = libdecor_new(_display, &_decorInterface);

        if (_shm)
        {
            _cursorSurface = wl_compositor_create_surface(_compositor);
        }
    }

    WaylandWindowManager::~WaylandWindowManager()
    {
        std::vector<std::uint64_t> ids;
        for (const auto& window : _windows)
        {
            ids.push_back(window.first);
        }
        for (const auto id : ids)
        {
            Destroy(id);
        }

        for (const auto& offer : _dataOffers)
        {
            wl_data_offer_destroy(offer.first);
        }
        _dataOffers.clear();
        if (_clipboardSource) wl_data_source_destroy(_clipboardSource);
        if (_dataDevice) wl_data_device_destroy(_dataDevice);
        if (_dataDeviceManager) wl_data_device_manager_destroy(_dataDeviceManager);
        if (_keyboard) wl_keyboard_release(_keyboard);
        if (_pointer) wl_pointer_release(_pointer);
        if (_seat) wl_seat_release(_seat);
        for (const auto& output : _outputScales)
        {
            wl_output_release(output.first);
        }
        if (_cursorSurface) wl_surface_destroy(_cursorSurface);
        if (_cursorTheme) wl_cursor_theme_destroy(_cursorTheme);
        if (_shm) wl_shm_destroy(_shm);
        if (_decorContext) libdecor_unref(_decorContext);
        if (_compositor) wl_compositor_destroy(_compositor);
        if (_registry) wl_registry_destroy(_registry);
        if (_display) wl_display_disconnect(_display);
        _keyboards.clear();
        if (_xkbContext) xkb_context_unref(_xkbContext);
    }

    vk::SurfaceKHR WaylandWindowManager::CreateSurface(const std::uint64_t& id, const vk::Instance& instance)
    {
        if (const auto info = GetWindowInfo(id))
        {
            const VkWaylandSurfaceCreateInfoKHR createInfo{
                .sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
                .display = _display,
                .surface = info->surface
            };

            VkSurfaceKHR surface{};
            vkCreateWaylandSurfaceKHR(instance, &createInfo, nullptr, &surface);
            return surface;
        }

        return {};
    }

    std::uint64_t WaylandWindowManager::GetEvents(const std::span<WindowEvent>& events)
    {
        int count = 0;
        for (auto& event : events)
        {
            if (_pendingEvents.empty())
            {
                break;
            }

            event = _pendingEvents.front();
            _pendingEvents.pop_front();
            count++;
        }
        return count;
    }

    std::uint64_t WaylandWindowManager::Create(const std::string_view& title, const Extent2D& size,
                                               const Flags<WindowFlags>& flags)
    {
        const auto windowId = _idFactory.New();
        auto windowInfo = std::make_shared<WindowInfo>();
        windowInfo->windowId = windowId;
        const auto surface = wl_compositor_create_surface(_compositor);
        const auto frame = libdecor_decorate(_decorContext, surface, &_frameInterface, windowInfo.get());
        _windows.insert_or_assign(windowId, windowInfo);
        windowInfo->windowManager = this;
        windowInfo->surface = surface;
        _surfaceToWindows.insert_or_assign(surface, windowInfo);
        wl_surface_add_listener(surface, &_surfaceListener, this);
        windowInfo->flags = flags;
        windowInfo->size = size;
        windowInfo->frame = frame;

        const std::string titleString{title};
        libdecor_frame_set_title(frame, titleString.c_str());
        libdecor_frame_set_app_id(frame, "rwin");
        if (!flags.Has(WindowFlags::Resizable))
        {
            libdecor_frame_unset_capabilities(frame, LIBDECOR_ACTION_RESIZE);
        }
        libdecor_frame_map(frame);
        wl_display_roundtrip(_display);
        wl_display_roundtrip(_display);
        return windowId;
    }

    void WaylandWindowManager::Destroy(const std::uint64_t& id)
    {
        if (id == _cursorFocusedHandle)
        {
            _cursorFocusedHandle = UINT64_NULL_HANDLE;
        }

        if (id == _keyboardFocusedHandle)
        {
            _keyboardFocusedHandle = UINT64_NULL_HANDLE;
            _repeatKeycode = 0;
        }

        if (id == _dragWindow)
        {
            _dragWindow = UINT64_NULL_HANDLE;
        }

        const auto found = _windows.find(id);
        if (found == _windows.end())
        {
            return;
        }

        const auto info = found->second;
        libdecor_frame_unref(info->frame);
        wl_surface_destroy(info->surface);
        _surfaceToWindows.erase(info->surface);
        _windows.erase(found);
        std::erase_if(_pendingEvents, [id](const WindowEvent& event) { return event.info.windowId == id; });
        _idFactory.Free(id);
    }

    Extent2D WaylandWindowManager::GetClientSize(const std::uint64_t& id)
    {
        if (const auto info = GetWindowInfo(id))
        {
            return GetPhysicalSize(info);
        }
        return {};
    }

    Point2D WaylandWindowManager::GetClientPosition(const std::uint64_t& id)
    {
        return {};
    }

    Vector2 WaylandWindowManager::GetCursorPosition(const std::uint64_t& id)
    {
        if (const auto info = GetWindowInfo(id))
        {
            return info->cursorPosition;
        }
        return {};
    }

    void WaylandWindowManager::Show(const std::uint64_t& id)
    {

    }

    void WaylandWindowManager::Hide(const std::uint64_t& id)
    {
        if (const auto info = GetWindowInfo(id))
        {

        }
    }

    void WaylandWindowManager::Minimize(const std::uint64_t& id)
    {
        if (const auto info = GetWindowInfo(id))
        {
            libdecor_frame_set_minimized(info->frame);
        }
    }

    void WaylandWindowManager::Maximize(const std::uint64_t& id)
    {
        if (const auto info = GetWindowInfo(id))
        {
            libdecor_frame_set_maximized(info->frame);
        }
    }

    float WaylandWindowManager::GetDpi(const std::uint64_t& id)
    {
        if (const auto info = GetWindowInfo(id))
        {
            return GetDefaultDpi() * static_cast<float>(info->scale);
        }
        return GetDefaultDpi();
    }

    Extent2D WaylandWindowManager::GetPhysicalSize(const WindowInfo* info) const
    {
        const auto scale = static_cast<std::uint32_t>(info->scale);
        return Extent2D{info->size.width * scale, info->size.height * scale};
    }

    void WaylandWindowManager::ApplyScale(WindowInfo* info, const std::int32_t scale)
    {
        const auto clamped = std::max(scale, 1);
        if (clamped == info->scale)
        {
            return;
        }

        info->scale = clamped;
        wl_surface_set_buffer_scale(info->surface, clamped);

        WindowEvent ev{};
        new(&ev.resize) ResizeEvent{
            .type = WindowEventType::Resize,
            .windowId = info->windowId,
            .size = GetPhysicalSize(info),
        };
        _pendingEvents.push_back(ev);
    }

    void WaylandWindowManager::UpdateScaleFromOutputs(WindowInfo* info)
    {
        if (_hasPreferredBufferScale)
        {
            return;
        }

        std::int32_t scale = 1;
        for (const auto output : info->outputs)
        {
            if (const auto found = _outputScales.find(output); found != _outputScales.end())
            {
                scale = std::max(scale, found->second);
            }
        }
        ApplyScale(info, scale);
    }

    void WaylandWindowManager::FlushScroll()
    {
        if (_scrollContinuous.x == 0.0f && _scrollContinuous.y == 0.0f &&
            _scrollDiscrete120.x == 0.0f && _scrollDiscrete120.y == 0.0f)
        {
            return;
        }

        const auto info = GetWindowInfo(_cursorFocusedHandle);
        if (info)
        {
            const auto scale = static_cast<float>(info->scale);
            const auto notchPixels = SCROLL_DEFAULT_LINES_PER_NOTCH * SCROLL_LINE_PIXELS * scale;
            const auto convert = [&](const float continuous, const float discrete120)
            {
                return discrete120 != 0.0f ? discrete120 / 120.0f * notchPixels : continuous * scale;
            };

            WindowEvent ev{};
            new(&ev.scroll) ScrollEvent{
                .type = WindowEventType::Scroll,
                .windowId = info->windowId,
                .position = info->cursorPosition,
                .delta = Vector2{
                    // Positive x scrolls left, matching the other backends.
                    -convert(_scrollContinuous.x, _scrollDiscrete120.x),
                    -convert(_scrollContinuous.y, _scrollDiscrete120.y),
                },
            };
            _pendingEvents.push_back(ev);
        }

        _scrollContinuous = {};
        _scrollDiscrete120 = {};
    }

    float WaylandWindowManager::GetDefaultDpi()
    {
        return 72.0f;
    }

    void WaylandWindowManager::DispatchEvents()
    {
        if (libdecor_dispatch(_decorContext, 0) < 0)
        {
            _connectionLost = true;
        }
        wl_display_dispatch_pending(_display);

        while (wl_display_prepare_read(_display) != 0)
        {
            wl_display_dispatch_pending(_display);
        }
        wl_display_flush(_display);

        pollfd descriptor{wl_display_get_fd(_display), POLLIN, 0};
        if (poll(&descriptor, 1, 0) > 0)
        {
            if (wl_display_read_events(_display) < 0)
            {
                _connectionLost = true;
            }
        }
        else
        {
            wl_display_cancel_read(_display);
        }

        if (wl_display_dispatch_pending(_display) < 0)
        {
            _connectionLost = true;
        }
    }

    void WaylandWindowManager::PumpEvents()
    {
        if (_pendingEvents.empty())
        {
            _textArena.Reset();
        }

        if (_connectionLost)
        {
            return;
        }

        DispatchEvents();
        ProcessKeyRepeat();

        if (_connectionLost)
        {
            std::cerr << "rwin: lost the connection to the Wayland compositor" << std::endl;
            for (const auto& window : _windows)
            {
                WindowEvent ev{};
                new(&ev.close) CloseEvent{
                    .type = WindowEventType::Close,
                    .windowId = window.first,
                };
                _pendingEvents.push_back(ev);
            }
        }
    }

    Flags<InputModifier> WaylandWindowManager::CurrentModifiers()
    {
        if (_keyboard)
        {
            if (const auto found = _keyboards.find(_keyboard); found != _keyboards.end())
            {
                return getInputModifiers(found->second->state);
            }
        }
        return {};
    }

    void WaylandWindowManager::HandleKey(KeyboardInfo& keyboard, const std::uint32_t keycode, const InputState state)
    {
        const auto info = GetWindowInfo(_keyboardFocusedHandle);
        if (!info)
        {
            return;
        }

        const xkb_keysym_t* symbols = nullptr;
        const auto layout = xkb_state_key_get_layout(keyboard.state, keycode);
        const auto symbolCount = xkb_keymap_key_get_syms_by_level(keyboard.keymap, keycode, layout, 0, &symbols);
        const auto key = symbolCount > 0 ? xkbKeyToInputKey(symbols[0]) : InputKey::Unknown;
        const auto modifiers = getInputModifiers(keyboard.state);

        WindowEvent ev{};
        new(&ev.key) KeyEvent{
            .type = WindowEventType::Key,
            .windowId = info->windowId,
            .key = key,
            .state = state,
            .modifier = static_cast<InputModifier>(modifiers)
        };
        _pendingEvents.push_back(ev);

        const bool shortcut = modifiers.Has(InputModifier::Control) || modifiers.Has(InputModifier::Alt) ||
            modifiers.Has(InputModifier::Super);
        if (state == InputState::Released || !info->textInputActive || shortcut)
        {
            return;
        }

        char utf8[16]{};
        const auto length = xkb_state_key_get_utf8(keyboard.state, keycode, utf8, sizeof(utf8));
        if (length <= 0 || static_cast<unsigned char>(utf8[0]) < 0x20 || static_cast<unsigned char>(utf8[0]) == 0x7F)
        {
            return;
        }

        const auto text = Utf8ToUtf16(std::string_view{utf8, static_cast<std::size_t>(length)});
        if (text.empty())
        {
            return;
        }

        WindowEvent textEv{};
        new(&textEv.textCommit) TextCommitEvent{
            .type = WindowEventType::TextCommit,
            .windowId = info->windowId,
            .text = _textArena.Append(text),
        };
        _pendingEvents.push_back(textEv);
    }

    void WaylandWindowManager::ProcessKeyRepeat()
    {
        if (_repeatKeycode == 0 || !_keyboard || _repeatRate <= 0)
        {
            return;
        }

        const auto found = _keyboards.find(_keyboard);
        if (found == _keyboards.end())
        {
            return;
        }

        const auto interval = std::chrono::milliseconds(std::max(1000 / _repeatRate, 1));
        const auto now = std::chrono::steady_clock::now();
        for (int emitted = 0; emitted < 8 && now >= _repeatNext; ++emitted)
        {
            HandleKey(*found->second, _repeatKeycode, InputState::Repeat);
            _repeatNext += interval;
        }
        if (now >= _repeatNext)
        {
            _repeatNext = now + interval;
        }
    }

    void WaylandWindowManager::ApplyCursor(const std::uint32_t serial, const std::int32_t scale)
    {
        if (!_pointer || !_shm || !_cursorSurface)
        {
            return;
        }

        if (!_cursorTheme || _cursorThemeScale != scale)
        {
            if (_cursorTheme)
            {
                wl_cursor_theme_destroy(_cursorTheme);
            }
            _cursorTheme = wl_cursor_theme_load(nullptr, 24 * scale, _shm);
            _cursorThemeScale = scale;
            _cursor = nullptr;
            if (_cursorTheme)
            {
                _cursor = wl_cursor_theme_get_cursor(_cursorTheme, "default");
                if (!_cursor) _cursor = wl_cursor_theme_get_cursor(_cursorTheme, "left_ptr");
            }
        }

        if (!_cursor || _cursor->image_count == 0)
        {
            return;
        }

        const auto image = _cursor->images[0];
        const auto buffer = wl_cursor_image_get_buffer(image);
        if (!buffer)
        {
            return;
        }

        wl_surface_set_buffer_scale(_cursorSurface, scale);
        wl_surface_attach(_cursorSurface, buffer, 0, 0);
        wl_surface_damage_buffer(_cursorSurface, 0, 0, static_cast<std::int32_t>(image->width),
                                 static_cast<std::int32_t>(image->height));
        wl_surface_commit(_cursorSurface);
        wl_pointer_set_cursor(_pointer, serial, _cursorSurface, static_cast<std::int32_t>(image->hotspot_x) / scale,
                              static_cast<std::int32_t>(image->hotspot_y) / scale);
    }

    void WaylandWindowManager::StartTextInput(const std::uint64_t& id, const Rect2D&)
    {
        if (const auto info = GetWindowInfo(id))
        {
            info->textInputActive = true;
        }
    }

    void WaylandWindowManager::StopTextInput(const std::uint64_t& id)
    {
        if (const auto info = GetWindowInfo(id))
        {
            info->textInputActive = false;
        }
    }

    std::u16string_view WaylandWindowManager::GetEventText(const TextRef& ref)
    {
        return _textArena.View(ref);
    }

    void WaylandWindowManager::GetRequiredExtensions(std::vector<const char*>& extensions)
    {
        extensions.emplace_back(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
        extensions.emplace_back(vk::KHRSurfaceExtensionName);
    }

    void WaylandWindowManager::SetHitTestCallback(const std::uint64_t& id,
                                                  const std::function<HitTestResult(const Vector2&)>& callback)
    {
    }

    void WaylandWindowManager::ClearHitTestCallback(const std::uint64_t& id)
    {
    }

    void WaylandWindowManager::SetDropCallbacks(const std::uint64_t& id, const DropCallbacks& callbacks)
    {
        if (const auto info = GetWindowInfo(id))
        {
            info->dropCallbacks = callbacks;
        }
    }

    void WaylandWindowManager::ClearDropCallbacks(const std::uint64_t& id)
    {
        if (const auto info = GetWindowInfo(id))
        {
            info->dropCallbacks = {};
        }
    }

    void WaylandWindowManager::CreateDataDevice()
    {
        if (!_dataDeviceManager || !_seat || _dataDevice)
        {
            return;
        }
        _dataDevice = wl_data_device_manager_get_data_device(_dataDeviceManager, _seat);
        wl_data_device_add_listener(_dataDevice, &_dataDeviceListener, this);
    }

    DataOffer* WaylandWindowManager::FindDataOffer(wl_data_offer* offer)
    {
        if (const auto found = _dataOffers.find(offer); found != _dataOffers.end())
        {
            return found->second.get();
        }
        return nullptr;
    }

    void WaylandWindowManager::DestroyDataOffer(wl_data_offer* offer)
    {
        if (_dataOffers.erase(offer) > 0)
        {
            wl_data_offer_destroy(offer);
        }
        if (offer == _selectionOffer)
        {
            _selectionOffer = nullptr;
        }
    }

    DropInfo WaylandWindowManager::MakeDropInfo(const DataOffer& offer)
    {
        Flags<DropEffect> allowed{};
        // Sources older than version 3, or ones that have not announced yet, report no actions.
        if (offer.sourceActions == 0 || (offer.sourceActions & WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY))
        {
            allowed.Add(DropEffect::Copy);
        }
        if (offer.sourceActions == 0 || (offer.sourceActions & WL_DATA_DEVICE_MANAGER_DND_ACTION_MOVE))
        {
            allowed.Add(DropEffect::Move);
        }

        return DropInfo{
            .position = _dragPosition,
            .allowed = allowed,
            .modifiers = CurrentModifiers(),
        };
    }

    void WaylandWindowManager::ApplyDragEffect(const DropEffect effect)
    {
        const auto dataOffer = FindDataOffer(_dragOffer);
        if (!dataOffer)
        {
            return;
        }

        const char* mimeType = nullptr;
        if (effect != DropEffect::None)
        {
            mimeType = FindMimeType(*dataOffer, {&URI_LIST_MIME_TYPE, 1});
            if (!mimeType)
            {
                mimeType = FindMimeType(*dataOffer, TEXT_MIME_TYPES);
            }
        }

        wl_data_offer_accept(dataOffer->offer, _dragSerial, mimeType);
        _dragAccepted = mimeType != nullptr;
        _dragEffect = _dragAccepted ? effect : DropEffect::None;

        if (wl_data_offer_get_version(dataOffer->offer) >= WL_DATA_OFFER_SET_ACTIONS_SINCE_VERSION)
        {
            std::uint32_t action = WL_DATA_DEVICE_MANAGER_DND_ACTION_NONE;
            if (_dragAccepted)
            {
                action = effect == DropEffect::Move
                             ? WL_DATA_DEVICE_MANAGER_DND_ACTION_MOVE
                             : WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY;
            }
            wl_data_offer_set_actions(dataOffer->offer, action, action);
        }
    }

    void WaylandWindowManager::EndDrag()
    {
        if (_dragOffer)
        {
            DestroyDataOffer(_dragOffer);
        }
        _dragOffer = nullptr;
        _dragWindow = UINT64_NULL_HANDLE;
        _dragAccepted = false;
        _dragEffect = DropEffect::None;
        _dragEnterEffect = DropEffect::None;
    }

    bool WaylandWindowManager::HasClipboardText()
    {
        if (_clipboardSource)
        {
            return !_clipboardText.empty();
        }
        const auto offer = FindDataOffer(_selectionOffer);
        return offer && FindMimeType(*offer, TEXT_MIME_TYPES) != nullptr;
    }

    bool WaylandWindowManager::GetClipboardText(std::u16string& out)
    {
        // Reading our own offer through a pipe would wait on ourselves.
        if (_clipboardSource)
        {
            if (_clipboardText.empty())
            {
                return false;
            }
            out = Utf8ToUtf16(_clipboardText);
            return true;
        }

        const auto offer = FindDataOffer(_selectionOffer);
        return offer && WaylandDataView{_display, offer}.GetText(out);
    }

    bool WaylandWindowManager::SetClipboardText(const std::u16string_view text)
    {
        // The compositor only accepts a selection tied to a recent input event.
        if (!_dataDeviceManager || !_dataDevice || _lastInputSerial == 0)
        {
            return false;
        }

        if (_clipboardSource)
        {
            wl_data_source_destroy(_clipboardSource);
        }
        _clipboardText = Utf16ToUtf8(text);
        _clipboardSource = wl_data_device_manager_create_data_source(_dataDeviceManager);
        wl_data_source_add_listener(_clipboardSource, &_dataSourceListener, this);
        for (const auto mimeType : TEXT_MIME_TYPES)
        {
            wl_data_source_offer(_clipboardSource, mimeType);
        }
        wl_data_device_set_selection(_dataDevice, _clipboardSource, _lastInputSerial);
        wl_display_flush(_display);
        return true;
    }

    void WaylandWindowManager::ClearClipboard()
    {
        if (_clipboardSource)
        {
            wl_data_source_destroy(_clipboardSource);
            _clipboardSource = nullptr;
            _clipboardText.clear();
        }
        if (_dataDevice && _lastInputSerial != 0)
        {
            wl_data_device_set_selection(_dataDevice, nullptr, _lastInputSerial);
            wl_display_flush(_display);
        }
    }

    WindowInfo* WaylandWindowManager::GetWindowInfo(const std::uint64_t& id)
    {
        if (_windows.contains(id))
        {
            return _windows[id].get();
        }

        return nullptr;
    }

    WindowInfo* WaylandWindowManager::GetWindowInfo(wl_surface* surface)
    {
        if (_surfaceToWindows.contains(surface))
        {
            return _surfaceToWindows[surface].get();
        }

        return nullptr;
    }
}
#endif
