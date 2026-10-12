#pragma once
#include <cstdint>
#include <list>
namespace rwin
{
    struct  IdFactory {
        std::uint64_t New();
        void Free(const std::uint64_t& id);
    private:
        std::uint64_t _id = 1; // 0 is the invalid id
        std::list<std::uint64_t> _freeIds{};
    };
}