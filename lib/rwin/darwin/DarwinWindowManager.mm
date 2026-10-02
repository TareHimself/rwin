#import "DarwinWindowManager.h"

#ifdef RWIN_PLATFORM_DARWIN

#import <AppKit/AppKit.h>
#import <Carbon/Carbon.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#include <vulkan/vulkan_metal.h>

#include <deque>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "rwin/IdFactory.h"
#include "rwin/IDropContext.h"
#include "../TextArena.h"

// Forward declarations for ObjC classes used in WindowInfo
@class RWINView;
@class RWINWindowDelegate;

// ---------------------------------------------------------------------------
// Per-window state, shared with the ObjC view and delegate via their info pointer
// ---------------------------------------------------------------------------
namespace {
    using namespace rwin;

    struct DarwinWindowInfo {
        uint64_t id{};
        __strong NSWindow*           window{nil};
        __strong CAMetalLayer*       metalLayer{nil};
        __strong RWINView*           view{nil};
        __strong RWINWindowDelegate* delegate{nil};
        // Owning manager's queue; the view and delegate reach it through their `info` pointer.
        std::deque<WindowEvent>* events{nullptr};
        TextArena* text{nullptr};
        bool textInputActive{false};
        Rect2D caretRect{}; // window-local backing pixels, pushed by SetTextInputRect
        std::function<HitTestResult(const Vector2&)> hitTestCallback{};
        DropCallbacks dropCallbacks{};
        bool zoomed{false}; // last observed zoom state, so Maximize fires once per transition
    };

    InputKey ToInputKey(unsigned short code) {
        switch (code) {
        case kVK_ANSI_A:            return InputKey::A;
        case kVK_ANSI_B:            return InputKey::B;
        case kVK_ANSI_C:            return InputKey::C;
        case kVK_ANSI_D:            return InputKey::D;
        case kVK_ANSI_E:            return InputKey::E;
        case kVK_ANSI_F:            return InputKey::F;
        case kVK_ANSI_G:            return InputKey::G;
        case kVK_ANSI_H:            return InputKey::H;
        case kVK_ANSI_I:            return InputKey::I;
        case kVK_ANSI_J:            return InputKey::J;
        case kVK_ANSI_K:            return InputKey::K;
        case kVK_ANSI_L:            return InputKey::L;
        case kVK_ANSI_M:            return InputKey::M;
        case kVK_ANSI_N:            return InputKey::N;
        case kVK_ANSI_O:            return InputKey::O;
        case kVK_ANSI_P:            return InputKey::P;
        case kVK_ANSI_Q:            return InputKey::Q;
        case kVK_ANSI_R:            return InputKey::R;
        case kVK_ANSI_S:            return InputKey::S;
        case kVK_ANSI_T:            return InputKey::T;
        case kVK_ANSI_U:            return InputKey::U;
        case kVK_ANSI_V:            return InputKey::V;
        case kVK_ANSI_W:            return InputKey::W;
        case kVK_ANSI_X:            return InputKey::X;
        case kVK_ANSI_Y:            return InputKey::Y;
        case kVK_ANSI_Z:            return InputKey::Z;
        case kVK_ANSI_0:            return InputKey::Zero;
        case kVK_ANSI_1:            return InputKey::One;
        case kVK_ANSI_2:            return InputKey::Two;
        case kVK_ANSI_3:            return InputKey::Three;
        case kVK_ANSI_4:            return InputKey::Four;
        case kVK_ANSI_5:            return InputKey::Five;
        case kVK_ANSI_6:            return InputKey::Six;
        case kVK_ANSI_7:            return InputKey::Seven;
        case kVK_ANSI_8:            return InputKey::Eight;
        case kVK_ANSI_9:            return InputKey::Nine;
        case kVK_F1:                return InputKey::F1;
        case kVK_F2:                return InputKey::F2;
        case kVK_F3:                return InputKey::F3;
        case kVK_F4:                return InputKey::F4;
        case kVK_F5:                return InputKey::F5;
        case kVK_F6:                return InputKey::F6;
        case kVK_F7:                return InputKey::F7;
        case kVK_F8:                return InputKey::F8;
        case kVK_F9:                return InputKey::F9;
        case kVK_F10:               return InputKey::F10;
        case kVK_F11:               return InputKey::F11;
        case kVK_F12:               return InputKey::F12;
        case kVK_F13:               return InputKey::F13;
        case kVK_F14:               return InputKey::F14;
        case kVK_F15:               return InputKey::F15;
        case kVK_F16:               return InputKey::F16;
        case kVK_F17:               return InputKey::F17;
        case kVK_F18:               return InputKey::F18;
        case kVK_F19:               return InputKey::F19;
        case kVK_F20:               return InputKey::F20;
        case kVK_Space:             return InputKey::Space;
        case kVK_ANSI_Quote:        return InputKey::Apostrophe;
        case kVK_ANSI_Comma:        return InputKey::Comma;
        case kVK_ANSI_Minus:        return InputKey::Minus;
        case kVK_ANSI_Period:       return InputKey::Period;
        case kVK_ANSI_Slash:        return InputKey::Slash;
        case kVK_ANSI_Semicolon:    return InputKey::Semicolon;
        case kVK_ANSI_Equal:        return InputKey::Equal;
        case kVK_ANSI_LeftBracket:  return InputKey::LeftBracket;
        case kVK_ANSI_Backslash:    return InputKey::Backslash;
        case kVK_ANSI_RightBracket: return InputKey::RightBracket;
        case kVK_ANSI_Grave:        return InputKey::GraveAccent;
        case kVK_Escape:            return InputKey::Escape;
        case kVK_Return:            return InputKey::Enter;
        case kVK_Tab:               return InputKey::Tab;
        case kVK_Delete:            return InputKey::Backspace;
        case kVK_Help:              return InputKey::Insert;
        case kVK_ForwardDelete:     return InputKey::Delete;
        case kVK_RightArrow:        return InputKey::Right;
        case kVK_LeftArrow:         return InputKey::Left;
        case kVK_DownArrow:         return InputKey::Down;
        case kVK_UpArrow:           return InputKey::Up;
        case kVK_PageUp:            return InputKey::PageUp;
        case kVK_PageDown:          return InputKey::PageDown;
        case kVK_Home:              return InputKey::Home;
        case kVK_End:               return InputKey::End;
        case kVK_CapsLock:          return InputKey::CapsLock;
        case kVK_Shift:             return InputKey::LeftShift;
        case kVK_Control:           return InputKey::LeftControl;
        case kVK_Option:            return InputKey::LeftAlt;
        case kVK_Command:           return InputKey::LeftSuper;
        case kVK_RightShift:        return InputKey::RightShift;
        case kVK_RightControl:      return InputKey::RightControl;
        case kVK_RightOption:       return InputKey::RightAlt;
        case 0x36:                  return InputKey::RightSuper; // kVK_RightCommand (not in HIToolbox)
        default:                    return InputKey::Unknown;
        }
    }

    InputModifier ToInputModifier(NSEventModifierFlags flags) {
        Flags<InputModifier> result{};
        if (flags & NSEventModifierFlagShift)    result.Add(InputModifier::Shift);
        if (flags & NSEventModifierFlagControl)  result.Add(InputModifier::Control);
        if (flags & NSEventModifierFlagOption)   result.Add(InputModifier::Alt);
        if (flags & NSEventModifierFlagCommand)  result.Add(InputModifier::Super);
        if (flags & NSEventModifierFlagCapsLock) result.Add(InputModifier::CapsLock);
        return static_cast<InputModifier>(result);
    }

    CursorButton ToCursorButton(NSInteger button) {
        switch (button) {
        case 0:  return CursorButton::One;
        case 1:  return CursorButton::Two;
        case 2:  return CursorButton::Three;
        case 3:  return CursorButton::Four;
        case 4:  return CursorButton::Five;
        case 5:  return CursorButton::Six;
        default: return CursorButton::Seven;
        }
    }

    void ActivateApp() {
        if (@available(macOS 14.0, *)) {
            [NSApp activate];
        } else {
            [NSApp activateIgnoringOtherApps:YES];
        }
    }
} // anonymous namespace

// ---------------------------------------------------------------------------
// Drop context
// ---------------------------------------------------------------------------
struct DarwinDropContext final : rwin::IDropContext {
    __strong NSPasteboard* pasteboard{nil};

    bool HasFiles() override {
        return [pasteboard canReadObjectForClasses:@[NSURL.class]
                                          options:@{NSPasteboardURLReadingFileURLsOnlyKey: @YES}];
    }

    bool HasText() override {
        return [pasteboard availableTypeFromArray:@[NSPasteboardTypeString]] != nil;
    }

    bool GetFiles(std::vector<std::filesystem::path>& paths) override {
        NSArray* urls = [pasteboard readObjectsForClasses:@[NSURL.class]
                                                  options:@{NSPasteboardURLReadingFileURLsOnlyKey: @YES}];
        for (NSURL* url in urls) {
            paths.emplace_back(url.fileSystemRepresentation);
        }
        return !paths.empty();
    }

    bool GetText(std::vector<std::string>& text) override {
        NSString* str = [pasteboard stringForType:NSPasteboardTypeString];
        if (str) {
            text.emplace_back(str.UTF8String);
            return true;
        }
        return false;
    }
};

// ---------------------------------------------------------------------------
// Window delegate
// ---------------------------------------------------------------------------
@interface RWINWindowDelegate : NSObject <NSWindowDelegate>
// Owned by the manager; cleared in Destroy() before the info is freed.
@property (nonatomic, assign) DarwinWindowInfo* info;
@end

@implementation RWINWindowDelegate

- (BOOL)windowShouldClose:(NSWindow*)sender {
    auto* info = self.info;
    if (!info) return YES;
    WindowEvent ev{};
    new (&ev.close) rwin::CloseEvent{
        .type    = rwin::WindowEventType::Close,
        .windowId = info->id,
    };
    self.info->events->push_back(ev);
    return NO;
}

- (void)windowDidResize:(NSNotification*)notification {
    auto* info = self.info;
    if (!info) return;
    NSWindow* window = info->window;
    NSView* view = window.contentView;
    CGSize sz = [view convertSizeToBacking:view.bounds.size];
    if (info->metalLayer) {
        info->metalLayer.drawableSize = sz;
    }
    const bool zoomed = [window isZoomed];
    if (zoomed && !info->zoomed) {
        WindowEvent maxEv{};
        new (&maxEv.maximize) rwin::MaximizeEvent{
            .type    = rwin::WindowEventType::Maximize,
            .windowId = info->id,
        };
        self.info->events->push_back(maxEv);
    }
    info->zoomed = zoomed;
    WindowEvent ev{};
    new (&ev.resize) rwin::ResizeEvent{
        .type    = rwin::WindowEventType::Resize,
        .windowId = info->id,
        .size    = rwin::Extent2D{
            .width  = static_cast<uint32_t>(sz.width),
            .height = static_cast<uint32_t>(sz.height),
        },
    };
    self.info->events->push_back(ev);
}

- (void)windowDidChangeBackingProperties:(NSNotification*)notification {
    auto* info = self.info;
    if (!info) return;
    info->metalLayer.contentsScale = info->window.backingScaleFactor;
    [self windowDidResize:notification];
}

- (void)windowDidMiniaturize:(NSNotification*)notification {
    auto* info = self.info;
    if (!info) return;
    WindowEvent ev{};
    new (&ev.minimize) rwin::MinimizeEvent{
        .type    = rwin::WindowEventType::Minimize,
        .windowId = info->id,
    };
    self.info->events->push_back(ev);
}

- (void)windowDidBecomeKey:(NSNotification*)notification {
    auto* info = self.info;
    if (!info) return;
    WindowEvent ev{};
    new (&ev.keyboardFocus) rwin::FocusEvent{
        .type    = rwin::WindowEventType::KeyboardFocus,
        .windowId = info->id,
        .focused = 1,
    };
    self.info->events->push_back(ev);
}

- (void)windowDidResignKey:(NSNotification*)notification {
    auto* info = self.info;
    if (!info) return;
    WindowEvent ev{};
    new (&ev.keyboardFocus) rwin::FocusEvent{
        .type    = rwin::WindowEventType::KeyboardFocus,
        .windowId = info->id,
        .focused = 0,
    };
    self.info->events->push_back(ev);
}

@end

// ---------------------------------------------------------------------------
// Window
// ---------------------------------------------------------------------------
// Borderless windows refuse key/main status by default, which would leave frameless windows
// without keyboard input.
@interface RWINWindow : NSWindow
// Asks the application to close this window by queueing a Close event; never closes it directly.
// Unlike performClose:, this also works for frameless windows, which have no close button.
- (void)requestClose:(id)sender;
@end

@implementation RWINWindow
- (BOOL)canBecomeKeyWindow  { return YES; }
- (BOOL)canBecomeMainWindow { return YES; }

- (void)requestClose:(id)sender {
    id<NSWindowDelegate> d = self.delegate;
    if (d && ![d windowShouldClose:self]) return;
    [self close];
}
@end

// ---------------------------------------------------------------------------
// App-level setup
// ---------------------------------------------------------------------------
// Target for the Quit menu item: quitting means asking every window to close, so the application
// receives a Close event per window and decides when to actually exit.
@interface RWINAppController : NSObject
- (void)requestQuit:(id)sender;
@end

@implementation RWINAppController
- (void)requestQuit:(id)sender {
    for (NSWindow* window in [NSApp.windows copy]) {
        if ([window isKindOfClass:RWINWindow.class]) {
            [(RWINWindow*)window requestClose:sender];
        }
    }
}
@end

namespace {
    NSMenuItem* AddMenuItem(NSMenu* menu, NSString* title, SEL action, NSString* key,
        NSEventModifierFlags mods = NSEventModifierFlagCommand) {
        NSMenuItem* item = [menu addItemWithTitle:title action:action keyEquivalent:key];
        item.keyEquivalentModifierMask = mods;
        return item;
    }

    // Without a main menu the standard Cmd shortcuts (hide, minimize, close, quit) do nothing.
    void InstallMainMenu() {
        static RWINAppController* controller = [[RWINAppController alloc] init];
        NSMenu* bar = [[NSMenu alloc] init];

        NSMenuItem* appItem = [bar addItemWithTitle:@"" action:nil keyEquivalent:@""];
        NSMenu* appMenu = [[NSMenu alloc] init];
        AddMenuItem(appMenu, @"Hide", @selector(hide:), @"h");
        AddMenuItem(appMenu, @"Hide Others", @selector(hideOtherApplications:), @"h",
            NSEventModifierFlagCommand | NSEventModifierFlagOption);
        AddMenuItem(appMenu, @"Show All", @selector(unhideAllApplications:), @"");
        [appMenu addItem:[NSMenuItem separatorItem]];
        AddMenuItem(appMenu, @"Quit", @selector(requestQuit:), @"q").target = controller;
        appItem.submenu = appMenu;

        NSMenuItem* windowItem = [bar addItemWithTitle:@"Window" action:nil keyEquivalent:@""];
        NSMenu* windowMenu = [[NSMenu alloc] initWithTitle:@"Window"];
        AddMenuItem(windowMenu, @"Minimize", @selector(performMiniaturize:), @"m");
        AddMenuItem(windowMenu, @"Zoom", @selector(performZoom:), @"");
        AddMenuItem(windowMenu, @"Close", @selector(requestClose:), @"w");
        windowItem.submenu = windowMenu;

        [NSApp setMainMenu:bar];
        [NSApp setWindowsMenu:windowMenu];
    }

    NSString* PlainString(id string) {
        return [string isKindOfClass:NSAttributedString.class] ? ((NSAttributedString*)string).string : string;
    }

    // Queues a text event for `string`; its UTF-16 payload goes into the manager's text arena.
    void PushText(DarwinWindowInfo& info, NSString* string, const WindowEventType type,
        const std::uint32_t caretStart = 0, const std::uint32_t caretEnd = 0) {
        static_assert(sizeof(unichar) == sizeof(char16_t));
        const auto length = static_cast<std::uint32_t>(string.length);
        const TextRef ref = info.text->Reserve(length);
        if (length > 0) {
            [string getCharacters:reinterpret_cast<unichar*>(info.text->Data(ref)) range:NSMakeRange(0, length)];
        }

        WindowEvent ev{};
        if (type == WindowEventType::TextCommit) {
            new (&ev.textCommit) TextCommitEvent{.type = type, .windowId = info.id, .text = ref};
        } else {
            new (&ev.textPreedit) TextPreeditEvent{
                .type = type, .windowId = info.id, .text = ref,
                .caretStart = caretStart, .caretEnd = caretEnd};
        }
        info.events->push_back(ev);
    }

    // NSApplication is process-wide, so this is the one piece of state that is legitimately global.
    void EnsureAppInitialized() {
        static bool initialized = false;
        if (initialized) return;
        initialized = true;
        @autoreleasepool {
            if (NSApp == nil) {
                [NSApplication sharedApplication];
                [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
                InstallMainMenu();
                [NSApp finishLaunching];
            }
        }
    }
} // anonymous namespace

// ---------------------------------------------------------------------------
// Content view
// ---------------------------------------------------------------------------
@interface RWINView : NSView <NSDraggingDestination, NSTextInputClient>
@property (nonatomic, assign) DarwinWindowInfo* info;
// The IME's current composition, nil when idle.
@property (nonatomic, copy) NSString* markedText;
@end

@implementation RWINView

- (BOOL)acceptsFirstResponder { return YES; }
- (BOOL)isFlipped              { return YES; }

- (void)updateTrackingAreas {
    for (NSTrackingArea* ta in self.trackingAreas.copy) {
        [self removeTrackingArea:ta];
    }
    [super updateTrackingAreas];
    NSTrackingAreaOptions opts =
        NSTrackingMouseEnteredAndExited |
        NSTrackingMouseMoved |
        NSTrackingActiveAlways |
        NSTrackingInVisibleRect;
    [self addTrackingArea:[[NSTrackingArea alloc] initWithRect:NSZeroRect
                                                       options:opts
                                                         owner:self
                                                      userInfo:nil]];
}

// Keyboard

- (void)keyDown:(NSEvent*)event {
    auto* info = self.info;

    // While an IME is composing it owns the keys (candidate selection, confirm, cancel), so the
    // application only sees keys the IME wasn't already handling.
    if (!self.hasMarkedText) {
        WindowEvent keyEv{};
        new (&keyEv.key) rwin::KeyEvent{
            .type     = rwin::WindowEventType::Key,
            .windowId = info->id,
            .key      = ToInputKey([event keyCode]),
            .state    = [event isARepeat] ? rwin::InputState::Repeat : rwin::InputState::Pressed,
            .modifier = ToInputModifier([event modifierFlags]),
        };
        info->events->push_back(keyEv);
    }

    // Routes the key through the input method, which calls back into the NSTextInputClient
    // methods below with committed or marked text.
    if (info->textInputActive) {
        [self interpretKeyEvents:@[event]];
    }
}

- (void)keyUp:(NSEvent*)event {
    WindowEvent ev{};
    new (&ev.key) rwin::KeyEvent{
        .type     = rwin::WindowEventType::Key,
        .windowId = self.info->id,
        .key      = ToInputKey([event keyCode]),
        .state    = rwin::InputState::Released,
        .modifier = ToInputModifier([event modifierFlags]),
    };
    self.info->events->push_back(ev);
}

- (void)flagsChanged:(NSEvent*)event {
    rwin::InputKey key  = rwin::InputKey::Unknown;
    bool pressed        = false;
    NSEventModifierFlags mods = [event modifierFlags];
    switch ([event keyCode]) {
    case kVK_Shift:        key = rwin::InputKey::LeftShift;    pressed = (mods & NSEventModifierFlagShift)    != 0; break;
    case kVK_RightShift:   key = rwin::InputKey::RightShift;   pressed = (mods & NSEventModifierFlagShift)    != 0; break;
    case kVK_Control:      key = rwin::InputKey::LeftControl;  pressed = (mods & NSEventModifierFlagControl)  != 0; break;
    case kVK_RightControl: key = rwin::InputKey::RightControl; pressed = (mods & NSEventModifierFlagControl)  != 0; break;
    case kVK_Option:       key = rwin::InputKey::LeftAlt;      pressed = (mods & NSEventModifierFlagOption)   != 0; break;
    case kVK_RightOption:  key = rwin::InputKey::RightAlt;     pressed = (mods & NSEventModifierFlagOption)   != 0; break;
    case kVK_Command:      key = rwin::InputKey::LeftSuper;    pressed = (mods & NSEventModifierFlagCommand)  != 0; break;
    case 0x36:             key = rwin::InputKey::RightSuper;   pressed = (mods & NSEventModifierFlagCommand)  != 0; break; // kVK_RightCommand
    case kVK_CapsLock:     key = rwin::InputKey::CapsLock;     pressed = (mods & NSEventModifierFlagCapsLock) != 0; break;
    default: return;
    }
    WindowEvent ev{};
    new (&ev.key) rwin::KeyEvent{
        .type     = rwin::WindowEventType::Key,
        .windowId = self.info->id,
        .key      = key,
        .state    = pressed ? rwin::InputState::Pressed : rwin::InputState::Released,
        .modifier = ToInputModifier(mods),
    };
    self.info->events->push_back(ev);
}

// Text input (NSTextInputClient)

- (BOOL)hasMarkedText { return self.markedText.length > 0; }

- (NSRange)markedRange {
    return self.hasMarkedText ? NSMakeRange(0, self.markedText.length) : NSMakeRange(NSNotFound, 0);
}

// The application owns the document selection and it isn't exposed to the IME.
- (NSRange)selectedRange { return NSMakeRange(NSNotFound, 0); }

- (NSArray<NSAttributedStringKey>*)validAttributesForMarkedText { return @[]; }

- (NSAttributedString*)attributedSubstringForProposedRange:(NSRange)range actualRange:(NSRangePointer)actualRange {
    return nil;
}

- (NSUInteger)characterIndexForPoint:(NSPoint)point { return 0; }

// Editing commands (Enter, Backspace, arrows) were already reported as KeyEvents; swallowing them
// here also avoids the system beep for unhandled commands.
- (void)doCommandBySelector:(SEL)selector {}

- (void)setMarkedText:(id)string selectedRange:(NSRange)selectedRange replacementRange:(NSRange)replacementRange {
    auto* info = self.info;
    if (!info->textInputActive) return;

    NSString* text = PlainString(string);
    self.markedText = text.length > 0 ? text : nil;

    const NSUInteger start = selectedRange.location == NSNotFound ? text.length : MIN(selectedRange.location, text.length);
    const NSUInteger end   = MIN(start + selectedRange.length, text.length);
    PushText(*info, text, rwin::WindowEventType::TextPreedit,
        static_cast<std::uint32_t>(start), static_cast<std::uint32_t>(end));
}

// AppKit calls this to accept the composition as typed (focus loss, clicking away), so the marked
// text becomes a commit rather than being dropped.
- (void)unmarkText {
    auto* info = self.info;
    NSString* text = self.markedText;
    if (!info || text.length == 0) return;

    self.markedText = nil;
    PushText(*info, @"", rwin::WindowEventType::TextPreedit);
    PushText(*info, text, rwin::WindowEventType::TextCommit);
}

- (void)insertText:(id)string replacementRange:(NSRange)replacementRange {
    auto* info = self.info;
    if (!info->textInputActive) return;

    if (self.hasMarkedText) {
        self.markedText = nil;
        PushText(*info, @"", rwin::WindowEventType::TextPreedit);
    }
    NSString* text = PlainString(string);
    if (text.length > 0) {
        PushText(*info, text, rwin::WindowEventType::TextCommit);
    }
}

// Screen rect of the text caret, which the IME uses to place its candidate popup.
- (NSRect)firstRectForCharacterRange:(NSRange)range actualRange:(NSRangePointer)actualRange {
    const Rect2D& caret = self.info->caretRect;
    const NSRect inView = [self convertRectFromBacking:
        NSMakeRect(caret.offset.x, caret.offset.y, caret.extent.width, caret.extent.height)];
    return [self.window convertRectToScreen:[self convertRect:inView toView:nil]];
}

// Mouse helpers

// Window-local position in backing pixels, the same unit as resize events and GetClientSize().
- (NSPoint)localPos:(NSEvent*)event {
    return [self convertPointToBacking:[self convertPoint:[event locationInWindow] fromView:nil]];
}

- (NSPoint)dragPos:(id<NSDraggingInfo>)sender {
    return [self convertPointToBacking:[self convertPoint:[sender draggingLocation] fromView:nil]];
}

- (void)pushCursorButton:(NSEvent*)event button:(rwin::CursorButton)btn state:(rwin::InputState)st {
    WindowEvent ev{};
    new (&ev.cursorButton) rwin::CursorButtonEvent{
        .type     = rwin::WindowEventType::CursorButton,
        .windowId = self.info->id,
        .button   = btn,
        .state    = st,
        .modifier = ToInputModifier([event modifierFlags]),
    };
    self.info->events->push_back(ev);
}

- (void)pushCursorMove:(NSEvent*)event {
    NSPoint p = [self localPos:event];
    WindowEvent ev{};
    new (&ev.cursorMove) rwin::CursorMoveEvent{
        .type     = rwin::WindowEventType::CursorMove,
        .windowId = self.info->id,
        .position = {.x = static_cast<float>(p.x), .y = static_cast<float>(p.y)},
    };
    self.info->events->push_back(ev);
}

// Mouse events

- (void)mouseDown:(NSEvent*)event {
    auto* info = self.info;
    if (info && info->hitTestCallback) {
        NSPoint p = [self localPos:event];
        auto result = info->hitTestCallback(
            rwin::Vector2{.x = static_cast<float>(p.x), .y = static_cast<float>(p.y)});
        switch (result) {
        case rwin::HitTestResult::DragArea:
            [self.window performWindowDragWithEvent:event];
            return;
        case rwin::HitTestResult::CloseButton:
            [(RWINWindow*)self.window requestClose:nil];
            return;
        case rwin::HitTestResult::MinimizeButton:
            [self.window miniaturize:nil];
            return;
        case rwin::HitTestResult::MaximizeButton:
            [self.window zoom:nil];
            return;
        default:
            break;
        }
    }
    [self pushCursorButton:event button:rwin::CursorButton::One state:rwin::InputState::Pressed];
}

- (void)mouseUp:(NSEvent*)event {
    [self pushCursorButton:event button:rwin::CursorButton::One state:rwin::InputState::Released];
}

- (void)rightMouseDown:(NSEvent*)event {
    [self pushCursorButton:event button:rwin::CursorButton::Two state:rwin::InputState::Pressed];
}

- (void)rightMouseUp:(NSEvent*)event {
    [self pushCursorButton:event button:rwin::CursorButton::Two state:rwin::InputState::Released];
}

- (void)otherMouseDown:(NSEvent*)event {
    [self pushCursorButton:event button:ToCursorButton([event buttonNumber]) state:rwin::InputState::Pressed];
}

- (void)otherMouseUp:(NSEvent*)event {
    [self pushCursorButton:event button:ToCursorButton([event buttonNumber]) state:rwin::InputState::Released];
}

- (void)mouseMoved:(NSEvent*)event     { [self pushCursorMove:event]; }
- (void)mouseDragged:(NSEvent*)event   { [self pushCursorMove:event]; }
- (void)rightMouseDragged:(NSEvent*)event { [self pushCursorMove:event]; }
- (void)otherMouseDragged:(NSEvent*)event { [self pushCursorMove:event]; }

- (void)mouseEntered:(NSEvent*)event {
    WindowEvent ev{};
    new (&ev.cursorFocus) rwin::FocusEvent{
        .type    = rwin::WindowEventType::CursorFocus,
        .windowId = self.info->id,
        .focused = 1,
    };
    self.info->events->push_back(ev);
}

- (void)mouseExited:(NSEvent*)event {
    WindowEvent ev{};
    new (&ev.cursorFocus) rwin::FocusEvent{
        .type    = rwin::WindowEventType::CursorFocus,
        .windowId = self.info->id,
        .focused = 0,
    };
    self.info->events->push_back(ev);
}

- (void)scrollWheel:(NSEvent*)event {
    NSPoint p = [self localPos:event];

    // AppKit's point deltas cover both trackpads and line-based wheels (it converts lines to points
    // itself). Scale to backing pixels so scroll shares units with positions and resize events.
    const CGFloat scale = self.window.backingScaleFactor;
    CGFloat dx = [event scrollingDeltaX] * scale;
    CGFloat dy = [event scrollingDeltaY] * scale;
    if (CGEventRef cg = [event CGEvent]) {
        dx = CGEventGetDoubleValueField(cg, kCGScrollWheelEventPointDeltaAxis2) * scale;
        dy = CGEventGetDoubleValueField(cg, kCGScrollWheelEventPointDeltaAxis1) * scale;
    }

    WindowEvent ev{};
    new (&ev.scroll) rwin::ScrollEvent{
        .type     = rwin::WindowEventType::Scroll,
        .windowId = self.info->id,
        .position = {.x = static_cast<float>(p.x), .y = static_cast<float>(p.y)},
        .delta    = {
            .x = static_cast<float>(dx),
            .y = static_cast<float>(dy),
        },
    };
    self.info->events->push_back(ev);
}

// Drag and drop

- (NSDragOperation)draggingEntered:(id<NSDraggingInfo>)sender {
    auto* info = self.info;
    if (!info || !info->dropCallbacks.enter)
        return NSDragOperationNone;
    NSPoint p = [self dragPos:sender];
    DarwinDropContext ctx;
    ctx.pasteboard = [sender draggingPasteboard];
    bool ok = info->dropCallbacks.enter(
        {.x = static_cast<float>(p.x), .y = static_cast<float>(p.y)}, &ctx);
    return ok ? NSDragOperationCopy : NSDragOperationNone;
}

- (NSDragOperation)draggingUpdated:(id<NSDraggingInfo>)sender {
    auto* info = self.info;
    if (!info || !info->dropCallbacks.over)
        return NSDragOperationNone;
    NSPoint p = [self dragPos:sender];
    DarwinDropContext ctx;
    ctx.pasteboard = [sender draggingPasteboard];
    bool ok = info->dropCallbacks.over(
        {.x = static_cast<float>(p.x), .y = static_cast<float>(p.y)}, &ctx);
    return ok ? NSDragOperationCopy : NSDragOperationNone;
}

- (void)draggingExited:(id<NSDraggingInfo>)sender {
    auto* info = self.info;
    if (info && info->dropCallbacks.leave)
        info->dropCallbacks.leave();
}

- (BOOL)prepareForDragOperation:(id<NSDraggingInfo>)sender { return YES; }

- (BOOL)performDragOperation:(id<NSDraggingInfo>)sender {
    auto* info = self.info;
    if (!info || !info->dropCallbacks.drop)
        return NO;
    NSPoint p = [self dragPos:sender];
    DarwinDropContext ctx;
    ctx.pasteboard = [sender draggingPasteboard];
    info->dropCallbacks.drop(
        {.x = static_cast<float>(p.x), .y = static_cast<float>(p.y)}, &ctx);
    return YES;
}

@end

// ---------------------------------------------------------------------------
// DarwinWindowManager method implementations
// ---------------------------------------------------------------------------
namespace rwin {

struct DarwinWindowManager::Impl {
    IdFactory idFactory{};
    std::unordered_map<uint64_t, std::unique_ptr<DarwinWindowInfo>> windows{};
    std::deque<WindowEvent> pendingEvents{};
    TextArena text{};

    DarwinWindowInfo* Find(const uint64_t id) {
        auto it = windows.find(id);
        return it != windows.end() ? it->second.get() : nullptr;
    }

    // Ids are recycled, so events still queued for a destroyed window must not reach its successor.
    void PurgeEvents(const uint64_t id) {
        std::erase_if(pendingEvents, [id](const WindowEvent& e) { return e.close.windowId == id; });
    }

    // Detach the ObjC objects from `info` so they can't touch it after it's freed.
    static void Close(DarwinWindowInfo& info) {
        info.view.info = nullptr;
        info.delegate.info = nullptr;
        [info.window setDelegate:nil];
        [info.window close];
    }
};

DarwinWindowManager::DarwinWindowManager() : _impl(std::make_unique<Impl>()) {}

DarwinWindowManager::~DarwinWindowManager() {
    for (auto& [id, info] : _impl->windows) {
        Impl::Close(*info);
    }
}

vk::SurfaceKHR DarwinWindowManager::CreateSurface(const std::uint64_t& id,
    const vk::Instance& instance) {
    auto* info = _impl->Find(id);
    if (!info || !info->metalLayer) return {};

    auto fn = reinterpret_cast<PFN_vkCreateMetalSurfaceEXT>(
        vkGetInstanceProcAddr(static_cast<VkInstance>(instance), "vkCreateMetalSurfaceEXT"));
    if (!fn) return {};

    VkMetalSurfaceCreateInfoEXT ci{};
    ci.sType  = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT;
    ci.pLayer = info->metalLayer;

    VkSurfaceKHR surface{};
    if (fn(static_cast<VkInstance>(instance), &ci, nullptr, &surface) != VK_SUCCESS)
        return {};
    return vk::SurfaceKHR{surface};
}

std::uint64_t DarwinWindowManager::GetEvents(const std::span<WindowEvent>& events) {
    std::uint64_t gotten = 0;
    for (auto& event : events) {
        if (_impl->pendingEvents.empty()) break;
        event = _impl->pendingEvents.front();
        _impl->pendingEvents.pop_front();
        ++gotten;
    }
    return gotten;
}

std::uint64_t DarwinWindowManager::Create(const std::string_view& title, const Extent2D& size,
    const Flags<WindowFlags>& flags) {
    @autoreleasepool {
        EnsureAppInitialized();

        NSWindowStyleMask styleMask;
        if (flags.Has(WindowFlags::Frameless)) {
            styleMask = NSWindowStyleMaskBorderless;
        } else {
            styleMask = NSWindowStyleMaskTitled |
                        NSWindowStyleMaskClosable |
                        NSWindowStyleMaskMiniaturizable;
            if (flags.Has(WindowFlags::Resizable))
                styleMask |= NSWindowStyleMaskResizable;
        }

        NSRect frame = NSMakeRect(0, 0,
            static_cast<CGFloat>(size.width),
            static_cast<CGFloat>(size.height));

        NSWindow* window = [[RWINWindow alloc]
            initWithContentRect:frame
                      styleMask:styleMask
                        backing:NSBackingStoreBuffered
                          defer:NO];
        if (!window) return 0;
        // ARC owns the window via DarwinWindowInfo; AppKit's default release-on-close would over-release it.
        [window setReleasedWhenClosed:NO];

        [window setTitle:[NSString stringWithUTF8String:std::string(title).c_str()]];
        [window center];
        [window setAcceptsMouseMovedEvents:YES];

        if (flags.Has(WindowFlags::Floating))
            [window setLevel:NSFloatingWindowLevel];

        if (flags.Has(WindowFlags::Transparent)) {
            [window setOpaque:NO];
            [window setBackgroundColor:[NSColor clearColor]];
        }

        // Content view backed by a CAMetalLayer
        RWINView* view = [[RWINView alloc] initWithFrame:frame];
        view.wantsLayer = YES;

        CAMetalLayer* metalLayer = [CAMetalLayer layer];
        metalLayer.pixelFormat   = MTLPixelFormatBGRA8Unorm;
        // Render-target only, so Metal can use its faster compositor paths. Copying from or sampling
        // the drawable is unsupported; MoltenVK may relax this at swapchain creation based on usage flags.
        metalLayer.framebufferOnly = YES;
        view.layer = metalLayer;

        [window setContentView:view];
        [window makeFirstResponder:view];

        // The window's real screen is only known once it's attached, so size the drawable here.
        const CGFloat scale = window.backingScaleFactor;
        metalLayer.contentsScale = scale;
        metalLayer.drawableSize  = CGSizeMake(size.width * scale, size.height * scale);

        if (flags.Has(WindowFlags::DragAndDrop)) {
            [view registerForDraggedTypes:
                @[NSPasteboardTypeFileURL, NSPasteboardTypeString]];
        }

        const auto windowId = _impl->idFactory.New();

        RWINWindowDelegate* delegate = [[RWINWindowDelegate alloc] init];
        [window setDelegate:delegate];

        auto info        = std::make_unique<DarwinWindowInfo>();
        info->id         = windowId;
        info->window     = window;
        info->metalLayer = metalLayer;
        info->view       = view;
        info->delegate   = delegate;
        info->events     = &_impl->pendingEvents;
        info->text       = &_impl->text;
        view.info        = info.get();
        delegate.info    = info.get();

        _impl->windows.emplace(windowId, std::move(info));

        if (flags.Has(WindowFlags::Visible) || flags.Has(WindowFlags::Focused)) {
            [window makeKeyAndOrderFront:nil];
            ActivateApp();
        }

        return windowId;
    }
}

void DarwinWindowManager::Destroy(const std::uint64_t& id) {
    auto it = _impl->windows.find(id);
    if (it == _impl->windows.end()) return;

    Impl::Close(*it->second);
    _impl->PurgeEvents(id);
    _impl->windows.erase(it);
    _impl->idFactory.Free(id);
}

Extent2D DarwinWindowManager::GetClientSize(const std::uint64_t& id) {
    auto* info = _impl->Find(id);
    if (!info) return {};
    CGSize sz = [info->view convertSizeToBacking:info->view.bounds.size];
    return Extent2D{
        .width  = static_cast<std::uint32_t>(sz.width),
        .height = static_cast<std::uint32_t>(sz.height),
    };
}

Point2D DarwinWindowManager::GetClientPosition(const std::uint64_t& id) {
    auto* info = _impl->Find(id);
    if (!info) return {};

    NSRect frame       = [info->window frame];
    NSRect contentRect = [info->window contentRectForFrameRect:frame];
    NSScreen* primary  = [[NSScreen screens] firstObject];
    CGFloat screenH    = primary ? primary.frame.size.height : 0.0;

    return Point2D{
        .x = static_cast<int>(contentRect.origin.x),
        .y = static_cast<int>(screenH - contentRect.origin.y - contentRect.size.height),
    };
}

Vector2 DarwinWindowManager::GetCursorPosition(const std::uint64_t& id) {
    auto* info = _impl->Find(id);
    if (!info) return {};

    NSPoint global  = [NSEvent mouseLocation];
    NSPoint inWindow = [info->window convertPointFromScreen:global];
    NSPoint inView   = [info->view convertPoint:inWindow fromView:nil];
    // Window size/resize events report backing-store (physical pixel) extents via
    // convertSizeToBacking: - match that unit here so cursor position lands in the same
    // space as GetSize()/the render extent, instead of points.
    NSPoint inBacking = [info->view convertPointToBacking:inView];
    return Vector2{.x = static_cast<float>(inBacking.x), .y = static_cast<float>(inBacking.y)};
}

void DarwinWindowManager::Show(const std::uint64_t& id) {
    auto* info = _impl->Find(id);
    if (!info) return;
    [info->window makeKeyAndOrderFront:nil];
    ActivateApp();
}

void DarwinWindowManager::Hide(const std::uint64_t& id) {
    auto* info = _impl->Find(id);
    if (!info) return;
    [info->window orderOut:nil];
}

void DarwinWindowManager::Minimize(const std::uint64_t& id) {
    auto* info = _impl->Find(id);
    if (!info) return;
    [info->window miniaturize:nil];
}

void DarwinWindowManager::Maximize(const std::uint64_t& id) {
    auto* info = _impl->Find(id);
    if (!info) return;
    [info->window zoom:nil];
}

float DarwinWindowManager::GetDpi(const std::uint64_t& id) {
    auto* info = _impl->Find(id);
    if (!info) return GetDefaultDpi();
    NSScreen* screen = [info->window screen];
    if (!screen) return GetDefaultDpi();
    return 96.0f * static_cast<float>(screen.backingScaleFactor);
}

float DarwinWindowManager::GetDefaultDpi() {
    NSScreen* screen = [NSScreen mainScreen];
    if (!screen) return 96.0f;
    return 96.0f * static_cast<float>(screen.backingScaleFactor);
}

void DarwinWindowManager::PumpEvents() {
    // Texts of events still queued must stay readable, so only reset once the queue is empty.
    if (_impl->pendingEvents.empty()) {
        _impl->text.Reset();
    }
    @autoreleasepool {
        NSEvent* event;
        while ((event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                          untilDate:nil
                                             inMode:NSDefaultRunLoopMode
                                            dequeue:YES]) != nil) {
            [NSApp sendEvent:event];
        }
    }
}

void DarwinWindowManager::GetRequiredExtensions(std::vector<const char*>& extensions) {
    extensions.emplace_back("VK_KHR_surface");
    extensions.emplace_back("VK_EXT_metal_surface");
}

void DarwinWindowManager::SetHitTestCallback(const std::uint64_t& id,
    const std::function<HitTestResult(const Vector2&)>& callback) {
    auto* info = _impl->Find(id);
    if (!info) return;
    info->hitTestCallback = callback;
}

void DarwinWindowManager::ClearHitTestCallback(const std::uint64_t& id) {
    auto* info = _impl->Find(id);
    if (!info) return;
    info->hitTestCallback = {};
}

void DarwinWindowManager::SetDropCallbacks(const std::uint64_t& id,
    const DropCallbacks& callbacks) {
    auto* info = _impl->Find(id);
    if (!info) return;
    info->dropCallbacks = callbacks;
    [info->view registerForDraggedTypes:
        @[NSPasteboardTypeFileURL, NSPasteboardTypeString]];
}

void DarwinWindowManager::ClearDropCallbacks(const std::uint64_t& id) {
    auto* info = _impl->Find(id);
    if (!info) return;
    info->dropCallbacks = {};
    [info->view unregisterDraggedTypes];
}

void DarwinWindowManager::StartTextInput(const std::uint64_t& id, const Rect2D& caret) {
    auto* info = _impl->Find(id);
    if (!info) return;
    info->textInputActive = true;
    info->caretRect = caret;
    [info->view.inputContext invalidateCharacterCoordinates];
}

void DarwinWindowManager::StopTextInput(const std::uint64_t& id) {
    auto* info = _impl->Find(id);
    if (!info) return;
    info->textInputActive = false;
    if (info->view.hasMarkedText) {
        // Cleared first so the IME's teardown can't turn the abandoned composition into a commit.
        info->view.markedText = nil;
        [info->view.inputContext discardMarkedText];
    }
}

void DarwinWindowManager::SetTextInputRect(const std::uint64_t& id, const Rect2D& caret) {
    auto* info = _impl->Find(id);
    if (!info) return;
    info->caretRect = caret;
    [info->view.inputContext invalidateCharacterCoordinates];
}

std::u16string_view DarwinWindowManager::GetEventText(const TextRef& ref) {
    return _impl->text.View(ref);
}

} // namespace rwin

#endif // RWIN_PLATFORM_DARWIN
