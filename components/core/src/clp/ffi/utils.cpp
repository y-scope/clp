#include "utils.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>

using std::string;
using std::string_view;

namespace clp::ffi {
namespace {
/*
 * @param byte
 * @return Whether the input byte is a valid utf8 continuation byte. A valid utf8 continuation byte
 * should match 0b10xx_xxxx.
 */
[[nodiscard]] auto is_valid_utf8_continuation_byte(uint8_t byte) -> bool;

/**
 * Appends a single-byte utf8 character into the given string, and escapes it if necessary.
 * @param character Single-byte utf8 character.
 * @parma escaped_string Input string where the character(s) are appended to.
 */
auto escape_and_append_single_byte_utf8_char(uint8_t character, string& escaped_string) -> void;

/**
 * Validates whether the given code point is a valid UTF8 encoding with the given length.
 * The valid range is defined as following:
 * .---------------------------------------------.
 * | Length | First Code Point | Last Code Point |
 * |--------|------------------|-----------------|
 * | 1 Byte | 0x00             | 0x7F            |
 * | 2 Byte | 0x80             | 0x7FF           |
 * | 3 Byte | 0x8FF            | 0xFFFF          |
 * | 4 Byte | 0x10000          | 0x10FFFF        |
 * |--------|------------------|-----------------|
 * @param code_point
 * @param encoding_length
 * @return Whether the code point is a valid encoding.
 */
[[nodiscard]] auto is_valid_code_point(uint32_t code_point, size_t encoding_length) -> bool;

/**
 * Updates the code point by applying the payload of the given continuation byte.
 * @param code_point
 * @param continuation_byte
 * @return Updated code point.
 */
[[nodiscard]] auto update_code_point(uint32_t code_point, uint8_t continuation_byte) -> uint32_t;

auto is_valid_utf8_continuation_byte(uint8_t byte) -> bool {
    constexpr uint8_t cContinuationByteMask{0xC0};
    constexpr uint8_t cValidMaskedContinuationByte{0x80};
    return (byte & cContinuationByteMask) == cValidMaskedContinuationByte;
}

auto escape_and_append_single_byte_utf8_char(uint8_t character, string& escaped_string) -> void {
    switch (character) {
        // NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
        case 0x08:
            escaped_string.push_back('\\');
            escaped_string.push_back('b');
            break;
        case 0x09:
            escaped_string.push_back('\\');
            escaped_string.push_back('t');
            break;
        case 0x0A:
            escaped_string.push_back('\\');
            escaped_string.push_back('n');
            break;
        case 0x0C:
            escaped_string.push_back('\\');
            escaped_string.push_back('f');
            break;
        case 0x0D:
            escaped_string.push_back('\\');
            escaped_string.push_back('r');
            break;
        case 0x22:
            escaped_string.push_back('\\');
            escaped_string.push_back('\"');
            break;
        case 0x5C:
            escaped_string.push_back('\\');
            escaped_string.push_back('\\');
            break;
        // NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
        default: {
            constexpr uint8_t cControlCharacter{0x1F};
            if (cControlCharacter >= character) {
                // Allocate 6 + 1 size buffer to format control characters as "\u00bb", with the
                // last byte used by `snprintf` to append '\0'
                constexpr size_t cControlCharacterBufSize{7};
                std::array<char, cControlCharacterBufSize> buf{};
                std::ignore = snprintf(buf.data(), buf.size(), "\\u00%02x", character);
                escaped_string.append(buf.cbegin(), buf.cend() - 1);
            } else {
                escaped_string.push_back(static_cast<char>(character));
            }
            break;
        }
    }
}

auto is_valid_code_point(uint32_t code_point, size_t encoding_length) -> bool {
    switch (encoding_length) {
        // NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
        case 1:
            return code_point <= 0x7F;
        case 2:
            return (0x80 <= code_point && code_point <= 0x7FF);
        case 3:
            return (0x800 <= code_point && code_point <= 0xFFFF);
        case 4:
            return (0x1'0000 <= code_point && code_point <= 0x10'FFFF);
        // NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
        default:
            return false;
    }
}

auto update_code_point(uint32_t code_point, uint8_t continuation_byte) -> uint32_t {
    constexpr uint32_t cContinuationBytePayloadMask{0x3F};
    constexpr uint8_t cNumContinuationBytePayloadBits{6};
    return (code_point << cNumContinuationBytePayloadBits)
           + (continuation_byte & cContinuationBytePayloadMask);
}
}  // namespace

auto escape_utf8_string(string_view raw) -> std::optional<string> {
    string_view::const_iterator bookmark_it{};
    size_t encoding_length{};
    enum class State : uint8_t {
        HeadByteToValidate = 0,
        OneContinuationByteToValidate,
        TwoContinuationBytesToValidate,
        ThreeContinuationBytesToValidate
    };
    State state{State::HeadByteToValidate};
    string escaped;
    escaped.reserve(raw.size() + (raw.size() >> 2));

    uint32_t code_point{};
    auto validate_encoding_length_and_set_state
            = [&encoding_length, &state, &code_point](uint8_t byte) -> bool {
        constexpr uint8_t cThreeByteContinuationMask{0xF8};  // 0b1111_1xxx
        constexpr uint8_t cValidThreeByteContinuation{0xF0};  // 0b1111_0xxx
        constexpr uint8_t cTwoByteContinuationMask{0xF0};  // 0b1111_xxxx
        constexpr uint8_t cValidTwoByteContinuation{0xE0};  // 0b1110_xxxx
        constexpr uint8_t cOneByteContinuationMask{0xE0};  // 0b111x_xxxx
        constexpr uint8_t cValidOneByteContinuation{0xC0};  // 0b110x_xxxx
        if ((byte & cThreeByteContinuationMask) == cValidThreeByteContinuation) {
            encoding_length = 4;
            code_point = (~cThreeByteContinuationMask & byte);
            state = State::ThreeContinuationBytesToValidate;
        } else if ((byte & cTwoByteContinuationMask) == cValidTwoByteContinuation) {
            encoding_length = 3;
            code_point = (~cTwoByteContinuationMask & byte);
            state = State::TwoContinuationBytesToValidate;
        } else if ((byte & cOneByteContinuationMask) == cValidOneByteContinuation) {
            encoding_length = 2;
            code_point = (~cOneByteContinuationMask & byte);
            state = State::OneContinuationByteToValidate;
        } else {
            return false;
        }
        return true;
    };

    // For multibyte encoded values, we will incrementally build the code point, and validate its
    // range in the end.
    for (string_view::const_iterator it{raw.cbegin()}; it != raw.cend(); ++it) {
        auto const byte{static_cast<uint8_t>(*it)};
        switch (state) {
            case State::HeadByteToValidate: {
                if (is_valid_code_point(static_cast<uint32_t>(byte), 1)) {
                    escape_and_append_single_byte_utf8_char(byte, escaped);
                } else {
                    if (false == validate_encoding_length_and_set_state(byte)) {
                        return std::nullopt;
                    }
                    bookmark_it = it;
                }
                break;
            }
            case State::OneContinuationByteToValidate:
                if (false == is_valid_utf8_continuation_byte(byte)) {
                    return std::nullopt;
                }
                code_point = update_code_point(code_point, byte);

                if (false == is_valid_code_point(code_point, encoding_length)) {
                    return std::nullopt;
                }
                escaped.append(bookmark_it, bookmark_it + encoding_length);
                state = State::HeadByteToValidate;
                break;
            case State::TwoContinuationBytesToValidate:
                if (false == is_valid_utf8_continuation_byte(byte)) {
                    return std::nullopt;
                }
                code_point = update_code_point(code_point, byte);
                state = State::OneContinuationByteToValidate;
                break;
            case State::ThreeContinuationBytesToValidate:
                if (false == is_valid_utf8_continuation_byte(byte)) {
                    return std::nullopt;
                }
                code_point = update_code_point(code_point, byte);
                state = State::TwoContinuationBytesToValidate;
                break;
            default:
                return std::nullopt;
        }
    }

    if (State::HeadByteToValidate != state) {
        // Incomplete multibyte UTF8 sequence
        return std::nullopt;
    }

    return std::move(escaped);
}
}  // namespace clp::ffi
