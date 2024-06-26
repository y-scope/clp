#include "utils.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>

#include "../utf8_utils.hpp"

using std::string;
using std::string_view;

namespace clp::ffi {
auto validate_and_escape_utf8_string(string_view raw) -> std::optional<string> {
    string_view::const_iterator next_char_to_copy_it{raw.cbegin()};
    std::optional<std::string> ret_val;
    auto& escaped{ret_val.emplace()};
    escaped.reserve(raw.size() + (raw.size() / 2));

    auto escape_handler = [&](string_view::const_iterator it) -> void {
        // Allocate 6 + 1 size buffer to format control characters as "\u00bb", with the last byte
        // used by `snprintf` to append '\0'
        constexpr size_t cControlCharacterBufSize{7};
        std::array<char, cControlCharacterBufSize> buf{};
        std::string_view escaped_char;
        bool escape_required{true};
        switch (*it) {
            case '\b':
                escaped_char = "\\b";
                break;
            case '\t':
                escaped_char = "\\t";
                break;
            case '\n':
                escaped_char = "\\n";
                break;
            case '\f':
                escaped_char = "\\f";
                break;
            case '\r':
                escaped_char = "\\r";
                break;
            case '\\':
                escaped_char = "\\\\";
                break;
            case '"':
                escaped_char = "\\\"";
                break;
            default: {
                constexpr uint8_t cLargestControlCharacter{0x1F};
                auto const byte{static_cast<uint8_t>(*it)};
                if (cLargestControlCharacter >= byte) {
                    std::ignore = snprintf(buf.data(), buf.size(), "\\u00%02x", byte);
                    escaped_char = {buf.data(), buf.size() - 1};
                } else {
                    escape_required = false;
                }
                break;
            }
        }
        if (escape_required) {
            escaped.append(next_char_to_copy_it, it);
            escaped += escaped_char;
            next_char_to_copy_it = it + 1;
        }
    };

    if (false == validate_utf8_string(raw, escape_handler)) {
        return std::nullopt;
    }

    if (raw.cend() != next_char_to_copy_it) {
        escaped.append(next_char_to_copy_it, raw.cend());
    }

    return ret_val;
}
}  // namespace clp::ffi
