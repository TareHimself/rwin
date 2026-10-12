#pragma once
#include "rwin/DropCallbacks.h"

namespace rwin
{
    inline DropEffect FilterDropEffect(const DropEffect effect, const Flags<DropEffect>& allowed)
    {
        return allowed.Has(effect) ? effect : DropEffect::None;
    }

    inline DropEffect DispatchDropEnter(const DropCallbacks* callbacks, const DropInfo& info, const IDataView& data)
    {
        if (!callbacks)
        {
            return DropEffect::None;
        }
        if (callbacks->enter)
        {
            return FilterDropEffect(callbacks->enter(info, data), info.allowed);
        }
        return callbacks->drop ? FilterDropEffect(DropEffect::Copy, info.allowed) : DropEffect::None;
    }

    inline DropEffect DispatchDropOver(const DropCallbacks* callbacks, const DropInfo& info, const IDataView& data,
                                       const DropEffect enterEffect)
    {
        if (!callbacks)
        {
            return DropEffect::None;
        }
        return FilterDropEffect(callbacks->over ? callbacks->over(info, data) : enterEffect, info.allowed);
    }

    inline DropEffect DispatchDrop(const DropCallbacks* callbacks, const DropInfo& info, const IDataView& data)
    {
        if (!callbacks || !callbacks->drop)
        {
            return DropEffect::None;
        }
        return FilterDropEffect(callbacks->drop(info, data), info.allowed);
    }

    inline void DispatchDropLeave(const DropCallbacks* callbacks)
    {
        if (callbacks && callbacks->leave)
        {
            callbacks->leave();
        }
    }
}
