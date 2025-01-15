#ifndef CLP_UTF8_UTILS_HPP
#define CLP_UTF8_UTILS_HPP

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace clp {
// Constants
// Lead byte signature
constexpr uint8_t cTwoByteUtf8CharHeaderMask{0xE0};  // 0b111x_xxxx
constexpr uint8_t cTwoByteUtf8CharHeader{0xC0};  // 0b110x_xxxx
constexpr uint8_t cThreeByteUtf8CharHeaderMask{0xF0};  // 0b1111_xxxx
constexpr uint8_t cThreeByteUtf8CharHeader{0xE0};  // 0b1110_xxxx
constexpr uint8_t cFourByteUtf8CharHeaderMask{0xF8};  // 0b1111_1xxx
constexpr uint8_t cFourByteUtf8CharHeader{0xF0};  // 0b1111_0xxx

// Code point ranges (inclusive)
constexpr uint32_t cOneByteUtf8CharCodePointLowerBound{0};
constexpr uint32_t cOneByteUtf8CharCodePointUpperBound{0x7F};
constexpr uint32_t cTwoByteUtf8CharCodePointLowerBound{0x80};
constexpr uint32_t cTwoByteUtf8CharCodePointUpperBound{0x7FF};
constexpr uint32_t cThreeByteUtf8CharCodePointLowerBound{0x800};
constexpr uint32_t cThreeByteUtf8CharCodePointUpperBound{0xFFFF};
constexpr uint32_t cFourByteUtf8CharCodePointLowerBound{0x1'0000};
constexpr uint32_t cFourByteUtf8CharCodePointUpperBound{0x10'FFFF};

// Continuation byte
constexpr uint32_t cUtf8ContinuationByteMask{0xC0};
constexpr uint32_t cUtf8ContinuationByteHeader{0x80};
constexpr uint32_t cUtf8ContinuationByteCodePointMask{0x3F};
constexpr uint8_t cUtf8NumContinuationByteCodePointBits{6};

/**
 * Validates whether the given string is UTF-8 encoded, optionally escaping ASCII characters using
 * the given handler.
 * @tparam EscapeHandler Method to optionally escape any ASCII character in the string.
 * @param src
 * @param escape_handler
 * @return Whether the input is a valid UTF-8 encoded string.
 */
template <typename EscapeHandler>
requires std::is_invocable_v<EscapeHandler, std::string_view::const_iterator>
[[nodiscard]] auto validate_utf8_string(std::string_view src, EscapeHandler escape_handler) -> bool;

/**
 * @param str
 * @return Whether the input is a valid UTF-8 encoded string.
 */
[[nodiscard]] auto is_utf8_encoded(std::string_view str) -> bool;

namespace utf8_utils_internal {
/**
 * Validates whether the given byte is a valid lead byte for a multi-byte UTF-8 character, parses
 * the byte, and returns the parsed properties as well as associated properties.
 * @param byte Byte to validate.
 * @param num_continuation_bytes Returns the number of continuation bytes expected.
 * @param code_point Returns the code point bits parsed from the lead byte.
 * @param code_point_lower_bound Returns the lower bound of the code point range for the UTF-8
 * character.
 * @param code_point_upper_bound Returns the upper bound of the code point range for the UTF-8
 * character.
 * @return Whether the input byte is a valid lead byte for a multi-byte UTF-8 character.
 */
[[nodiscard]] auto parse_and_validate_lead_byte(
        uint8_t byte,
        size_t& num_continuation_bytes,
        uint32_t& code_point,
        uint32_t& code_point_lower_bound,
        uint32_t& code_point_upper_bound
) -> bool;

/**
 * @param byte
 * @return Whether the given byte is a valid ASCII character.
 */
[[nodiscard]] auto is_ascii_char(uint8_t byte) -> bool;

/*
 * @param byte
 * @return Whether the input byte is a valid UTF-8 continuation byte.
 */
[[nodiscard]] auto is_valid_utf8_continuation_byte(uint8_t byte) -> bool;

/**
 * Parses the code-point bits from the given continuation byte and combines them with the given
 * code point.
 * @param code_point
 * @param continuation_byte
 * @return The updated code point.
 */
[[nodiscard]] auto parse_continuation_byte(uint32_t code_point, uint8_t continuation_byte)
        -> uint32_t;
}  // namespace utf8_utils_internal

template <typename EscapeHandler>
requires std::is_invocable_v<EscapeHandler, std::string_view::const_iterator>
auto validate_utf8_string(std::string_view src, EscapeHandler escape_handler) -> bool {
    size_t num_continuation_bytes_to_validate{0};
    uint32_t code_point{};
    uint32_t code_point_lower_bound{};
    uint32_t code_point_upper_bound{};

    // NOLINTNEXTLINE(readability-qualified-auto)
    for (auto it{src.cbegin()}; it != src.cend(); ++it) {
        auto const byte{static_cast<uint8_t>(*it)};
        if (0 == num_continuation_bytes_to_validate) {
            if (utf8_utils_internal::is_ascii_char(byte)) {
                escape_handler(it);
            } else if (false
                       == utf8_utils_internal::parse_and_validate_lead_byte(
                               byte,
                               num_continuation_bytes_to_validate,
                               code_point,
                               code_point_lower_bound,
                               code_point_upper_bound
                       ))
            {
                return false;
            }
        } else {
            if (false == utf8_utils_internal::is_valid_utf8_continuation_byte(byte)) {
                return false;
            }
            code_point = utf8_utils_internal::parse_continuation_byte(code_point, byte);
            --num_continuation_bytes_to_validate;
            if (0 == num_continuation_bytes_to_validate
                && (code_point < code_point_lower_bound || code_point_upper_bound < code_point))
            {
                return false;
            }
        }
    }

    if (0 != num_continuation_bytes_to_validate) {
        // Incomplete UTF-8 character
        return false;
    }

    return true;
}
}  // namespace clp

#endif  // CLP_UTF8_UTILS_HPP
