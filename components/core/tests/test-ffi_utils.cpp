#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <random>
#include <string>
#include <string_view>
#include <vector>

#include <Catch2/single_include/catch2/catch.hpp>
#include <json/single_include/nlohmann/json.hpp>

#include "../src/clp/ffi/utils.hpp"

using clp::ffi::validate_and_escape_utf8_string;

namespace {
/**
 * Gets an expected escaped string by first convert the raw string into a json string and then dumps
 * the a printable string using nlohmann::json.
 * @param raw
 * @return Escaped string dumped by nlohmann::json, with surrounding '"' dropped.
 */
[[nodiscard]] auto get_expected_escaped_string(std::string_view raw) -> std::string;

auto get_expected_escaped_string(std::string_view raw) -> std::string {
    nlohmann::json const json_str = raw;  // Don't use '{}' initializer
    auto const dumped_str{json_str.dump()};
    return {dumped_str.begin() + 1, dumped_str.end() - 1};
}
}  // namespace

TEST_CASE("escape_utf8_string_basic", "[ffi][utils]") {
    std::string test_str;
    std::optional<std::string> actual;

    // Test empty string
    actual = validate_and_escape_utf8_string(test_str);
    REQUIRE((actual.has_value() && actual.value() == get_expected_escaped_string(test_str)));

    // Test string that has nothing to escape
    test_str = "This string has nothing to escape :)";
    actual = validate_and_escape_utf8_string(test_str);
    REQUIRE((actual.has_value() && actual.value() == get_expected_escaped_string(test_str)));

    // Test string with all single byte UTF8 characters, which include all characters we escape
    test_str.clear();
    for (uint8_t i{0}; i <= static_cast<uint8_t>(INT8_MAX); ++i) {
        test_str.push_back(static_cast<char>(i));
    }
    // Shuffle characters randomly, ensure control characters are not grouped together.
    // NOLINTNEXTLINE(cert-msc32-c, cert-msc51-cpp)
    std::shuffle(test_str.begin(), test_str.end(), std::default_random_engine{});
    actual = validate_and_escape_utf8_string(test_str);
    REQUIRE((actual.has_value() && actual.value() == get_expected_escaped_string(test_str)));

    // Test valid UTF8 chars with continuation bytes
    std::vector<std::string> const valid_utf8{
            "\n",
            "\xF0\xA0\x80\x8F",  // https://en.wiktionary.org/wiki/%F0%A0%80%8F
            "a",
            "\xE4\xB8\xAD",  // https://en.wiktionary.org/wiki/%E4%B8%AD
            "\x1F",
            "\xC2\xA2",  // ¢
            "\\"
    };
    test_str.clear();
    for (auto const& str : valid_utf8) {
        test_str.append(str);
    }
    actual = validate_and_escape_utf8_string(test_str);
    REQUIRE((actual.has_value() && actual.value() == get_expected_escaped_string(test_str)));
}

TEST_CASE("escape_utf8_string_with_continuation", "[ffi][utils]") {
    std::string test_str;
    std::optional<std::string> actual;

    // Test UTF8 code point range validation
    auto const valid_code_point_lower_bound = GENERATE(
            std::string_view{"\xC2\x80"},
            std::string_view{"\xE0\xA0\x80"},
            std::string_view{"\xF0\x90\x80\x80"}
    );

    auto const valid_code_point_upper_bound = GENERATE(
            std::string_view{"\xDF\xBF"},
            std::string_view{"\xEF\xBF\xBF"},
            std::string_view{"\xF4\x8F\xBF\xBF"}
    );

    test_str = valid_code_point_lower_bound;
    actual = validate_and_escape_utf8_string(test_str);
    REQUIRE((actual.has_value() && actual.value() == get_expected_escaped_string(test_str)));

    test_str = valid_code_point_upper_bound;
    actual = validate_and_escape_utf8_string(test_str);
    REQUIRE((actual.has_value() && actual.value() == get_expected_escaped_string(test_str)));

    // Test invalid code point: 0x7F (only need one byte)
    test_str = "\xC1\xBF";
    REQUIRE((false == validate_and_escape_utf8_string(test_str).has_value()));

    test_str = "\xE0\x81\xBF";
    REQUIRE((false == validate_and_escape_utf8_string(test_str).has_value()));

    test_str = "\xF0\x81\x81\xBF";
    REQUIRE((false == validate_and_escape_utf8_string(test_str).has_value()));

    // Test invalid code point: 0x73 (only need one byte)
    test_str = "\xC1\xB3";
    REQUIRE((false == validate_and_escape_utf8_string(test_str).has_value()));

    test_str = "\xE0\x81\xB3";
    REQUIRE((false == validate_and_escape_utf8_string(test_str).has_value()));

    test_str = "\xF0\x81\x81\xB3";
    REQUIRE((false == validate_and_escape_utf8_string(test_str).has_value()));

    // Test invalid code point: 0x7FF (only need 2 bytes)
    test_str = "\xE0\x9F\xBF";
    REQUIRE((false == validate_and_escape_utf8_string(test_str).has_value()));

    test_str = "\xF0\x80\x9F\xBF";
    REQUIRE((false == validate_and_escape_utf8_string(test_str).has_value()));

    // Test invalid code point: 0x7F3 (only need 2 bytes)
    test_str = "\xE0\x9F\xB3";
    REQUIRE((false == validate_and_escape_utf8_string(test_str).has_value()));

    test_str = "\xF0\x80\x9F\xB3";
    REQUIRE((false == validate_and_escape_utf8_string(test_str).has_value()));

    // Test invalid code point: 0xFFFF (only need 3 bytes)
    test_str = "\xF0\x8F\xBF\xBF";
    REQUIRE((false == validate_and_escape_utf8_string(test_str).has_value()));

    // Test invalid code point: 0xFFF3 (only need 3 bytes)
    test_str = "\xF0\x8F\xBF\xB3";
    REQUIRE((false == validate_and_escape_utf8_string(test_str).has_value()));

    // Test incomplete continuation bytes
    std::string_view::const_iterator const it_begin{valid_code_point_lower_bound.cbegin()};
    std::string const valid{"Valid"};
    for (std::string_view::const_iterator it_end{valid_code_point_lower_bound.cend() - 1};
         valid_code_point_lower_bound.cbegin() != it_end;
         --it_end)
    {
        std::string const incomplete_byte_sequence{it_begin, it_end};
        REQUIRE(
                (false
                 == validate_and_escape_utf8_string(valid + incomplete_byte_sequence).has_value())
        );
        REQUIRE(
                (false
                 == validate_and_escape_utf8_string(incomplete_byte_sequence + valid).has_value())
        );
    }

    // Test invalid header byte
    test_str = valid_code_point_lower_bound;
    constexpr char cInvalidHeaderByte{'\xFF'};
    test_str.front() = cInvalidHeaderByte;
    REQUIRE((false == validate_and_escape_utf8_string(test_str).has_value()));

    // Test invalid continuation bytes
    for (size_t idx{1}; idx < valid_code_point_lower_bound.size(); ++idx) {
        test_str = valid_code_point_lower_bound;
        constexpr uint8_t cInvalidateMask{0x40};
        test_str.at(idx) |= cInvalidateMask;
        REQUIRE((false == validate_and_escape_utf8_string(test_str).has_value()));
    }
}
