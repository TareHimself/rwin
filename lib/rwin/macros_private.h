#pragma once

#ifndef RWIN_MACROS
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
