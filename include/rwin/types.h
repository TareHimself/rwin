#pragma once
#include <cstdint>
namespace rwin
{
    enum class Platform
    {
        Linux,
        Mac,
        Windows
    };
    struct Vector2
    {
        float x;
        float y;
    };

    struct Vector3 : Vector2
    {
        float z;
    };

    struct Vector4 : Vector3
    {
        float w;
    };

    struct Point2D
    {
        int x;
        int y;
    };

    struct Offset2D
    {
        uint32_t x;
        uint32_t y;
    };

    struct Extent2D
    {
        uint32_t width;
        uint32_t height;
        bool operator==(const Extent2D &other) const {
            return width == other.width && height == other.height;
        }
    };

    struct Extent3D : Extent2D
    {
        uint32_t dimension;
    };

    struct Rect2D
    {
        Offset2D offset;
        Extent2D extent;
    };

    struct WindowRect
    {
        Point2D position;
        Extent2D extent;
    };

    enum class HitTestResult : uint32_t
    {
        None,
        DragArea,
        TopResize,
        LeftResize,
        RightResize,
        BottomResize,
        CloseButton,
        MinimizeButton,
        MaximizeButton,
    };

    enum class WindowDropFormat
    {
        File,
        Image,
        Text,
    };

    enum class WindowEventType : uint32_t
    {
        Key,
        Resize,
        Minimize,
        Maximize,
        Scroll,
        CursorMove,
        CursorButton,
        Close,
        Text,
        CursorFocus,
        KeyboardFocus,
        DndEnter,
        DndDrop,
        DndLeave
    };

    enum class InputState : uint32_t
    {
        Pressed,
        Released,
        Repeat
    };

    enum class InputModifier : uint32_t
    {
        Shift = 0x0001,
        Control = 0x0002,
        Alt = 0x0004,
        Super = 0x0008,
        CapsLock = 0x0010,
        NumLock = 0x0020
    };

    enum class InputKey
    {
        Unknown,
        A,
        B,
        C,
        D,
        E,
        F,
        G,
        H,
        I,
        J,
        K,
        L,
        M,
        N,
        O,
        P,
        Q,
        R,
        S,
        T,
        U,
        V,
        W,
        X,
        Y,
        Z,
        Zero,
        One,
        Two,
        Three,
        Four,
        Five,
        Six,
        Seven,
        Eight,
        Nine,
        F1,
        F2,
        F3,
        F4,
        F5,
        F6,
        F7,
        F8,
        F9,
        F10,
        F11,
        F12,
        F13,
        F14,
        F15,
        F16,
        F17,
        F18,
        F19,
        F20,
        F21,
        F22,
        F23,
        F24,
        F25,
        Space,
        Apostrophe,
        Comma,
        Minus,
        Period,
        Slash,
        Semicolon,
        Equal,
        LeftBracket,
        Backslash,
        RightBracket,
        GraveAccent,
        Escape,
        Enter,
        Tab,
        Backspace,
        Insert,
        Delete,
        Right,
        Left,
        Down,
        Up,
        PageUp,
        PageDown,
        Home,
        End,
        CapsLock,
        ScrollLock,
        NumLock,
        PrintScreen,
        Pause,
        LeftShift,
        LeftControl,
        LeftAlt,
        LeftSuper,
        RightShift,
        RightControl,
        RightAlt,
        RightSuper,
        Menu
    };

    enum class CursorButton
    {
        One,
        Two,
        Three,
        Four,
        Five,
        Six,
        Seven
    };

    enum class WindowFlags : uint32_t
    {
        None = 0,
        Frameless =  1 << 0,
        Floating =  1 << 1,
        Resizable =  1 << 2,
        Visible =  1 << 3,
        Transparent =  1 << 4,
        Focused =  1 << 5,
        DragAndDrop =  1 << 5
    };

    struct EventInfo
    {
        WindowEventType type;
        std::uint64_t windowId;
    };

    struct KeyEvent
    {
        WindowEventType type;
        std::uint64_t windowId;
        InputKey key;
        InputState state;
        InputModifier modifier;
    };

    struct ResizeEvent
    {
        WindowEventType type;
        std::uint64_t windowId;
        Extent2D size;
    };

    struct MinimizeEvent
    {
        WindowEventType type;
        std::uint64_t windowId;

    };

    struct MaximizeEvent
    {
        WindowEventType type;
        std::uint64_t windowId;

    };

    struct ScrollEvent
    {
        WindowEventType type;
        std::uint64_t windowId;
        Vector2 position;
        Vector2 delta;
    };

    struct CursorMoveEvent
    {
        WindowEventType type;
        std::uint64_t windowId;
        Vector2 position;
    };

    struct CursorButtonEvent
    {
        WindowEventType type;
        std::uint64_t windowId;
        CursorButton button;
        InputState state;
        InputModifier modifier;
    };

    struct FocusEvent
    {
        WindowEventType type;
        std::uint64_t windowId;
        int focused;
    };


    struct CloseEvent
    {
        WindowEventType type;
        std::uint64_t windowId;

    };

    struct TextEvent
    {
        WindowEventType type;
        std::uint64_t windowId;
        char16_t text;
    };

    struct WindowEvent
    {
        union
        {
            EventInfo info;
            KeyEvent key;
            ResizeEvent resize;
            MinimizeEvent minimize;
            MaximizeEvent maximize;
            ScrollEvent scroll;
            CursorMoveEvent cursorMove;
            CursorButtonEvent cursorButton;
            FocusEvent cursorFocus;
            FocusEvent keyboardFocus;
            CloseEvent close;
            TextEvent text;
        };
    };
}