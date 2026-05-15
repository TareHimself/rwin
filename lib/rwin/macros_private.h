#pragma once

#if !defined(RWIN_MACROS) && !defined(RWIN_PLATFORM_COMPAT)
#define RWIN_MACROS
    #ifdef _WIN32
    #define RWIN_PLATFORM_WIN
    #endif

    #ifdef __APPLE__
    #define RWIN_PLATFORM_DARWIN
    #endif

    #ifdef __linux__
    #define RWIN_PLATFORM_LINUX
    #define RWIN_PLATFORM_LINUX_WAYLAND
    #endif
#endif
