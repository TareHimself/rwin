#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

#include "rwin/types.h"

namespace rwin {
    // Backing store for the text carried by TextCommit/TextPreedit events. Events stay plain data
    // and point into this buffer by offset rather than pointer, so it can grow while events are
    // still queued. Backends reset it once the queue has drained.
    class TextArena {
    public:
        TextRef Append(const std::u16string_view text) {
            const auto ref = Reserve(static_cast<std::uint32_t>(text.size()));
            std::copy(text.begin(), text.end(), _chars.begin() + ref.offset);
            return ref;
        }

        // Reserves `length` code units for the caller to fill through Data().
        TextRef Reserve(const std::uint32_t length) {
            const auto offset = static_cast<std::uint32_t>(_chars.size());
            _chars.resize(_chars.size() + length);
            return TextRef{.offset = offset, .length = length};
        }

        char16_t* Data(const TextRef& ref) { return _chars.data() + ref.offset; }

        std::u16string_view View(const TextRef& ref) const {
            if (static_cast<std::size_t>(ref.offset) + ref.length > _chars.size()) {
                return {};
            }
            return {_chars.data() + ref.offset, ref.length};
        }

        void Reset() noexcept { _chars.clear(); }

    private:
        std::vector<char16_t> _chars{};
    };
}
