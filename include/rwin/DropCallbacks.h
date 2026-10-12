#pragma once
#include <cstdint>
#include <functional>

#include "IDataView.h"
#include "flags.h"
#include "types.h"
namespace rwin
{
    // What will happen to the data if it is dropped here.
    enum class DropEffect : uint32_t
    {
        None = 0,
        Copy = 1,
        Move = 2,
        Link = 4,
    };

    struct DropInfo
    {
        Vector2 position; // window local, backing pixels
        Flags<DropEffect> allowed; // effects the dragging app permits
        Flags<InputModifier> modifiers;
    };

    // All four are optional. Returning DropEffect::None rejects the drop.
    struct DropCallbacks {
        std::function<DropEffect(const DropInfo&, const IDataView&)> enter;
        std::function<DropEffect(const DropInfo&, const IDataView&)> over;
        std::function<DropEffect(const DropInfo&, const IDataView&)> drop;
        std::function<void()> leave;
    };
}
