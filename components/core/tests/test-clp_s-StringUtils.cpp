#include <cstring>
#include <string>
#include <string_view>

#include <catch2/catch_test_macros.hpp>
#include <simdjson.h>

#include "../src/clp_s/Utils.hpp"

using clp_s::JsonSanitizeResult;
using clp_s::StringUtils;

// We use C++14 string literals (the `s` suffix) to construct strings containing embedded null bytes
// and control characters. Without the `s` suffix, std::string's constructor from a C-string literal
// stops at the first null byte, truncating the test input. Additionally, we use string
// concatenation (e.g., "\x00" "end") to prevent hex escape sequences from being extended by
// following hex digits.
using namespace std::string_literals;

namespace {
/**
 * Helper to create a buffer with simdjson padding and run sanitization.
 * @param input The input string to sanitize
 * @return The sanitized string
 * @note If sanitize_json_buffer reallocates the buffer, the new buffer pointer is returned
 *       and the original buffer is automatically deleted by sanitize_json_buffer. This helper
 *       correctly handles both cases (reallocation and no reallocation) by always deleting
 *       the final buffer pointer.
 */
auto sanitize_string(std::string_view input) -> std::string {
    size_t buf_size = input.size();
    size_t buf_occupied = input.size();
    char* buf = new char[buf_size + simdjson::SIMDJSON_PADDING];
    std::memcpy(buf, input.data(), input.size());

    try {
        auto const result = StringUtils::sanitize_json_buffer(
                buf,
                buf_size,
                buf_occupied,
                simdjson::SIMDJSON_PADDING
        );
        std::string output(buf, result.new_buf_occupied);
        // Delete the buffer (may be reallocated, but buf points to the current buffer)
        delete[] buf;
        return output;
    } catch (...) {
        // If sanitization failed, buf still points to the original buffer
        delete[] buf;
        throw;
    }
}

/**
 * Helper to verify that a sanitized JSON string can be parsed by simdjson.
 * @param json The JSON string to parse
 * @return true if parsing succeeds, false otherwise
 */
auto can_parse_json(std::string_view json) -> bool {
    simdjson::ondemand::parser parser;
    simdjson::padded_string padded(json);
    auto result = parser.iterate(padded);
    return false == result.error();
}
}  // namespace

TEST_CASE("sanitize_json_buffer_no_changes", "[clp_s][StringUtils]") {
    // Valid JSON without control characters should pass through unchanged
    SECTION("simple object") {
        std::string input = R"({"key": "value"})";
        REQUIRE(sanitize_string(input) == input);
    }

    SECTION("object with escaped characters") {
        std::string input = R"({"key": "line1\nline2\ttab"})";
        REQUIRE(sanitize_string(input) == input);
    }

    SECTION("object with unicode escapes") {
        std::string input = R"({"key": "null char: \u0000"})";
        REQUIRE(sanitize_string(input) == input);
    }

    SECTION("multiple objects") {
        std::string input = R"({"a": "1"}{"b": "2"})";
        REQUIRE(sanitize_string(input) == input);
    }

    SECTION("control chars outside strings are unchanged") {
        // Control chars outside strings aren't valid JSON anyway,
        // but we don't touch them - let the JSON parser report the error
        std::string input = "{\x01\"key\": \"value\"}";
        REQUIRE(sanitize_string(input) == input);
    }
}

TEST_CASE("sanitize_json_buffer_escapes_control_chars", "[clp_s][StringUtils]") {
    SECTION("null byte in string value") {
        auto input = "{\"key\": \"val\x00ue\"}"s;
        std::string expected = R"({"key": "val\u0000ue"})";
        REQUIRE(sanitize_string(input) == expected);
    }

    SECTION("multiple null bytes") {
        auto input = "{\"key\": \"\x00\x00\x00\"}"s;
        std::string expected = R"({"key": "\u0000\u0000\u0000"})";
        REQUIRE(sanitize_string(input) == expected);
    }

    SECTION("various control characters") {
        // Test 0x01 (SOH), 0x02 (STX), 0x1F (US)
        auto input = "{\"key\": \"a\x01"
                     "b\x02"
                     "c\x1F"
                     "d\"}"s;
        std::string expected = R"({"key": "a\u0001b\u0002c\u001fd"})";
        REQUIRE(sanitize_string(input) == expected);
    }

    SECTION("control char in key") {
        auto input = "{\"ke\x00y\": \"value\"}"s;
        std::string expected = R"({"ke\u0000y": "value"})";
        REQUIRE(sanitize_string(input) == expected);
    }

    SECTION("mixed valid escapes and control chars") {
        auto input = "{\"key\": \"line1\\nhas\x00null\"}"s;
        std::string expected = R"({"key": "line1\nhas\u0000null"})";
        REQUIRE(sanitize_string(input) == expected);
    }
}

TEST_CASE("sanitize_json_buffer_handles_escapes_correctly", "[clp_s][StringUtils]") {
    SECTION("escaped quote should not toggle string state") {
        // The \" should not end the string
        std::string input = R"({"key": "quote:\"value"})";
        REQUIRE(sanitize_string(input) == input);
    }

    SECTION("escaped backslash before quote") {
        // \\" means escaped backslash followed by end of string
        std::string input = R"({"key": "backslash:\\"})";
        REQUIRE(sanitize_string(input) == input);
    }

    SECTION("control char after escaped backslash") {
        // \\\x00 means escaped backslash followed by a control char still inside the string
        auto input = "{\"key\": \"slash:\\\\\x00"
                     "end\"}"s;
        std::string expected = R"({"key": "slash:\\\u0000end"})";
        REQUIRE(sanitize_string(input) == expected);
    }

    SECTION("control char after escaped quote") {
        auto input = "{\"key\": \"quote:\\\"\x00"
                     "after\"}"s;
        std::string expected = R"({"key": "quote:\"\u0000after"})";
        REQUIRE(sanitize_string(input) == expected);
    }

    SECTION("control char as invalid escape sequence target") {
        // \<control-char> is not a valid JSON escape sequence, but we leave it as-is
        // since the sanitizer treats any char after backslash as an escape target.
        // This JSON is invalid anyway (bad escape sequence) and will fail parsing.
        auto input = "{\"key\": \"bad:\\\x00"
                     "end\"}"s;
        auto expected = "{\"key\": \"bad:\\\x00"
                        "end\"}"s;
        REQUIRE(sanitize_string(input) == expected);
    }

    SECTION("triple backslash followed by control char") {
        // \\\ followed by \x00: first \\ is escaped backslash, third \ starts escape sequence.
        // The third backslash makes the null appear to be an escape target, so it's not escaped.
        auto input = "{\"key\": \"\\\\\\\x00"
                     "end\"}"s;
        auto expected = "{\"key\": \"\\\\\\\x00"
                        "end\"}"s;
        REQUIRE(sanitize_string(input) == expected);
    }
}

TEST_CASE("sanitize_json_buffer_result_is_valid_json", "[clp_s][StringUtils]") {
    SECTION("sanitized null byte produces valid JSON") {
        auto input = "{\"key\": \"val\x00ue\"}"s;
        std::string sanitized = sanitize_string(input);
        REQUIRE(can_parse_json(sanitized));
    }

    SECTION("sanitized multiple control chars produces valid JSON") {
        auto input = "{\"data\": \"\x01\x02\x03\x04\x05\"}"s;
        std::string sanitized = sanitize_string(input);
        REQUIRE(can_parse_json(sanitized));
    }
}

TEST_CASE("sanitize_json_buffer_handles_buffer_growth", "[clp_s][StringUtils]") {
    // Each control char expands from 1 byte to 6 bytes (\u00XX)
    // Create input that will require buffer growth
    SECTION("many control chars requiring expansion") {
        // 100 null bytes -> 600 bytes after escaping
        std::string value(100, '\x00');
        std::string input = "{\"key\": \"" + value + "\"}";

        std::string sanitized = sanitize_string(input);

        // Verify all nulls were escaped
        REQUIRE(sanitized.find('\x00') == std::string::npos);
        // Verify correct number of escape sequences
        size_t count = 0;
        size_t pos = 0;
        while ((pos = sanitized.find("\\u0000", pos)) != std::string::npos) {
            ++count;
            pos += 6;
        }
        REQUIRE(count == 100);
        // Verify result is valid JSON
        REQUIRE(can_parse_json(sanitized));
    }
}

TEST_CASE("sanitize_json_buffer_jsonl", "[clp_s][StringUtils]") {
    SECTION("multiple JSON objects with control chars") {
        auto input = "{\"a\": \"x\x00y\"}\n{\"b\": \"p\x01q\"}"s;
        std::string expected = "{\"a\": \"x\\u0000y\"}\n{\"b\": \"p\\u0001q\"}";
        REQUIRE(sanitize_string(input) == expected);
    }
}

TEST_CASE("sanitize_json_buffer_truncated_json", "[clp_s][StringUtils]") {
    SECTION("truncated JSON with unmatched quote - control chars may be escaped incorrectly") {
        // This tests the edge case documented in the code: if JSON is truncated with unmatched
        // quotes, string state tracking may be incorrect, potentially escaping control chars
        // outside of actual JSON strings. This is acceptable since malformed JSON will fail
        // parsing anyway.
        //
        // Input: truncated JSON where the string is not closed, followed by control chars
        // The sanitizer may incorrectly think it's still in a string and escape the control chars
        auto input = "{\"key\": \"unclosed string\x00\x01"
                     "after\"}"s;
        // The sanitizer will escape the control chars because it thinks we're still in a string
        // (the quote after "unclosed string" is escaped, so the string continues)
        std::string sanitized = sanitize_string(input);
        // The control chars should be escaped (behavior may vary based on quote matching)
        // The important thing is that the function doesn't crash and handles it gracefully
        REQUIRE(
                (sanitized.find('\x00') == std::string::npos
                 || sanitized.find("\\u0000") != std::string::npos)
        );
    }

    SECTION("truncated JSON ending mid-string") {
        // JSON truncated in the middle of a string with control chars (no closing quote)
        // This simulates a real truncation scenario where the buffer ends mid-string
        auto input = "{\"key\": \"value\x00\x01"
                     "truncated"s;
        std::string sanitized = sanitize_string(input);
        // Should handle gracefully without crashing
        // Control chars inside the (unclosed) string should be escaped
        REQUIRE(sanitized.size() >= input.size());
        // Verify no raw control characters remain (they should be escaped)
        REQUIRE(sanitized.find('\x00') == std::string::npos);
        REQUIRE(sanitized.find('\x01') == std::string::npos);
    }

    SECTION("truncated JSON with control chars after unmatched quote") {
        // JSON with unmatched quote followed by control chars outside the string
        // The sanitizer may incorrectly escape these if it thinks we're still in a string
        auto input = "{\"key\": \"value\x00"
                     "}\x01\x02"s;
        std::string sanitized = sanitize_string(input);
        // Function should not crash - behavior may vary but should be consistent
        REQUIRE(sanitized.size() >= input.size());
    }
}

// =====================================================================================
// UTF-8 Sanitization Tests
// =====================================================================================

namespace {
/**
 * Helper to create a buffer with simdjson padding and run UTF-8 sanitization.
 * @param input The input string to sanitize
 * @return The sanitized string
 */
auto sanitize_utf8_string(std::string_view input) -> std::string {
    size_t buf_size = input.size();
    size_t buf_occupied = input.size();
    char* buf = new char[buf_size + simdjson::SIMDJSON_PADDING];
    std::memcpy(buf, input.data(), input.size());

    try {
        auto const result = StringUtils::sanitize_utf8_buffer(
                buf,
                buf_size,
                buf_occupied,
                simdjson::SIMDJSON_PADDING
        );
        std::string output(buf, result.new_buf_occupied);
        delete[] buf;
        return output;
    } catch (...) {
        delete[] buf;
        throw;
    }
}

/**
 * Count occurrences of U+FFFD replacement character (0xEF 0xBF 0xBD) in a string.
 */
auto count_replacement_chars(std::string_view s) -> size_t {
    size_t count = 0;
    for (size_t i = 0; i + 2 < s.size(); ++i) {
        if (static_cast<unsigned char>(s[i]) == 0xEF && static_cast<unsigned char>(s[i + 1]) == 0xBF
            && static_cast<unsigned char>(s[i + 2]) == 0xBD)
        {
            ++count;
            i += 2;  // Skip past the replacement char
        }
    }
    return count;
}
}  // namespace

TEST_CASE("sanitize_utf8_buffer_no_changes", "[clp_s][StringUtils]") {
    SECTION("valid ASCII") {
        std::string input = "Hello, World!";
        REQUIRE(sanitize_utf8_string(input) == input);
    }

    SECTION("valid UTF-8 multibyte characters") {
        // 2-byte: é (U+00E9) = 0xC3 0xA9
        // 3-byte: € (U+20AC) = 0xE2 0x82 0xAC
        // 4-byte: 𝄞 (U+1D11E) = 0xF0 0x9D 0x84 0x9E
        std::string input = "café € 𝄞";
        REQUIRE(sanitize_utf8_string(input) == input);
    }

    SECTION("valid JSON with UTF-8") {
        std::string input = R"({"msg": "日本語テスト"})";
        REQUIRE(sanitize_utf8_string(input) == input);
    }

    SECTION("empty string") {
        std::string input = "";
        REQUIRE(sanitize_utf8_string(input) == input);
    }
}

TEST_CASE("sanitize_utf8_buffer_replaces_invalid_bytes", "[clp_s][StringUtils]") {
    // U+FFFD is encoded as 0xEF 0xBF 0xBD in UTF-8
    constexpr char cReplacementChar[] = "\xEF\xBF\xBD";

    SECTION("single invalid byte 0xFF") {
        auto input = "hello\xFF world"s;
        std::string expected = "hello" + std::string(cReplacementChar) + " world";
        REQUIRE(sanitize_utf8_string(input) == expected);
    }

    SECTION("single invalid byte 0xFE") {
        auto input = "test\xFE"s;
        std::string expected = "test" + std::string(cReplacementChar);
        REQUIRE(sanitize_utf8_string(input) == expected);
    }

    SECTION("multiple invalid bytes") {
        auto input = "\xFF\xFE\xFF"s;
        std::string expected = std::string(cReplacementChar) + cReplacementChar + cReplacementChar;
        REQUIRE(sanitize_utf8_string(input) == expected);
    }

    SECTION("continuation byte without leader (0x80-0xBF)") {
        auto input = "test\x80 value"s;
        std::string expected = "test" + std::string(cReplacementChar) + " value";
        REQUIRE(sanitize_utf8_string(input) == expected);
    }

    SECTION("truncated 2-byte sequence") {
        // 0xC3 expects one continuation byte, but we end the string
        auto input = "test\xC3"s;
        std::string expected = "test" + std::string(cReplacementChar);
        REQUIRE(sanitize_utf8_string(input) == expected);
    }

    SECTION("truncated 3-byte sequence") {
        // 0xE2 expects two continuation bytes
        auto input = "test\xE2\x82"s;
        std::string expected
                = "test" + std::string(cReplacementChar) + std::string(cReplacementChar);
        REQUIRE(sanitize_utf8_string(input) == expected);
    }

    SECTION("truncated 4-byte sequence") {
        // 0xF0 expects three continuation bytes
        auto input = "test\xF0\x9D\x84"s;
        std::string expected = "test" + std::string(cReplacementChar)
                               + std::string(cReplacementChar) + std::string(cReplacementChar);
        REQUIRE(sanitize_utf8_string(input) == expected);
    }

    SECTION("overlong 2-byte encoding") {
        // 0xC0 0x80 is overlong encoding of NUL (should be just 0x00)
        // 0xC0 and 0xC1 are never valid UTF-8 lead bytes
        auto input = "test\xC0\x80"s;
        // Both bytes are invalid
        std::string expected
                = "test" + std::string(cReplacementChar) + std::string(cReplacementChar);
        REQUIRE(sanitize_utf8_string(input) == expected);
    }

    SECTION("surrogate code points (U+D800-U+DFFF)") {
        // 0xED 0xA0 0x80 encodes U+D800 (invalid surrogate)
        auto input = "test\xED\xA0\x80 end"s;
        // The sequence is invalid, bytes replaced individually
        std::string expected = "test" + std::string(cReplacementChar)
                               + std::string(cReplacementChar) + std::string(cReplacementChar)
                               + " end";
        REQUIRE(sanitize_utf8_string(input) == expected);
    }

    SECTION("code point above U+10FFFF") {
        // 0xF4 0x90 0x80 0x80 would encode U+110000 (above max)
        auto input = "test\xF4\x90\x80\x80 end"s;
        std::string sanitized = sanitize_utf8_string(input);
        // Should contain replacement characters
        REQUIRE(count_replacement_chars(sanitized) >= 1);
    }
}

TEST_CASE("sanitize_utf8_buffer_mixed_valid_invalid", "[clp_s][StringUtils]") {
    constexpr char cReplacementChar[] = "\xEF\xBF\xBD";

    SECTION("invalid byte between valid UTF-8") {
        // café with invalid byte in middle
        auto input = "caf\xC3\xA9\xFF test"s;  // café + 0xFF + " test"
        std::string expected = "caf\xC3\xA9" + std::string(cReplacementChar) + " test";
        REQUIRE(sanitize_utf8_string(input) == expected);
    }

    SECTION("JSON with invalid UTF-8 in value") {
        auto input = "{\"msg\": \"test\xFF value\"}"s;
        std::string expected = "{\"msg\": \"test" + std::string(cReplacementChar) + " value\"}";
        REQUIRE(sanitize_utf8_string(input) == expected);
    }
}

TEST_CASE("sanitize_utf8_buffer_returns_correct_counts", "[clp_s][StringUtils]") {
    SECTION("counts invalid sequences") {
        auto input = "\xFF\xFE\xFF"s;  // 3 invalid bytes

        size_t buf_size = input.size();
        size_t buf_occupied = input.size();
        char* buf = new char[buf_size + simdjson::SIMDJSON_PADDING];
        std::memcpy(buf, input.data(), input.size());

        try {
            auto const result = StringUtils::sanitize_utf8_buffer(
                    buf,
                    buf_size,
                    buf_occupied,
                    simdjson::SIMDJSON_PADDING
            );

            // The count uses 0xFF as a special key for invalid UTF-8 sequences
            REQUIRE_FALSE(result.sanitized_char_counts.empty());
            size_t total = 0;
            for (auto const& [ch, count] : result.sanitized_char_counts) {
                total += count;
            }
            REQUIRE(total == 3);

            delete[] buf;
        } catch (...) {
            delete[] buf;
            throw;
        }
    }

    SECTION("returns empty counts when no sanitization needed") {
        std::string input = "valid UTF-8 string";

        size_t buf_size = input.size();
        size_t buf_occupied = input.size();
        char* buf = new char[buf_size + simdjson::SIMDJSON_PADDING];
        std::memcpy(buf, input.data(), input.size());

        try {
            auto const result = StringUtils::sanitize_utf8_buffer(
                    buf,
                    buf_size,
                    buf_occupied,
                    simdjson::SIMDJSON_PADDING
            );

            REQUIRE(result.sanitized_char_counts.empty());
            REQUIRE(result.new_buf_occupied == input.size());

            delete[] buf;
        } catch (...) {
            delete[] buf;
            throw;
        }
    }
}

TEST_CASE("sanitize_utf8_buffer_handles_buffer_growth", "[clp_s][StringUtils]") {
    // Each invalid byte (1 byte) becomes U+FFFD (3 bytes), so buffer may need to grow
    SECTION("many invalid bytes requiring expansion") {
        // 100 invalid bytes -> 300 bytes after replacement with U+FFFD
        std::string input(100, '\xFF');

        std::string sanitized = sanitize_utf8_string(input);

        // Verify all bytes were replaced
        REQUIRE(sanitized.find('\xFF') == std::string::npos);
        // Verify correct number of replacement characters
        REQUIRE(count_replacement_chars(sanitized) == 100);
        // Verify size: 100 * 3 = 300 bytes
        REQUIRE(sanitized.size() == 300);
    }
}

// =====================================================================================
// JSON Sanitization Character Count Tests
// =====================================================================================

TEST_CASE("sanitize_json_buffer_returns_correct_char_counts", "[clp_s][StringUtils]") {
    SECTION("counts multiple different control characters") {
        // Input with: 3x \x00, 2x \x01, 1x \x1f
        auto input = "{\"a\": \"\x00\x00\x01\x00\x01\x1f\"}"s;

        size_t buf_size = input.size();
        size_t buf_occupied = input.size();
        char* buf = new char[buf_size + simdjson::SIMDJSON_PADDING];
        std::memcpy(buf, input.data(), input.size());

        try {
            auto const result = StringUtils::sanitize_json_buffer(
                    buf,
                    buf_size,
                    buf_occupied,
                    simdjson::SIMDJSON_PADDING
            );

            REQUIRE(result.sanitized_char_counts.size() == 3);
            REQUIRE(result.sanitized_char_counts.at('\x00') == 3);
            REQUIRE(result.sanitized_char_counts.at('\x01') == 2);
            REQUIRE(result.sanitized_char_counts.at('\x1f') == 1);

            delete[] buf;
        } catch (...) {
            delete[] buf;
            throw;
        }
    }

    SECTION("returns empty counts when no sanitization needed") {
        std::string input = R"({"key": "valid value"})";

        size_t buf_size = input.size();
        size_t buf_occupied = input.size();
        char* buf = new char[buf_size + simdjson::SIMDJSON_PADDING];
        std::memcpy(buf, input.data(), input.size());

        try {
            auto const result = StringUtils::sanitize_json_buffer(
                    buf,
                    buf_size,
                    buf_occupied,
                    simdjson::SIMDJSON_PADDING
            );

            REQUIRE(result.sanitized_char_counts.empty());
            REQUIRE(result.new_buf_occupied == input.size());

            delete[] buf;
        } catch (...) {
            delete[] buf;
            throw;
        }
    }
}
