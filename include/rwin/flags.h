#pragma once
#include <type_traits>

namespace rwin
{
    template <typename T, std::enable_if_t<std::is_enum_v<T> && std::is_arithmetic_v<std::underlying_type_t<T>>>* =
                  nullptr>
    struct Flags
    {
        using ContainedType = std::underlying_type_t<T>;

    private:
        ContainedType _value{0};

    public:
        Flags() = default;

        Flags(const ContainedType& value) : _value(value)
        {
        }

        Flags(const T& value) : _value(static_cast<ContainedType>(value))
        {
        }

        explicit operator ContainedType() const
        {
            return _value;
        }

        explicit operator T() const
        {
            return static_cast<T>(_value);
        }

        Flags& operator&(const T& other)
        {
            _value &= static_cast<ContainedType>(other);
            return *this;
        }

        Flags& operator|(const T& other)
        {
            _value |= static_cast<ContainedType>(other);
            return *this;
        }

        bool Has(const T& value) const
        {
            return _value & static_cast<ContainedType>(value);
        }

        Flags& Add(const T& value)
        {
            _value |= static_cast<ContainedType>(value);
            return *this;
        }

        Flags& Remove(const T& value)
        {
            _value &= ~static_cast<ContainedType>(value);
            return *this;
        }

        Flags& operator|=(const T& value)
        {
            _value |= static_cast<ContainedType>(value);
            return *this;
        }

        Flags& operator&=(const T& value)
        {
            _value &= static_cast<ContainedType>(value);
            return *this;
        }
    };

#ifdef RWIN_FLAGS_OPERATORS
    template <typename T>
    constexpr std::enable_if_t<std::is_enum_v<T> && std::is_arithmetic_v<std::underlying_type_t<T>>, Flags<T>>
    operator&(T lhs, T rhs)
    {
        return Flags<T>(lhs) & rhs;
    }

    template <typename T>
    constexpr std::enable_if_t<std::is_enum_v<T> && std::is_arithmetic_v<std::underlying_type_t<T>>, Flags<T>>
    operator|(T lhs, T rhs)
    {
        return Flags<T>(lhs) | rhs;
    }
#endif
}
