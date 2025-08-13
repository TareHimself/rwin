#pragma once
#include <cstdint>
#include <list>
namespace rwin
{
    struct  IdFactory {
        std::uint64_t New();
        void Free(const std::uint64_t& id);
    private:
        std::uint64_t _id = 0;
        std::list<std::uint64_t> _freeIds{};
    };
}