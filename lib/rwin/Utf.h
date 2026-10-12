#pragma once
#include <cstddef>
#include <string>
#include <string_view>

namespace rwin
{
    inline std::u16string Utf8ToUtf16(const std::string_view text)
    {
        std::u16string out;
        for (std::size_t i = 0; i < text.size();)
        {
            const auto lead = static_cast<unsigned char>(text[i]);
            std::size_t length = 1;
            char32_t cp = lead;
            if (lead >= 0xF0)
            {
                length = 4;
                cp = lead & 0x07;
            }
            else if (lead >= 0xE0)
            {
                length = 3;
                cp = lead & 0x0F;
            }
            else if (lead >= 0xC0)
            {
                length = 2;
                cp = lead & 0x1F;
            }

            if (i + length > text.size())
            {
                break;
            }
            for (std::size_t j = 1; j < length; ++j)
            {
                cp = (cp << 6) | (static_cast<unsigned char>(text[i + j]) & 0x3F);
            }
            i += length;

            if (cp >= 0x10000)
            {
                cp -= 0x10000;
                out += static_cast<char16_t>(0xD800 + (cp >> 10));
                out += static_cast<char16_t>(0xDC00 + (cp & 0x3FF));
            }
            else
            {
                out += static_cast<char16_t>(cp);
            }
        }
        return out;
    }

    inline std::string Utf16ToUtf8(const std::u16string_view text)
    {
        std::string out;
        for (std::size_t i = 0; i < text.size(); ++i)
        {
            char32_t cp = text[i];
            if (cp >= 0xD800 && cp < 0xDC00 && i + 1 < text.size() && text[i + 1] >= 0xDC00 && text[i + 1] < 0xE000)
            {
                cp = 0x10000 + ((cp - 0xD800) << 10) + (text[++i] - 0xDC00);
            }

            if (cp < 0x80)
            {
                out += static_cast<char>(cp);
            }
            else if (cp < 0x800)
            {
                out += static_cast<char>(0xC0 | (cp >> 6));
                out += static_cast<char>(0x80 | (cp & 0x3F));
            }
            else if (cp < 0x10000)
            {
                out += static_cast<char>(0xE0 | (cp >> 12));
                out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                out += static_cast<char>(0x80 | (cp & 0x3F));
            }
            else
            {
                out += static_cast<char>(0xF0 | (cp >> 18));
                out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
                out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                out += static_cast<char>(0x80 | (cp & 0x3F));
            }
        }
        return out;
    }
}
