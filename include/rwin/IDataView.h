#pragma once
#include <filesystem>
#include <string>
#include <vector>
#include "macros.h"
namespace rwin
{
    // Data another app is offering. Only valid inside the callback that received it.
    struct RWIN_API IDataView
    {
        virtual ~IDataView() = default;
        virtual bool HasText() const = 0;
        virtual bool HasFiles() const = 0;
        virtual bool GetText(std::u16string& out) const = 0;
        virtual bool GetFiles(std::vector<std::filesystem::path>& out) const = 0;
    };
}
