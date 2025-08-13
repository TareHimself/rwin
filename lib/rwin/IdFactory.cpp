#include "rwin/IdFactory.h"
namespace rwin
{
    std::uint64_t IdFactory::New()
    {
        if (_freeIds.empty())
        {
            return _id++;
        }

        {
            const auto id = _freeIds.back();
            _freeIds.pop_back();
            return id;
        }
    }

    void IdFactory::Free(const std::uint64_t& id)
    {
        _freeIds.push_back(id);
    }
}
