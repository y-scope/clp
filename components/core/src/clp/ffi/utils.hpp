#ifndef CLP_FFI_UTILS_HPP
#define CLP_FFI_UTILS_HPP

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace clp::ffi {
/**
 * Validates whether the given string is UTF-8 encoded, and escapes any characters to make the
 * string compatible with the JSON specification.
 * @param raw The raw string to escape.
 * @return The escaped string on success.
 * @return std::nullopt if the string contains any non-UTF-8-encoded byte sequences.
 */
[[nodiscard]] auto validate_and_escape_utf8_string(std::string_view raw
) -> std::optional<std::string>;

/**
 * @param str
 * @return Whether the input is a valid UTF-8 encoded string.
 */
[[nodiscard]] auto is_utf8_encoded(std::string_view str) -> bool;

/**
 * Validates whether the given string is UTF-8 encoded, optionally escaping ASCII characters using
 * the given handler.
 * @tparam EscapeHandler Method to optionally escape any ASCII character in the string.
 * @param src
 * @param escape_handler
 * @return Whether the input is a valid UTF-8 encoded string.
 */
template <typename EscapeHandler>
[[nodiscard]] auto
generic_validate_utf8_string(std::string_view src, EscapeHandler escape_handler) -> bool;

namespace utils_hpp {
/**
 * Validates whether the given byte is a valid lead byte for a multi-byte UTF-8 character, parses
 * the byte, and returns the parsed properties as well as associated properties.
 * @param header Byte to validate.
 * @param num_continuation_bytes Returns the number of continuation bytes expected.
 * @param code_point Returns the code point bits parsed from the lead byte.
 * @param code_point_lower_bound Returns the lower bound of the code point range for the UTF-8
 * character.
 * @param code_point_upper_bound Returns the upper bound of the code point range for the UTF-8
 * character.
 * @return Whether the input byte is a valid lead byte for a multi-byte UTF-8 character.
 */
[[nodiscard]] auto validate_header_byte_and_set_code_point(
        uint8_t header,
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
[[nodiscard]] auto update_code_point(uint32_t code_point, uint8_t continuation_byte) -> uint32_t;
}  // namespace utils_hpp

template <typename EscapeHandler>
auto generic_validate_utf8_string(std::string_view src, EscapeHandler escape_handler) -> bool {
    size_t num_continuation_bytes_to_validate{0};
    uint32_t code_point{};
    uint32_t code_point_lower_bound{};
    uint32_t code_point_upper_bound{};

    for (std::string_view::const_iterator it{src.cbegin()}; it != src.cend(); ++it) {
        auto const byte{static_cast<uint8_t>(*it)};
        if (0 == num_continuation_bytes_to_validate) {
            if (utils_hpp::is_ascii_char(byte)) {
                escape_handler(it);
            } else {
                if (false
                    == utils_hpp::validate_header_byte_and_set_code_point(
                            byte,
                            num_continuation_bytes_to_validate,
                            code_point,
                            code_point_lower_bound,
                            code_point_upper_bound
                    ))
                {
                    return false;
                }
            }
        } else {
            if (false == utils_hpp::is_valid_utf8_continuation_byte(byte)) {
                return false;
            }
            code_point = utils_hpp::update_code_point(code_point, byte);
            --num_continuation_bytes_to_validate;
            if (0 != num_continuation_bytes_to_validate) {
                continue;
            }
            if (code_point < code_point_lower_bound || code_point_upper_bound < code_point) {
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
}  // namespace clp::ffi

#endif  // CLP_UTILS_HPP
