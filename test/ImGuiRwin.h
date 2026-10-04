#pragma once
#include <algorithm>
#include <cfloat>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <filesystem>
#include <string>
#include <imgui.h>
#include "rwin/rwin.h"

namespace imguirwin
{
    constexpr float PIXELS_PER_NOTCH = 60.0f;
    constexpr std::size_t MAX_LOG_LINES = 200;

    struct State
    {
        std::uint64_t windowId{};
        bool textInputActive{false};
        float appliedScale{0.0f};
        ImGuiStyle baseStyle{};
        ImVec2 imeInputPos{};
        float imeLineHeight{0.0f};
        std::string preedit{};
        std::deque<std::string> log{};
        ImVec2 lastScrollPixels{};
        float lastScrollNotches{0.0f};
        std::string textBuffer = std::string(512, '\0');
        std::string multilineBuffer = std::string(2048, '\0');
    };

    inline State& state()
    {
        static State instance;
        return instance;
    }

    inline std::string toUtf8(const std::u16string_view text)
    {
        std::string out;
        for (std::size_t i = 0; i < text.size(); ++i)
        {
            char32_t cp = text[i];
            if (cp >= 0xD800 && cp < 0xDC00 && i + 1 < text.size())
            {
                cp = 0x10000 + ((cp - 0xD800) << 10) + (text[++i] - 0xDC00);
            }
            if (cp < 0x80)
            {
                out += static_cast<char>(cp);
            }
            else if (cp < 0x800)
            {
                out += static_cast<char>(0xC0 | (cp >> 6));
                out += static_cast<char>(0x80 | (cp & 0x3F));
            }
            else if (cp < 0x10000)
            {
                out += static_cast<char>(0xE0 | (cp >> 12));
                out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                out += static_cast<char>(0x80 | (cp & 0x3F));
            }
            else
            {
                out += static_cast<char>(0xF0 | (cp >> 18));
                out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
                out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                out += static_cast<char>(0x80 | (cp & 0x3F));
            }
        }
        return out;
    }

    inline void pushLog(std::string line)
    {
        if (std::getenv("RWIN_TEST_LOG"))
        {
            std::fprintf(stderr, "%s\n", line.c_str());
        }
        auto& log = state().log;
        log.push_back(std::move(line));
        while (log.size() > MAX_LOG_LINES)
        {
            log.pop_front();
        }
    }

    inline ImGuiKey mapKey(const rwin::InputKey key)
    {
        using rwin::InputKey;
        if (key >= InputKey::A && key <= InputKey::Z)
        {
            return static_cast<ImGuiKey>(ImGuiKey_A + (static_cast<int>(key) - static_cast<int>(InputKey::A)));
        }
        if (key >= InputKey::Zero && key <= InputKey::Nine)
        {
            return static_cast<ImGuiKey>(ImGuiKey_0 + (static_cast<int>(key) - static_cast<int>(InputKey::Zero)));
        }
        if (key >= InputKey::F1 && key <= InputKey::F24)
        {
            return static_cast<ImGuiKey>(ImGuiKey_F1 + (static_cast<int>(key) - static_cast<int>(InputKey::F1)));
        }

        switch (key)
        {
        case InputKey::Space: return ImGuiKey_Space;
        case InputKey::Apostrophe: return ImGuiKey_Apostrophe;
        case InputKey::Comma: return ImGuiKey_Comma;
        case InputKey::Minus: return ImGuiKey_Minus;
        case InputKey::Period: return ImGuiKey_Period;
        case InputKey::Slash: return ImGuiKey_Slash;
        case InputKey::Semicolon: return ImGuiKey_Semicolon;
        case InputKey::Equal: return ImGuiKey_Equal;
        case InputKey::LeftBracket: return ImGuiKey_LeftBracket;
        case InputKey::Backslash: return ImGuiKey_Backslash;
        case InputKey::RightBracket: return ImGuiKey_RightBracket;
        case InputKey::GraveAccent: return ImGuiKey_GraveAccent;
        case InputKey::Escape: return ImGuiKey_Escape;
        case InputKey::Enter: return ImGuiKey_Enter;
        case InputKey::Tab: return ImGuiKey_Tab;
        case InputKey::Backspace: return ImGuiKey_Backspace;
        case InputKey::Insert: return ImGuiKey_Insert;
        case InputKey::Delete: return ImGuiKey_Delete;
        case InputKey::Right: return ImGuiKey_RightArrow;
        case InputKey::Left: return ImGuiKey_LeftArrow;
        case InputKey::Down: return ImGuiKey_DownArrow;
        case InputKey::Up: return ImGuiKey_UpArrow;
        case InputKey::PageUp: return ImGuiKey_PageUp;
        case InputKey::PageDown: return ImGuiKey_PageDown;
        case InputKey::Home: return ImGuiKey_Home;
        case InputKey::End: return ImGuiKey_End;
        case InputKey::CapsLock: return ImGuiKey_CapsLock;
        case InputKey::ScrollLock: return ImGuiKey_ScrollLock;
        case InputKey::NumLock: return ImGuiKey_NumLock;
        case InputKey::PrintScreen: return ImGuiKey_PrintScreen;
        case InputKey::Pause: return ImGuiKey_Pause;
        case InputKey::LeftShift: return ImGuiKey_LeftShift;
        case InputKey::LeftControl: return ImGuiKey_LeftCtrl;
        case InputKey::LeftAlt: return ImGuiKey_LeftAlt;
        case InputKey::LeftSuper: return ImGuiKey_LeftSuper;
        case InputKey::RightShift: return ImGuiKey_RightShift;
        case InputKey::RightControl: return ImGuiKey_RightCtrl;
        case InputKey::RightAlt: return ImGuiKey_RightAlt;
        case InputKey::RightSuper: return ImGuiKey_RightSuper;
        case InputKey::Menu: return ImGuiKey_Menu;
        default: return ImGuiKey_None;
        }
    }

    inline void loadFonts()
    {
        const auto& io = ImGui::GetIO();
        io.Fonts->AddFontDefault();

        const char* candidates[] = {
            "C:/Windows/Fonts/msyh.ttc",
            "C:/Windows/Fonts/YuGothM.ttc",
            "C:/Windows/Fonts/meiryo.ttc",
            "/usr/share/fonts/truetype/noto/NotoSansCJK-Regular.ttc",
            "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
            "/usr/share/fonts/truetype/wqy/wqy-microhei.ttc",
        };
        for (const auto* path : candidates)
        {
            if (std::filesystem::exists(path))
            {
                ImFontConfig config{};
                config.MergeMode = true;
                io.Fonts->AddFontFromFileTTF(path, 13.0f, &config, io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
                break;
            }
        }
    }

    inline void init(const std::uint64_t windowId)
    {
        auto& s = state();
        s.windowId = windowId;

        ImGui::CreateContext();
        ImGui::StyleColorsDark();
        s.baseStyle = ImGui::GetStyle();

        auto& io = ImGui::GetIO();
        io.IniFilename = nullptr;
        io.BackendPlatformName = "rwin";

        loadFonts();

        ImGui::GetPlatformIO().Platform_SetImeDataFn = [](ImGuiContext*, ImGuiViewport*, ImGuiPlatformImeData* data)
        {
            auto& s = state();
            s.imeInputPos = data->InputPos;
            s.imeLineHeight = data->InputLineHeight;
        };
    }

    inline void handleEvent(const rwin::WindowEvent& event)
    {
        using rwin::WindowEventType;
        auto& io = ImGui::GetIO();
        char line[160];

        switch (event.info.type)
        {
        case WindowEventType::CursorMove:
            io.AddMousePosEvent(event.cursorMove.position.x, event.cursorMove.position.y);
            break;
        case WindowEventType::CursorFocus:
            if (event.cursorFocus.focused == 0)
            {
                io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
            }
            std::snprintf(line, sizeof(line), "cursor focus %d", event.cursorFocus.focused);
            pushLog(line);
            break;
        case WindowEventType::KeyboardFocus:
            io.AddFocusEvent(event.keyboardFocus.focused != 0);
            std::snprintf(line, sizeof(line), "keyboard focus %d", event.keyboardFocus.focused);
            pushLog(line);
            break;
        case WindowEventType::CursorButton:
            {
                const auto index = static_cast<int>(event.cursorButton.button);
                if (index < ImGuiMouseButton_COUNT && event.cursorButton.state != rwin::InputState::Repeat)
                {
                    io.AddMouseButtonEvent(index, event.cursorButton.state == rwin::InputState::Pressed);
                }
                std::snprintf(line, sizeof(line), "button %d %s", index + 1,
                              event.cursorButton.state == rwin::InputState::Pressed ? "down" : "up");
                pushLog(line);
                break;
            }
        case WindowEventType::Scroll:
            {
                const float pixelsPerNotch = PIXELS_PER_NOTCH * std::max(state().appliedScale, 1.0f);
                io.AddMouseWheelEvent(event.scroll.delta.x / pixelsPerNotch, event.scroll.delta.y / pixelsPerNotch);
                state().lastScrollPixels = ImVec2{event.scroll.delta.x, event.scroll.delta.y};
                state().lastScrollNotches = event.scroll.delta.y / pixelsPerNotch;
                std::snprintf(line, sizeof(line), "scroll %.1f, %.1f px (%.2f notches)", event.scroll.delta.x,
                              event.scroll.delta.y, state().lastScrollNotches);
                pushLog(line);
                break;
            }
        case WindowEventType::Key:
            {
                const auto key = mapKey(event.key.key);
                if (key != ImGuiKey_None)
                {
                    io.AddKeyEvent(key, event.key.state != rwin::InputState::Released);
                }
                if (event.key.state != rwin::InputState::Repeat)
                {
                    std::snprintf(line, sizeof(line), "key %d %s mods 0x%x", static_cast<int>(event.key.key),
                                  event.key.state == rwin::InputState::Pressed ? "down" : "up",
                                  static_cast<unsigned>(event.key.modifier));
                    pushLog(line);
                }
                break;
            }
        case WindowEventType::TextCommit:
            {
                const auto text = toUtf8(rwin::getEventText(event.textCommit.text));
                io.AddInputCharactersUTF8(text.c_str());
                pushLog("commit: " + text);
                break;
            }
        case WindowEventType::TextPreedit:
            {
                state().preedit = toUtf8(rwin::getEventText(event.textPreedit.text));
                std::snprintf(line, sizeof(line), "preedit caret %u-%u", event.textPreedit.caretStart,
                              event.textPreedit.caretEnd);
                pushLog(line);
                break;
            }
        case WindowEventType::Resize:
            std::snprintf(line, sizeof(line), "resize %u x %u", event.resize.size.width, event.resize.size.height);
            pushLog(line);
            break;
        case WindowEventType::Minimize:
            pushLog("minimize");
            break;
        case WindowEventType::Maximize:
            pushLog("maximize");
            break;
        default:
            break;
        }
    }

    inline void newFrame(const float deltaTime)
    {
        auto& s = state();
        auto& io = ImGui::GetIO();

        const auto size = rwin::getWindowClientSize(s.windowId);
        io.DisplaySize = ImVec2{static_cast<float>(size.width), static_cast<float>(size.height)};
        io.DeltaTime = std::max(deltaTime, 1.0f / 1000.0f);

        const float scale = rwin::getWindowDpi(s.windowId) / rwin::getDefaultDpi();
        if (scale != s.appliedScale)
        {
            ImGui::GetStyle() = s.baseStyle;
            ImGui::GetStyle().ScaleAllSizes(scale);
            io.FontGlobalScale = scale;
            s.appliedScale = scale;
        }
    }

    inline void endFrame()
    {
        auto& s = state();
        const auto& io = ImGui::GetIO();

        if (io.WantTextInput)
        {
            const rwin::Rect2D caret{
                .offset = {static_cast<std::int32_t>(s.imeInputPos.x), static_cast<std::int32_t>(s.imeInputPos.y)},
                .extent = {2, static_cast<std::uint32_t>(std::max(s.imeLineHeight, 1.0f))},
            };
            if (!s.textInputActive)
            {
                rwin::startTextInput(s.windowId, caret);
                s.textInputActive = true;
            }
            else
            {
                rwin::setTextInputRect(s.windowId, caret);
            }
        }
        else if (s.textInputActive)
        {
            rwin::stopTextInput(s.windowId);
            s.textInputActive = false;
            s.preedit.clear();
        }
    }

    inline void drawUi()
    {
        auto& s = state();
        const auto& io = ImGui::GetIO();
        const float scale = s.appliedScale;

        ImGui::SetNextWindowPos(ImVec2{32.0f * scale, 40.0f * scale});
        ImGui::SetNextWindowSize(ImVec2{io.DisplaySize.x - 64.0f * scale, io.DisplaySize.y - 72.0f * scale});
        ImGui::Begin("rwin test", nullptr,
                     ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

        ImGui::Text("client %.0f x %.0f px   dpi %.0f   scale %.2f", io.DisplaySize.x, io.DisplaySize.y,
                    rwin::getWindowDpi(s.windowId), scale);
        ImGui::Text("cursor %.0f, %.0f   buttons %d%d%d", io.MousePos.x, io.MousePos.y, io.MouseDown[0] ? 1 : 0,
                    io.MouseDown[1] ? 1 : 0, io.MouseDown[2] ? 1 : 0);
        ImGui::Text("last scroll %.1f, %.1f px   %.2f notches", s.lastScrollPixels.x, s.lastScrollPixels.y,
                    s.lastScrollNotches);

        if (ImGui::Button("Minimize"))
        {
            rwin::minimizeWindow(s.windowId);
        }
        ImGui::SameLine();
        if (ImGui::Button("Maximize"))
        {
            rwin::maximizeWindow(s.windowId);
        }

        ImGui::SeparatorText("Scroll");
        ImGui::BeginChild("scrollList", ImVec2{0.0f, 160.0f * scale}, ImGuiChildFlags_Borders);
        for (int i = 0; i < 300; ++i)
        {
            ImGui::Text("Item %d", i);
        }
        ImGui::Text("scroll y %.0f / %.0f", ImGui::GetScrollY(), ImGui::GetScrollMaxY());
        ImGui::EndChild();

        ImGui::SeparatorText("Text input");
        ImGui::InputText("single line", s.textBuffer.data(), s.textBuffer.size());
        ImGui::InputTextMultiline("multi line", s.multilineBuffer.data(), s.multilineBuffer.size(),
                                  ImVec2{0.0f, 70.0f * scale});
        ImGui::Text("preedit: [%s]", s.preedit.c_str());

        ImGui::SeparatorText("Events");
        ImGui::BeginChild("eventLog", ImVec2{0.0f, 0.0f}, ImGuiChildFlags_Borders);
        for (const auto& line : s.log)
        {
            ImGui::TextUnformatted(line.c_str());
        }
        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 1.0f)
        {
            ImGui::SetScrollHereY(1.0f);
        }
        ImGui::EndChild();

        ImGui::End();
    }
}
