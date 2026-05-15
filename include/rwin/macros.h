#pragma once
#ifdef _WIN32
#ifdef RWIN_DX_PRODUCER
  #define RWIN_API __declspec(dllexport)
#else
  #define RWIN_API __declspec(dllimport)
#endif
#else
#define RWIN_API
#endif