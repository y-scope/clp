#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>
#include <fmt/base.h>
#include <fmt/core.h>
#include <fmt/format.h>

#include "../../Defs.hpp"
#include "../TimestampParser.hpp"

namespace clp_s::timestamp_parser::test {
namespace {
struct ExpectedParsingResult {
    ExpectedParsingResult(
            std::string_view timestamp,
            std::string_view pattern,
            epochtime_t epoch_timestamp
    )
            : timestamp(timestamp),
              pattern(pattern),
              epoch_timestamp(epoch_timestamp) {}

    std::string timestamp;
    std::string pattern;
    epochtime_t epoch_timestamp;
};

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

/**
 * Generates triangles of numbers up to a maximum number of digits each composed of a single digit
 * 1-9.
 *
 * E.g., generate_number_triangles(3) ->
 * "1", "11", "111", ..., "9", "99", "999".
 *
 * @param max_num_digits
 * @return The elements of all of the triangles of numbers up to the maximum number of digits.
 */
auto generate_number_triangles(size_t max_num_digits) -> std::vector<std::string>;

/**
 * @param num_digits
 * @return All of the padded numbers with `num_digits` digits having a single unique digit.
 */
auto generate_padded_number_subset(size_t num_digits) -> std::vector<std::string>;

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
        REQUIRE(result.value().second == pattern);
    }
}

auto generate_padded_numbers_in_range(size_t begin, size_t end, size_t field_length, char padding)
        -> std::vector<std::string> {
    REQUIRE(begin <= end);
    std::vector<std::string> generated_numbers;
    auto const pattern{fmt::format("{{:{}>{}d}}", padding, field_length)};
    for (size_t i{begin}; i <= end; ++i) {
        generated_numbers.emplace_back(fmt::vformat(pattern, fmt::make_format_args(i)));
    }
    return generated_numbers;
}

auto generate_number_triangles(size_t max_num_digits) -> std::vector<std::string> {
    std::vector<std::string> generated_numbers;
    for (char digit{'1'}; digit <= '9'; ++digit) {
        for (size_t i{1ULL}; i <= max_num_digits; ++i) {
            generated_numbers.emplace_back(i, digit);
        }
    }
    return generated_numbers;
}

auto generate_padded_number_subset(size_t num_digits) -> std::vector<std::string> {
    std::vector<std::string> generated_numbers;
    for (char digit{'0'}; digit <= '9'; ++digit) {
        generated_numbers.emplace_back(num_digits, digit);
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

        auto const space_padded_days{generate_padded_numbers_in_range(1, 31, 2, ' ')};
        assert_specifier_accepts_valid_content('e', space_padded_days);

        // The parser asserts that the day of the week in the timestamp is actually correct, so we
        // need to include some extra date information to have days of the week line up.
        std::vector<std::string> const abbreviated_day_in_week_timestamps{
                "04 Sun",
                "05 Mon",
                "06 Tue",
                "07 Wed",
                "01 Thu",  // Thursday January 1st, 1970
                "02 Fri",
                "03 Sat"
        };
        for (auto const& day_in_week_timestamp : abbreviated_day_in_week_timestamps) {
            constexpr std::string_view pattern{"\\d \\aa"};
            std::string generated_pattern;
            auto const timestamp{fmt::format("{}a", day_in_week_timestamp)};
            auto const result{parse_timestamp(timestamp, pattern, generated_pattern)};
            REQUIRE(false == result.has_error());
            REQUIRE(result.value().second == pattern);
        }

        auto const two_digit_hours{generate_padded_numbers_in_range(0, 23, 2, '0')};
        assert_specifier_accepts_valid_content('H', two_digit_hours);

        auto const space_padded_hours{generate_padded_numbers_in_range(0, 23, 2, ' ')};
        assert_specifier_accepts_valid_content('k', space_padded_hours);

        constexpr std::array cPartsOfDay{"AM", "PM"};
        auto const twelve_hour_clock_two_digit_hours{
                generate_padded_numbers_in_range(1, 12, 2, '0')
        };
        auto const twelve_hour_clock_zero_padded_hours{
                generate_padded_numbers_in_range(1, 12, 2, ' ')
        };
        for (auto const& part_of_day : cPartsOfDay) {
            std::string generated_pattern;
            auto assert_twelve_hour_clock_accepts_valid_content
                    = [&](char hour_type, std::vector<std::string> const& hours) -> void {
                auto const pattern{fmt::format("\\{} \\pa", hour_type)};
                for (auto const& hour : hours) {
                    auto const timestamp{fmt::format("{} {}a", hour, part_of_day)};
                    auto const result{parse_timestamp(timestamp, pattern, generated_pattern)};
                    REQUIRE(false == result.has_error());
                    REQUIRE(result.value().second == pattern);
                }
            };
            assert_twelve_hour_clock_accepts_valid_content('I', twelve_hour_clock_two_digit_hours);
            assert_twelve_hour_clock_accepts_valid_content(
                    'l',
                    twelve_hour_clock_zero_padded_hours
            );
        }

        auto const two_digit_minutes{generate_padded_numbers_in_range(0, 59, 2, '0')};
        assert_specifier_accepts_valid_content('M', two_digit_minutes);

        auto const two_digit_seconds{generate_padded_numbers_in_range(0, 60, 2, '0')};
        assert_specifier_accepts_valid_content('S', two_digit_seconds);

        auto const milliseconds{generate_padded_number_subset(3)};
        assert_specifier_accepts_valid_content('3', milliseconds);

        auto const microseconds{generate_padded_number_subset(6)};
        assert_specifier_accepts_valid_content('6', microseconds);

        auto const nanoseconds{generate_padded_number_subset(9)};
        assert_specifier_accepts_valid_content('9', nanoseconds);

        auto const variable_length_nanoseconds(generate_number_triangles(9));
        assert_specifier_accepts_valid_content('T', variable_length_nanoseconds);

        auto const epoch_timestamps(generate_number_triangles(15));
        std::vector<std::string> negative_epoch_timestamps;
        for (auto const& timestamp : epoch_timestamps) {
            negative_epoch_timestamps.emplace_back(fmt::format("-{}", timestamp));
        }
        assert_specifier_accepts_valid_content('E', epoch_timestamps);
        assert_specifier_accepts_valid_content('E', negative_epoch_timestamps);
        assert_specifier_accepts_valid_content('L', epoch_timestamps);
        assert_specifier_accepts_valid_content('L', negative_epoch_timestamps);
        assert_specifier_accepts_valid_content('C', epoch_timestamps);
        assert_specifier_accepts_valid_content('C', negative_epoch_timestamps);
        assert_specifier_accepts_valid_content('N', epoch_timestamps);
        assert_specifier_accepts_valid_content('N', negative_epoch_timestamps);
    }

    std::vector<ExpectedParsingResult> const expected_parsing_results{
            {"2015-02-01T01:02:03.004", "\\Y-\\m-\\dT\\H:\\M:\\S.\\3", 1'422'752'523'004'000'000},
            {"2015-02-01T01:02:03.004005",
             "\\Y-\\m-\\dT\\H:\\M:\\S.\\6",
             1'422'752'523'004'005'000},
            {"2015-02-01T01:02:03.004005006",
             "\\Y-\\m-\\dT\\H:\\M:\\S.\\9",
             1'422'752'523'004'005'006}
    };
    SECTION("Timestamps are parsed accurately") {
        std::string generated_pattern;
        for (auto const& expected_result : expected_parsing_results) {
            auto const result{parse_timestamp(
                    expected_result.timestamp,
                    expected_result.pattern,
                    generated_pattern
            )};
            REQUIRE(false == result.has_error());
            REQUIRE(expected_result.epoch_timestamp == result.value().first);
            REQUIRE(expected_result.pattern == result.value().second);
        }
    }
}
}  // namespace clp_s::timestamp_parser::test
