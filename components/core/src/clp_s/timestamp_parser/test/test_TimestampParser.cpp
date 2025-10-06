#include <iostream>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <fmt/core.h>
#include <fmt/format.h>

#include "../TimestampParser.hpp"

namespace clp_s::timestamp_parser::test {
namespace {
/**
 * Asserts that a format specifier is able to parse a variety of valid content.
 * @param specifier The format specifier.
 * @param content A list of string payloads that are valid content for `specifier`.
 */
void
assert_specifier_accepts_valid_content(char specifier, std::vector<std::string> const& content);

/**
 * Generates all numbers in the range [`begin`, `end`], left-padded with `padding` up to
 * `field_length`.
 * @param begin
 * @param end
 * @param field_length
 * @param padding
 * @return A vector of all generated numbers.
 */
auto generate_padded_numbers_in_range(size_t begin, size_t end, size_t field_length, char padding)
        -> std::vector<std::string>;

void
assert_specifier_accepts_valid_content(char specifier, std::vector<std::string> const& content) {
    // We use a trailing literal to ensure that the specifier exactly consumes all of the content.
    auto const pattern{fmt::format("\\{}a", specifier)};
    std::string generated_pattern;
    CAPTURE(pattern);
    for (auto const& test_case : content) {
        auto const timestamp{fmt::format("{}a", test_case)};
        CAPTURE(timestamp);
        auto const result{parse_timestamp(timestamp, pattern, generated_pattern)};
        REQUIRE(false == result.has_error());
    }
}

auto generate_padded_numbers_in_range(size_t begin, size_t end, size_t field_length, char padding)
        -> std::vector<std::string> {
    REQUIRE(begin < end);
    std::vector<std::string> generated_numbers;
    auto const pattern{fmt::format("{{:{}>{}d}}", padding, field_length)};
    for (size_t i{begin}; i <= end; ++i) {
        generated_numbers.emplace_back(fmt::vformat(pattern, fmt::make_format_args(i)));
    }
    return generated_numbers;
}
}  // namespace

TEST_CASE("timestamp_parser_parse_timestamp", "[clp-s][timestamp-parser]") {
    SECTION("Format specifiers accept valid content.") {
        auto const two_digit_years{generate_padded_numbers_in_range(0, 99, 2, '0')};
        assert_specifier_accepts_valid_content('y', two_digit_years);

        auto const four_digit_years{generate_padded_numbers_in_range(0, 9999, 4, '0')};
        assert_specifier_accepts_valid_content('Y', four_digit_years);

        std::vector<std::string> const months{
                "January",
                "February",
                "March",
                "April",
                "May",
                "June",
                "July",
                "August",
                "September",
                "October",
                "November",
                "December"
        };
        assert_specifier_accepts_valid_content('B', months);

        std::vector<std::string> const abbreviated_months{
                "Jan",
                "Feb",
                "Mar",
                "Apr",
                "May",
                "Jun",
                "Jul",
                "Aug",
                "Sep",
                "Oct",
                "Nov",
                "Dec"
        };
        assert_specifier_accepts_valid_content('b', abbreviated_months);

        auto const two_digit_months{generate_padded_numbers_in_range(1, 12, 2, '0')};
        assert_specifier_accepts_valid_content('m', two_digit_months);

        auto const two_digit_days{generate_padded_numbers_in_range(1, 31, 2, '0')};
        assert_specifier_accepts_valid_content('d', two_digit_days);

        auto const space_padded_days(generate_padded_numbers_in_range(1, 31, 2, ' '));
        assert_specifier_accepts_valid_content('e', space_padded_days);

        // The parser asserts that the day of the week in the timestamp is actually correct, so we
        // need to include some extra date information to have days of the week line up.
        std::vector<std::string> abbreviated_day_in_week_timestamps{
                "04 Sun",
                "05 Mon",
                "06 Tue",
                "07 Wed",
                "01 Thu",  // Thursday January 1st, 1970
                "02 Fri",
                "03 Sat"
        };
        for (auto const& day_in_week_timestamp : abbreviated_day_in_week_timestamps) {
            std::string generated_pattern;
            auto const timestamp{fmt::format("{}a", day_in_week_timestamp)};
            auto const result{parse_timestamp(timestamp, "\\d \\aa", generated_pattern)};
            REQUIRE(false == result.has_error());
        }
    }
}
}  // namespace clp_s::timestamp_parser::test
