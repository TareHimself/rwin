#pragma once

#ifndef RWIN_MACROS
#define RWIN_MACROS

    #ifdef _WIN32
    #define RWIN_PLATFORM_WIN
    #endif

    #ifdef __APPLE__
    #define RWIN_PLATFORM_MAC
    #endif

    #ifdef __linux__
    #define RWIN_PLATFORM_LINUX
    #endif

    #ifdef _WIN32
      #ifdef RWIN_DX_PRODUCER
        #define RWIN_API __declspec(dllexport)
      #else
        #define RWIN_API __declspec(dllimport)
      #endif
    #else
      #define RWIN_API
    #endif
#endif
