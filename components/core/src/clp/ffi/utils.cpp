#include "utils.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>

using std::string;
using std::string_view;

namespace clp::ffi {
auto validate_and_escape_utf8_string(string_view raw) -> std::optional<string> {
    string_view::const_iterator bookmark{raw.cbegin()};
    string escaped;
    escaped.reserve(raw.size() + (raw.size() / 2));

    auto escape_handler = [&](string_view::const_iterator it) -> void {
        // Allocate 6 + 1 size buffer to format control characters as "\u00bb", with the last byte
        // used by `snprintf` to append '\0'
        constexpr size_t cControlCharacterBufSize{7};
        std::array<char, cControlCharacterBufSize> buf{};
        std::string_view escaped_content;
        bool escape_required{true};
        switch (*it) {
            case '\b':
                escaped_content = "\\b";
                break;
            case '\t':
                escaped_content = "\\t";
                break;
            case '\n':
                escaped_content = "\\n";
                break;
            case '\f':
                escaped_content = "\\f";
                break;
            case '\r':
                escaped_content = "\\r";
                break;
            case '\\':
                escaped_content = "\\\\";
                break;
            case '"':
                escaped_content = "\\\"";
                break;
            default: {
                constexpr uint8_t cLargestControlCharacter{0x1F};
                auto const byte{static_cast<uint8_t>(*it)};
                if (cLargestControlCharacter >= byte) {
                    std::ignore = snprintf(buf.data(), buf.size(), "\\u00%02x", byte);
                    escaped_content = {buf.data(), buf.size() - 1};
                } else {
                    escape_required = false;
                }
                break;
            }
        }
        if (escape_required) {
            escaped.append(bookmark, it);
            escaped.append(escaped_content.cbegin(), escaped_content.cend());
            bookmark = it + 1;
        }
    };

    if (false == generic_validate_utf8_string(raw, escape_handler)) {
        return std::nullopt;
    }

    if (raw.cend() != bookmark) {
        escaped.append(bookmark, raw.cend());
    }

    return escaped;
}

auto is_utf8_encoded(string_view str) -> bool {
    auto escape_handler = []([[maybe_unused]] string_view::const_iterator it) -> void {};
    return generic_validate_utf8_string(str, escape_handler);
}

namespace utils_hpp {
auto validate_header_byte_and_set_code_point(
        uint8_t header,
        size_t& num_continuation_bytes,
        uint32_t& code_point,
        uint32_t& code_point_lower_bound,
        uint32_t& code_point_upper_bound
) -> bool {
    constexpr uint8_t cThreeByteContinuationMask{0xF8};  // 0b1111_1xxx
    constexpr uint8_t cValidThreeByteContinuation{0xF0};  // 0b1111_0xxx
    constexpr uint8_t cTwoByteContinuationMask{0xF0};  // 0b1111_xxxx
    constexpr uint8_t cValidTwoByteContinuation{0xE0};  // 0b1110_xxxx
    constexpr uint8_t cOneByteContinuationMask{0xE0};  // 0b111x_xxxx
    constexpr uint8_t cValidOneByteContinuation{0xC0};  // 0b110x_xxxx

    if ((header & cThreeByteContinuationMask) == cValidThreeByteContinuation) {
        num_continuation_bytes = 3;
        code_point = (~cThreeByteContinuationMask & header);
        // NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
        code_point_lower_bound = 0x1'0000;
        code_point_upper_bound = 0x10'FFFF;
        // NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
    } else if ((header & cTwoByteContinuationMask) == cValidTwoByteContinuation) {
        num_continuation_bytes = 2;
        code_point = (~cTwoByteContinuationMask & header);
        // NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
        code_point_lower_bound = 0x800;
        code_point_upper_bound = 0xFFFF;
        // NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
    } else if ((header & cOneByteContinuationMask) == cValidOneByteContinuation) {
        num_continuation_bytes = 1;
        code_point = (~cOneByteContinuationMask & header);
        // NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
        code_point_lower_bound = 0x80;
        code_point_upper_bound = 0x7FF;
        // NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
    } else {
        return false;
    }
    return true;
}

auto is_ascii_char(uint8_t byte) -> bool {
    constexpr uint8_t cLargestValidASCIIChar{0x7F};
    return cLargestValidASCIIChar >= byte;
}

auto is_valid_utf8_continuation_byte(uint8_t byte) -> bool {
    constexpr uint8_t cContinuationByteMask{0xC0};
    constexpr uint8_t cValidMaskedContinuationByte{0x80};
    return (byte & cContinuationByteMask) == cValidMaskedContinuationByte;
}

auto update_code_point(uint32_t code_point, uint8_t continuation_byte) -> uint32_t {
    constexpr uint32_t cContinuationBytePayloadMask{0x3F};
    constexpr uint8_t cNumContinuationBytePayloadBits{6};
    return (code_point << cNumContinuationBytePayloadBits)
           + (continuation_byte & cContinuationBytePayloadMask);
}
}  // namespace utils_hpp

}  // namespace clp::ffi
