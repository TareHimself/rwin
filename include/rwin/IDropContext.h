#pragma once
#include "macros.h"
#include <vector>
#include <filesystem>
namespace rwin
{
    struct RWIN_API IDropContext
    {
        virtual ~IDropContext() = default;
        virtual bool HasFiles() = 0;
        virtual bool HasText() = 0;
        virtual bool GetFiles(std::vector<std::filesystem::path>& paths) = 0;
        virtual bool GetText(std::vector<std::string>& text) = 0;
    };
}