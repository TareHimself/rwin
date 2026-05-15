#include "../macros_private.h"

#ifdef RWIN_PLATFORM_COMPAT
#include "GLFWWindowManager.h"

#include <deque>
#include <memory>
#include <string>
#include <unordered_map>

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include "rwin/IdFactory.h"

namespace rwin {
    namespace {
        struct WindowInfo {
            std::uint64_t id{};
            GLFWwindow* window{};
            std::function<HitTestResult(const Vector2&)> hitTestCallback{};
            DropCallbacks dropCallbacks{};
            bool hasHitTestCallback{false};
            bool hasDropCallbacks{false};
        };

        IdFactory g_idFactory{};
        std::unordered_map<std::uint64_t, std::unique_ptr<WindowInfo>> g_windows{};
        std::unordered_map<GLFWwindow*, std::uint64_t> g_windowIds{};
        std::deque<WindowEvent> g_pendingEvents{};

        WindowInfo* GetWindowInfo(const std::uint64_t& id) {
            const auto found = g_windows.find(id);
            if (found == g_windows.end()) {
                return nullptr;
            }

            return found->second.get();
        }

        WindowInfo* GetWindowInfo(GLFWwindow* window) {
            const auto found = g_windowIds.find(window);
            if (found == g_windowIds.end()) {
                return nullptr;
            }

            return GetWindowInfo(found->second);
        }

        InputState ToInputState(const int action) {
            switch (action) {
            case GLFW_PRESS:
                return InputState::Pressed;
            case GLFW_RELEASE:
                return InputState::Released;
            case GLFW_REPEAT:
                return InputState::Repeat;
            default:
                return InputState::Released;
            }
        }

        InputModifier ToInputModifier(const int mods) {
            Flags<InputModifier> result{};

            if ((mods & GLFW_MOD_SHIFT) != 0) {
                result.Add(InputModifier::Shift);
            }

            if ((mods & GLFW_MOD_CONTROL) != 0) {
                result.Add(InputModifier::Control);
            }

            if ((mods & GLFW_MOD_ALT) != 0) {
                result.Add(InputModifier::Alt);
            }

            if ((mods & GLFW_MOD_SUPER) != 0) {
                result.Add(InputModifier::Super);
            }

            if ((mods & GLFW_MOD_CAPS_LOCK) != 0) {
                result.Add(InputModifier::CapsLock);
            }

            if ((mods & GLFW_MOD_NUM_LOCK) != 0) {
                result.Add(InputModifier::NumLock);
            }

            return static_cast<InputModifier>(result);
        }

        InputKey ToInputKey(const int key) {
            switch (key) {
            case GLFW_KEY_A: return InputKey::A;
            case GLFW_KEY_B: return InputKey::B;
            case GLFW_KEY_C: return InputKey::C;
            case GLFW_KEY_D: return InputKey::D;
            case GLFW_KEY_E: return InputKey::E;
            case GLFW_KEY_F: return InputKey::F;
            case GLFW_KEY_G: return InputKey::G;
            case GLFW_KEY_H: return InputKey::H;
            case GLFW_KEY_I: return InputKey::I;
            case GLFW_KEY_J: return InputKey::J;
            case GLFW_KEY_K: return InputKey::K;
            case GLFW_KEY_L: return InputKey::L;
            case GLFW_KEY_M: return InputKey::M;
            case GLFW_KEY_N: return InputKey::N;
            case GLFW_KEY_O: return InputKey::O;
            case GLFW_KEY_P: return InputKey::P;
            case GLFW_KEY_Q: return InputKey::Q;
            case GLFW_KEY_R: return InputKey::R;
            case GLFW_KEY_S: return InputKey::S;
            case GLFW_KEY_T: return InputKey::T;
            case GLFW_KEY_U: return InputKey::U;
            case GLFW_KEY_V: return InputKey::V;
            case GLFW_KEY_W: return InputKey::W;
            case GLFW_KEY_X: return InputKey::X;
            case GLFW_KEY_Y: return InputKey::Y;
            case GLFW_KEY_Z: return InputKey::Z;

            case GLFW_KEY_0: return InputKey::Zero;
            case GLFW_KEY_1: return InputKey::One;
            case GLFW_KEY_2: return InputKey::Two;
            case GLFW_KEY_3: return InputKey::Three;
            case GLFW_KEY_4: return InputKey::Four;
            case GLFW_KEY_5: return InputKey::Five;
            case GLFW_KEY_6: return InputKey::Six;
            case GLFW_KEY_7: return InputKey::Seven;
            case GLFW_KEY_8: return InputKey::Eight;
            case GLFW_KEY_9: return InputKey::Nine;

            case GLFW_KEY_F1: return InputKey::F1;
            case GLFW_KEY_F2: return InputKey::F2;
            case GLFW_KEY_F3: return InputKey::F3;
            case GLFW_KEY_F4: return InputKey::F4;
            case GLFW_KEY_F5: return InputKey::F5;
            case GLFW_KEY_F6: return InputKey::F6;
            case GLFW_KEY_F7: return InputKey::F7;
            case GLFW_KEY_F8: return InputKey::F8;
            case GLFW_KEY_F9: return InputKey::F9;
            case GLFW_KEY_F10: return InputKey::F10;
            case GLFW_KEY_F11: return InputKey::F11;
            case GLFW_KEY_F12: return InputKey::F12;
            case GLFW_KEY_F13: return InputKey::F13;
            case GLFW_KEY_F14: return InputKey::F14;
            case GLFW_KEY_F15: return InputKey::F15;
            case GLFW_KEY_F16: return InputKey::F16;
            case GLFW_KEY_F17: return InputKey::F17;
            case GLFW_KEY_F18: return InputKey::F18;
            case GLFW_KEY_F19: return InputKey::F19;
            case GLFW_KEY_F20: return InputKey::F20;
            case GLFW_KEY_F21: return InputKey::F21;
            case GLFW_KEY_F22: return InputKey::F22;
            case GLFW_KEY_F23: return InputKey::F23;
            case GLFW_KEY_F24: return InputKey::F24;
            case GLFW_KEY_F25: return InputKey::F25;

            case GLFW_KEY_SPACE: return InputKey::Space;
            case GLFW_KEY_APOSTROPHE: return InputKey::Apostrophe;
            case GLFW_KEY_COMMA: return InputKey::Comma;
            case GLFW_KEY_MINUS: return InputKey::Minus;
            case GLFW_KEY_PERIOD: return InputKey::Period;
            case GLFW_KEY_SLASH: return InputKey::Slash;
            case GLFW_KEY_SEMICOLON: return InputKey::Semicolon;
            case GLFW_KEY_EQUAL: return InputKey::Equal;
            case GLFW_KEY_LEFT_BRACKET: return InputKey::LeftBracket;
            case GLFW_KEY_BACKSLASH: return InputKey::Backslash;
            case GLFW_KEY_RIGHT_BRACKET: return InputKey::RightBracket;
            case GLFW_KEY_GRAVE_ACCENT: return InputKey::GraveAccent;

            case GLFW_KEY_ESCAPE: return InputKey::Escape;
            case GLFW_KEY_ENTER: return InputKey::Enter;
            case GLFW_KEY_TAB: return InputKey::Tab;
            case GLFW_KEY_BACKSPACE: return InputKey::Backspace;
            case GLFW_KEY_INSERT: return InputKey::Insert;
            case GLFW_KEY_DELETE: return InputKey::Delete;
            case GLFW_KEY_RIGHT: return InputKey::Right;
            case GLFW_KEY_LEFT: return InputKey::Left;
            case GLFW_KEY_DOWN: return InputKey::Down;
            case GLFW_KEY_UP: return InputKey::Up;
            case GLFW_KEY_PAGE_UP: return InputKey::PageUp;
            case GLFW_KEY_PAGE_DOWN: return InputKey::PageDown;
            case GLFW_KEY_HOME: return InputKey::Home;
            case GLFW_KEY_END: return InputKey::End;
            case GLFW_KEY_CAPS_LOCK: return InputKey::CapsLock;
            case GLFW_KEY_SCROLL_LOCK: return InputKey::ScrollLock;
            case GLFW_KEY_NUM_LOCK: return InputKey::NumLock;
            case GLFW_KEY_PRINT_SCREEN: return InputKey::PrintScreen;
            case GLFW_KEY_PAUSE: return InputKey::Pause;
            case GLFW_KEY_LEFT_SHIFT: return InputKey::LeftShift;
            case GLFW_KEY_LEFT_CONTROL: return InputKey::LeftControl;
            case GLFW_KEY_LEFT_ALT: return InputKey::LeftAlt;
            case GLFW_KEY_LEFT_SUPER: return InputKey::LeftSuper;
            case GLFW_KEY_RIGHT_SHIFT: return InputKey::RightShift;
            case GLFW_KEY_RIGHT_CONTROL: return InputKey::RightControl;
            case GLFW_KEY_RIGHT_ALT: return InputKey::RightAlt;
            case GLFW_KEY_RIGHT_SUPER: return InputKey::RightSuper;
            case GLFW_KEY_MENU: return InputKey::Menu;

            default:
                return InputKey::Unknown;
            }
        }

        CursorButton ToCursorButton(const int button) {
            switch (button) {
            case GLFW_MOUSE_BUTTON_1: return CursorButton::One;
            case GLFW_MOUSE_BUTTON_2: return CursorButton::Two;
            case GLFW_MOUSE_BUTTON_3: return CursorButton::Three;
            case GLFW_MOUSE_BUTTON_4: return CursorButton::Four;
            case GLFW_MOUSE_BUTTON_5: return CursorButton::Five;
            case GLFW_MOUSE_BUTTON_6: return CursorButton::Six;
            default: return CursorButton::Seven;
            }
        }

        void RegisterCallbacks(GLFWwindow* window) {
            glfwSetWindowSizeCallback(window, [](GLFWwindow* glfwWindow, const int width, const int height) {
                const auto info = GetWindowInfo(glfwWindow);
                if (!info) {
                    return;
                }

                WindowEvent event{};
                new (&event.resize) ResizeEvent{
                    .type = WindowEventType::Resize,
                    .windowId = info->id,
                    .size = Extent2D{
                        .width = static_cast<std::uint32_t>(width),
                        .height = static_cast<std::uint32_t>(height)
                    }
                };
                g_pendingEvents.push_back(event);
            });

            glfwSetWindowCloseCallback(window, [](GLFWwindow* glfwWindow) {
                const auto info = GetWindowInfo(glfwWindow);
                if (!info) {
                    return;
                }

                glfwSetWindowShouldClose(glfwWindow, GLFW_FALSE);

                WindowEvent event{};
                new (&event.close) CloseEvent{
                    .type = WindowEventType::Close,
                    .windowId = info->id,
                };
                g_pendingEvents.push_back(event);
            });

            glfwSetKeyCallback(window, [](GLFWwindow* glfwWindow, const int key, int, const int action, const int mods) {
                const auto info = GetWindowInfo(glfwWindow);
                if (!info) {
                    return;
                }

                WindowEvent event{};
                new (&event.key) KeyEvent{
                    .type = WindowEventType::Key,
                    .windowId = info->id,
                    .key = ToInputKey(key),
                    .state = ToInputState(action),
                    .modifier = ToInputModifier(mods),
                };
                g_pendingEvents.push_back(event);
            });

            glfwSetCharCallback(window, [](GLFWwindow* glfwWindow, const unsigned int codepoint) {
                const auto info = GetWindowInfo(glfwWindow);
                if (!info) {
                    return;
                }

                WindowEvent event{};
                new (&event.text) TextEvent{
                    .type = WindowEventType::Text,
                    .windowId = info->id,
                    .text = static_cast<char16_t>(codepoint),
                };
                g_pendingEvents.push_back(event);
            });

            glfwSetCursorPosCallback(window, [](GLFWwindow* glfwWindow, const double x, const double y) {
                const auto info = GetWindowInfo(glfwWindow);
                if (!info) {
                    return;
                }

                WindowEvent event{};
                new (&event.cursorMove) CursorMoveEvent{
                    .type = WindowEventType::CursorMove,
                    .windowId = info->id,
                    .position = Vector2{
                        .x = static_cast<float>(x),
                        .y = static_cast<float>(y),
                    },
                };
                g_pendingEvents.push_back(event);
            });

            glfwSetCursorEnterCallback(window, [](GLFWwindow* glfwWindow, const int entered) {
                const auto info = GetWindowInfo(glfwWindow);
                if (!info) {
                    return;
                }

                WindowEvent event{};
                new (&event.cursorFocus) FocusEvent{
                    .type = WindowEventType::CursorFocus,
                    .windowId = info->id,
                    .focused = entered,
                };
                g_pendingEvents.push_back(event);
            });

            glfwSetWindowFocusCallback(window, [](GLFWwindow* glfwWindow, const int focused) {
                const auto info = GetWindowInfo(glfwWindow);
                if (!info) {
                    return;
                }

                WindowEvent event{};
                new (&event.keyboardFocus) FocusEvent{
                    .type = WindowEventType::KeyboardFocus,
                    .windowId = info->id,
                    .focused = focused,
                };
                g_pendingEvents.push_back(event);
            });

            glfwSetMouseButtonCallback(window, [](GLFWwindow* glfwWindow, const int button, const int action, const int mods) {
                const auto info = GetWindowInfo(glfwWindow);
                if (!info) {
                    return;
                }

                WindowEvent event{};
                new (&event.cursorButton) CursorButtonEvent{
                    .type = WindowEventType::CursorButton,
                    .windowId = info->id,
                    .button = ToCursorButton(button),
                    .state = ToInputState(action),
                    .modifier = ToInputModifier(mods),
                };
                g_pendingEvents.push_back(event);
            });

            glfwSetScrollCallback(window, [](GLFWwindow* glfwWindow, const double xOffset, const double yOffset) {
                const auto info = GetWindowInfo(glfwWindow);
                if (!info) {
                    return;
                }

                double x{};
                double y{};
                glfwGetCursorPos(glfwWindow, &x, &y);

                WindowEvent event{};
                new (&event.scroll) ScrollEvent{
                    .type = WindowEventType::Scroll,
                    .windowId = info->id,
                    .position = Vector2{
                        .x = static_cast<float>(x),
                        .y = static_cast<float>(y),
                    },
                    .delta = Vector2{
                        .x = static_cast<float>(xOffset),
                        .y = static_cast<float>(yOffset),
                    },
                };
                g_pendingEvents.push_back(event);
            });

            glfwSetDropCallback(window, [](GLFWwindow* glfwWindow, const int count, const char** paths) {
                const auto info = GetWindowInfo(glfwWindow);
                if (!info || !info->hasDropCallbacks || !info->dropCallbacks.drop) {
                    return;
                }

                double x{};
                double y{};
                glfwGetCursorPos(glfwWindow, &x, &y);

                struct GLFWDropContext final : IDropContext {
                    std::vector<std::filesystem::path> droppedPaths{};

                    bool HasFiles() override {
                        return !droppedPaths.empty();
                    }

                    bool HasText() override {
                        return false;
                    }

                    bool GetFiles(std::vector<std::filesystem::path>& paths) override {
                        paths.insert(paths.end(), droppedPaths.begin(), droppedPaths.end());
                        return !droppedPaths.empty();
                    }

                    bool GetText(std::vector<std::string>& text) override {
                        return false;
                    }
                } context{};

                for (int i = 0; i < count; ++i) {
                    context.droppedPaths.emplace_back(paths[i]);
                }

                info->dropCallbacks.drop(
                    Vector2{
                        .x = static_cast<float>(x),
                        .y = static_cast<float>(y),
                    },
                    &context
                );
            });
        }
    }

    GLFWWindowManager::~GLFWWindowManager() {
        for (const auto& [id, info] : g_windows) {
            if (info->window) {
                glfwDestroyWindow(info->window);
            }
        }

        g_windows.clear();
        g_windowIds.clear();
        g_pendingEvents.clear();

        glfwTerminate();
    }

    vk::SurfaceKHR GLFWWindowManager::CreateSurface(const std::uint64_t& id, const vk::Instance& instance) {
        const auto info = GetWindowInfo(id);
        if (!info) {
            return {};
        }

        VkSurfaceKHR surface{};
        if (glfwCreateWindowSurface(instance, info->window, nullptr, &surface) != VK_SUCCESS) {
            return {};
        }

        return surface;
    }

    std::uint64_t GLFWWindowManager::GetEvents(const std::span<WindowEvent>& events) {
        std::uint64_t gotten = 0;

        for (auto& event : events) {
            if (g_pendingEvents.empty()) {
                break;
            }

            event = g_pendingEvents.front();
            g_pendingEvents.pop_front();
            ++gotten;
        }

        return gotten;
    }

    std::uint64_t GLFWWindowManager::Create(const std::string_view& title, const Extent2D& size,
        const Flags<WindowFlags>& flags) {
        if (!glfwInit()) {
            return 0;
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_DECORATED, flags.Has(WindowFlags::Frameless) ? GLFW_FALSE : GLFW_TRUE);
        glfwWindowHint(GLFW_FLOATING, flags.Has(WindowFlags::Floating) ? GLFW_TRUE : GLFW_FALSE);
        glfwWindowHint(GLFW_RESIZABLE, flags.Has(WindowFlags::Resizable) ? GLFW_TRUE : GLFW_FALSE);
        glfwWindowHint(GLFW_VISIBLE, flags.Has(WindowFlags::Visible) ? GLFW_TRUE : GLFW_FALSE);
        glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, flags.Has(WindowFlags::Transparent) ? GLFW_TRUE : GLFW_FALSE);
        glfwWindowHint(GLFW_FOCUSED, flags.Has(WindowFlags::Focused) ? GLFW_TRUE : GLFW_FALSE);

        const std::string titleString{title};
        GLFWwindow* window = glfwCreateWindow(
            static_cast<int>(size.width),
            static_cast<int>(size.height),
            titleString.c_str(),
            nullptr,
            nullptr
        );

        if (!window) {
            return 0;
        }

        const auto id = g_idFactory.New();

        auto info = std::make_unique<WindowInfo>();
        info->id = id;
        info->window = window;

        g_windowIds.emplace(window, id);
        g_windows.emplace(id, std::move(info));

        RegisterCallbacks(window);

        return id;
    }

    void GLFWWindowManager::Destroy(const std::uint64_t& id) {
        const auto found = g_windows.find(id);
        if (found == g_windows.end()) {
            return;
        }

        const auto window = found->second->window;
        g_windowIds.erase(window);
        glfwDestroyWindow(window);
        g_windows.erase(found);
        g_idFactory.Free(id);
    }

    Extent2D GLFWWindowManager::GetClientSize(const std::uint64_t& id) {
        const auto info = GetWindowInfo(id);
        if (!info) {
            return {};
        }

        int width{};
        int height{};
        glfwGetFramebufferSize(info->window, &width, &height);

        return Extent2D{
            .width = static_cast<std::uint32_t>(width),
            .height = static_cast<std::uint32_t>(height),
        };
    }

    Point2D GLFWWindowManager::GetClientPosition(const std::uint64_t& id) {
        const auto info = GetWindowInfo(id);
        if (!info) {
            return {};
        }

        int x{};
        int y{};
        glfwGetWindowPos(info->window, &x, &y);

        return Point2D{
            .x = x,
            .y = y,
        };
    }

    Vector2 GLFWWindowManager::GetCursorPosition(const std::uint64_t& id) {
        const auto info = GetWindowInfo(id);
        if (!info) {
            return {};
        }

        double x{};
        double y{};
        glfwGetCursorPos(info->window, &x, &y);

        return Vector2{
            .x = static_cast<float>(x),
            .y = static_cast<float>(y),
        };
    }

    void GLFWWindowManager::Show(const std::uint64_t& id) {
        if (const auto info = GetWindowInfo(id)) {
            glfwShowWindow(info->window);
        }
    }

    void GLFWWindowManager::Hide(const std::uint64_t& id) {
        if (const auto info = GetWindowInfo(id)) {
            glfwHideWindow(info->window);
        }
    }

    void GLFWWindowManager::Minimize(const std::uint64_t& id) {
        if (const auto info = GetWindowInfo(id)) {
            glfwIconifyWindow(info->window);
        }
    }

    void GLFWWindowManager::Maximize(const std::uint64_t& id) {
        if (const auto info = GetWindowInfo(id)) {
            glfwMaximizeWindow(info->window);
        }
    }

    float GLFWWindowManager::GetDpi(const std::uint64_t& id) {
        const auto info = GetWindowInfo(id);
        if (!info) {
            return GetDefaultDpi();
        }

        GLFWmonitor* monitor = glfwGetWindowMonitor(info->window);
        if (!monitor) {
            int x{};
            int y{};
            glfwGetWindowPos(info->window, &x, &y);

            int monitorCount{};
            GLFWmonitor** monitors = glfwGetMonitors(&monitorCount);
            for (int i = 0; i < monitorCount; ++i) {
                int monitorX{};
                int monitorY{};
                int widthMm{};
                int heightMm{};
                glfwGetMonitorPos(monitors[i], &monitorX, &monitorY);

                const GLFWvidmode* mode = glfwGetVideoMode(monitors[i]);
                if (!mode) {
                    continue;
                }

                if (x >= monitorX && y >= monitorY && x < monitorX + mode->width && y < monitorY + mode->height) {
                    monitor = monitors[i];
                    break;
                }

                glfwGetMonitorPhysicalSize(monitors[i], &widthMm, &heightMm);
            }
        }

        if (!monitor) {
            return GetDefaultDpi();
        }

        float xScale{};
        float yScale{};
        glfwGetMonitorContentScale(monitor, &xScale, &yScale);

        return 96.0f * ((xScale + yScale) * 0.5f);
    }

    float GLFWWindowManager::GetDefaultDpi() {
        if (!glfwInit()) {
            return 96.0f;
        }

        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        if (!monitor) {
            return 96.0f;
        }

        float xScale{};
        float yScale{};
        glfwGetMonitorContentScale(monitor, &xScale, &yScale);

        return 96.0f * ((xScale + yScale) * 0.5f);
    }

    void GLFWWindowManager::PumpEvents() {
        glfwPollEvents();
    }

    void GLFWWindowManager::GetRequiredExtensions(std::vector<const char*>& extensions) {
        if (!glfwInit()) {
            return;
        }

        std::uint32_t count{};
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&count);
        if (!glfwExtensions) {
            return;
        }

        for (std::uint32_t i = 0; i < count; ++i) {
            extensions.emplace_back(glfwExtensions[i]);
        }
    }

    void GLFWWindowManager::SetHitTestCallback(const std::uint64_t& id,
        const std::function<HitTestResult(const Vector2&)>& callback) {
        if (const auto info = GetWindowInfo(id)) {
            info->hitTestCallback = callback;
            info->hasHitTestCallback = true;
        }
    }

    void GLFWWindowManager::ClearHitTestCallback(const std::uint64_t& id) {
        if (const auto info = GetWindowInfo(id)) {
            info->hitTestCallback = {};
            info->hasHitTestCallback = false;
        }
    }

    void GLFWWindowManager::SetDropCallbacks(const std::uint64_t& id, const DropCallbacks& callbacks) {
        if (const auto info = GetWindowInfo(id)) {
            info->dropCallbacks = callbacks;
            info->hasDropCallbacks = true;
        }
    }

    void GLFWWindowManager::ClearDropCallbacks(const std::uint64_t& id) {
        if (const auto info = GetWindowInfo(id)) {
            info->dropCallbacks = {};
            info->hasDropCallbacks = false;
        }
    }
}
#endif
