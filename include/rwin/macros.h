#pragma once
#if defined(_WIN32) && !defined(RWIN_STATIC)
#ifdef RWIN_DX_PRODUCER
  #define RWIN_API __declspec(dllexport)
#else
  #define RWIN_API __declspec(dllimport)
#endif
#else
#define RWIN_API
#endif