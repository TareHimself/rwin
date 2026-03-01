#include "rwin/macros.h"
#if defined(RWIN_PLATFORM_LINUX) && defined(RWIN_PLATFORM_LINUX_WAYLAND)
#include "WaylandWindowManager.h"

#include <iostream>
#include <ranges>
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
            std::cerr << "Unknown keysym: " << key << std::endl;
            return InputKey::Unknown;
        }
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
        state = xkb_state_new(keymap);
    }

    KeyboardInfo::~KeyboardInfo()
    {
        xkb_state_unref(state);
        xkb_keymap_unref(keymap);
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
                if (auto self = static_cast<WaylandWindowManager*>(data))
                {
                    if (auto ptr = self->_keyboards.find(wl_keyboard); ptr != self->_keyboards.end())
                    {
                        self->_keyboards.erase(ptr);
                    }

                    const auto keymapString = static_cast<char*>(mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0));

                    self->_keyboards.emplace(wl_keyboard,
                                             std::make_shared<KeyboardInfo>(
                                                 self->_xkbContext, keymapString, XKB_KEYMAP_FORMAT_TEXT_V1));
                    munmap(keymapString, size);
                    close(fd);
                }
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
                if (auto self = static_cast<WaylandWindowManager*>(data))
                {
                    if (const auto info = self->GetWindowInfo(surface))
                    {
                        self->_keyboardFocusedHandle = UINT64_NULL_HANDLE;
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
                if (auto self = static_cast<WaylandWindowManager*>(data))
                {
                    if (self->_keyboardFocusedHandle == UINT64_NULL_HANDLE) return;

                    const auto keyboardPtr = self->_keyboards.find(wl_keyboard);
                    if (keyboardPtr == self->_keyboards.end()) return;

                    const auto keyboard = keyboardPtr->second;

                    InputState inputState{};
                    xkb_key_direction direction{};
                    if (state == WL_KEYBOARD_KEY_STATE_PRESSED)
                    {
                        inputState = InputState::Pressed;
                        direction = xkb_key_direction::XKB_KEY_DOWN;
                    }
                    else if (state == WL_KEYBOARD_KEY_STATE_RELEASED)
                    {
                        inputState = InputState::Released;
                        direction = xkb_key_direction::XKB_KEY_UP;
                    }

                    const auto keyCode = key + 8;
                    xkb_state_update_key(keyboard->state, keyCode, direction);

                    const auto xkbKey = xkb_state_key_get_one_sym(keyboard->state, keyCode);
                    const auto rinKey = xkbKeyToInputKey(xkbKey);

                    if (keyboard->keysPressed.contains(rinKey) && inputState == InputState::Pressed)
                    {
                        inputState = InputState::Repeat;
                    }
                    const auto modifiers = getInputModifiers(keyboard->state);
                    WindowEvent ev{};
                    new(&ev.key) KeyEvent{
                        .type = WindowEventType::Key,
                        .windowId = self->_keyboardFocusedHandle,
                        .key = rinKey,
                        .state = inputState,
                        .modifier = static_cast<InputModifier>(modifiers)
                    };
                    self->_pendingEvents.push_back(ev);

                    if (keyboard->keysPressed.contains(rinKey) && inputState == InputState::Released)
                    {
                        keyboard->keysPressed.erase(rinKey);
                    }
                    else
                    {
                        keyboard->keysPressed.insert(rinKey);
                    }
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
                if (const auto self = static_cast<WaylandWindowManager*>(data))
                {
                    const auto keyboard = self->_keyboards[wl_keyboard];

                    xkb_state_update_mask(keyboard->state,
                                          mods_depressed, mods_latched, mods_locked, 0, 0, group);
                }
            },
            .repeat_info = [](void* data,
                              struct wl_keyboard* wl_keyboard,
                              int32_t rate,
                              int32_t delay)
            {
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
                        WindowEvent ev{};
                        new(&ev.cursorFocus) FocusEvent{
                            .type = WindowEventType::CursorFocus,
                            .windowId = info->windowId,
                            .focused = 1,
                        };
                        self->_pendingEvents.push_back(ev);
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
                        const auto x = static_cast<float>(wl_fixed_to_double(surface_x));
                        const auto y = static_cast<float>(wl_fixed_to_double(surface_y));
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
                            .modifier = static_cast<InputModifier>(0),
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
            },
            .frame = [](void* data,
                        struct wl_pointer* wl_pointer)
            {
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
            },
            .axis_value120 = [](void* data,
                                struct wl_pointer* wl_pointer,
                                uint32_t axis,
                                int32_t value120)
            {
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
                if ((capabilities & WL_SEAT_CAPABILITY_KEYBOARD) > 0)
                {
                    self->_keyboard = wl_seat_get_keyboard(self->_seat);
                    wl_keyboard_add_listener(self->_keyboard, &self->_keyboardListener, self);
                }

                if ((capabilities & WL_SEAT_CAPABILITY_POINTER) > 0)
                {
                    self->_pointer = wl_seat_get_pointer(self->_seat);
                    wl_pointer_add_listener(self->_pointer, &self->_pointerListener, self);
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
                    const auto bindVersion = std::min<uint32_t>(version, wl_seat_interface.version);
                    self->_compositor = static_cast<wl_compositor*>(wl_registry_bind(
                        registry, name, &wl_compositor_interface,
                        bindVersion));
                }
                else if (interfaceName == wl_seat_interface.name)
                {
                    const auto bindVersion = std::min<uint32_t>(version, wl_seat_interface.version);
                    self->_seat = static_cast<wl_seat*>(wl_registry_bind(
                        registry, name, &wl_seat_interface, bindVersion));
                    //wl_seat_add_listener(self->_seat, &seatListener, nullptr);
                }
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
                            .size = info->size,
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

        _displayListener = {
            .error = [](void *data,
                        struct wl_display *wl_display,
                        void *object_id,
                        uint32_t code,
                        const char *message) {
                std::cerr << "Display listener error: " << message << std::endl;
            }
        };

        _xkbContext = xkb_context_new(static_cast<xkb_context_flags>(0));
        _display = wl_display_connect(nullptr);
        //wl_display_add_listener(_display,&_displayListener,nullptr); // errors out with display already has a listener ?
        _registry = wl_display_get_registry(_display);
        wl_registry_add_listener(_registry, &_registryListener, this);
        wl_display_roundtrip(_display);
        _decorContext = libdecor_new(_display, &_decorInterface);
    }

    WaylandWindowManager::~WaylandWindowManager()
    {
        if (_keyboard) wl_keyboard_destroy(_keyboard);
        if (_pointer) wl_pointer_destroy(_pointer);
        if (_seat) wl_seat_destroy(_seat);
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
        windowInfo->flags = flags;
        windowInfo->size = size;
        windowInfo->frame = frame;

        libdecor_frame_set_title(frame, title.data());
        libdecor_frame_set_app_id(frame, "rin_app");
        libdecor_frame_map(frame);
        PumpEvents();
        return windowId;
    }

    void WaylandWindowManager::Destroy(const std::uint64_t& id)
    {
        if (id == _cursorFocusedHandle)
        {
            _cursorFocusedHandle = std::numeric_limits<std::uint64_t>::max();
        }

        if (id == _keyboardFocusedHandle)
        {
            _keyboardFocusedHandle = std::numeric_limits<std::uint64_t>::max();
        }
        if (const auto info = GetWindowInfo(id))
        {
            libdecor_frame_unref(info->frame);
            wl_surface_destroy(info->surface);
            _windows.erase(id);
        }
    }

    Extent2D WaylandWindowManager::GetClientSize(const std::uint64_t& id)
    {
        if (const auto info = GetWindowInfo(id))
        {
            return info->size;
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
        return GetDefaultDpi();
    }

    float WaylandWindowManager::GetDefaultDpi()
    {
        return 72.0f;
    }

    void WaylandWindowManager::PumpEvents()
    {
        wl_display_roundtrip(_display);
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
    }

    void WaylandWindowManager::ClearDropCallbacks(const std::uint64_t& id)
    {
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
