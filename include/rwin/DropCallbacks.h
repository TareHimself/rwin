#pragma once
#include <cstdint>
#include <functional>

#include "IDropContext.h"
#include "types.h"
namespace rwin
{
    struct DropCallbacks {
        std::function<bool(const Vector2&,IDropContext *)> enter;
        std::function<bool(const Vector2&,IDropContext *)> over;
        std::function<void(const Vector2&,IDropContext *)> drop;
        std::function<void()> leave;
    };
}