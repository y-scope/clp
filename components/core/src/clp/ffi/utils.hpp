#ifndef CLP_FFI_UTILS_HPP
#define CLP_FFI_UTILS_HPP

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace clp::ffi {
/**
 * Validates whether the given string is UTF-8 encoded, and escapes any characters to generate to
 * make the string human readable.
 * @param raw The raw string to escape.
 * @return The escaped string on success.
 * @return std::nullopt if the string contains none-UTF8 encoded byte sequence.
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
 * @tparam EscapeHandler Method to optionally escape any ASCII character in the string. Signature:
 * (std::string_view::const_iterator it_ascii_char) -> void
 * @param src
 * @param escape_handler
 * @return Whether the input is a valid UTF-8 encoded string.
 */
template <typename EscapeHandler>
[[nodiscard]] auto
generic_validate_utf8_string(std::string_view src, EscapeHandler escape_handler) -> bool;

namespace utils_hpp {
/**
 * Validates whether the given byte is a valid UTF-8 header with continuation bytes, and set code
 * point and code point range accordingly.
 * The valid code point range is defined as following:
 * .----------------------------------------------------------.
 * | Continuation Length | First Code Point | Last Code Point |
 * |---------------------|------------------|-----------------|
 * | 1 Byte              | 0x80             | 0x7FF           |
 * | 2 Byte              | 0x800            | 0xFFFF          |
 * | 3 Byte              | 0x10000          | 0x10FFFF        |
 * |---------------------|------------------|-----------------|
 * @param header Input byte to validate
 * @param num_continuation_bytes Outputs the number of continuation bytes corresponded to the header
 * byte, if the header is valid.
 * @param code_point Outputs the code extracted from the header byte, if the header is valid.
 * @param code_point_lower_bound Outputs the lower bound of the valid code point range corresponded
 * with the header byte, if the header if valid.
 * @param code_point_upper_bound Outputs the upper bound of the valid code point range corresponded
 * with the header byte, if the header if valid.
 * @return Whether the input byte is a valid header byte.
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
 * @return Whether the input byte is a valid UTF-8 continuation byte. A valid UTF-8 continuation
 * byte should match 0b10xx_xxxx.
 */
[[nodiscard]] auto is_valid_utf8_continuation_byte(uint8_t byte) -> bool;

/**
 * Updates the code point by applying the payload of the given continuation byte.
 * @param code_point
 * @param continuation_byte
 * @return Updated code point.
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
        // Incomplete continuation byte sequence
        return false;
    }

    return true;
}
}  // namespace clp::ffi

#endif  // CLP_UTILS_HPP
