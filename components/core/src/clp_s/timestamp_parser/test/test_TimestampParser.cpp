#include <array>
#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <fmt/base.h>
#include <fmt/core.h>
#include <fmt/format.h>

#include "../../Defs.hpp"
#include "../ErrorCode.hpp"
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

struct ExpectedCatSequenceTransformation {
    ExpectedCatSequenceTransformation(
            std::string_view timestamp,
            std::string_view cat_sequence,
            std::string_view transformed_pattern
    )
            : timestamp{timestamp},
              cat_sequence{cat_sequence},
              transformed_pattern{transformed_pattern} {}

    std::string timestamp;
    std::string cat_sequence;
    std::string transformed_pattern;
};

/**
 * Asserts that a format specifier is able to parse a variety of valid content.
 * @param specifier The format specifier.
 * @param content A list of string payloads that are valid content for `specifier`.
 */
void assert_specifier_accepts_valid_content(
        std::string specifier,
        std::vector<std::string> const& content
);

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
[[nodiscard]] auto generate_number_triangles(size_t max_num_digits) -> std::vector<std::string>;

/**
 * @param num_digits
 * @return A vector containing all single-unique-digit (0-9) padded numbers with the unique digit
 * repeated to a length of `num_digits`.
 */
[[nodiscard]] auto generate_padded_number_subset(size_t num_digits) -> std::vector<std::string>;

void assert_specifier_accepts_valid_content(
        std::string specifier,
        std::vector<std::string> const& content
) {
    // We use a trailing literal to ensure that the specifier exactly consumes all of the content.
    auto const pattern{fmt::format(R"(\{}a)", specifier)};
    std::string generated_pattern;
    CAPTURE(pattern);
    auto const timestamp_pattern_result{TimestampPattern::create(pattern)};
    REQUIRE_FALSE(timestamp_pattern_result.has_error());
    for (auto const& test_case : content) {
        auto const timestamp{fmt::format("{}a", test_case)};
        CAPTURE(timestamp);
        auto const result{parse_timestamp(
                timestamp,
                timestamp_pattern_result.value(),
                false,
                generated_pattern
        )};
        REQUIRE_FALSE(result.has_error());
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
    SECTION("Timestamp pattern templates reject illegal sequences.") {
        std::vector<std::string> const illegal_character_timestamp_pattern_templates{
                R"(")",
                R"(abc")",
                R"("abc)",
                std::string{"\x00", 1ULL},
                "\x01",
                "\x1f",
        };
        for (auto const& illegal_timestamp_pattern_template :
             illegal_character_timestamp_pattern_templates)
        {
            auto const result{TimestampPattern::create(illegal_timestamp_pattern_template)};
            REQUIRE(result.has_error());
            REQUIRE(ErrorCode{ErrorCodeEnum::InvalidCharacter} == result.error());
        }

        std::vector<std::string> const illegal_escape_timestamp_pattern_templates{
                R"(\u0000)",
                R"(\b)",
                R"(\f)",
                R"(\n)",
                R"(\r)",
                R"(\t)",
                R"(\u)"
        };
        for (auto const& illegal_timestamp_pattern_template :
             illegal_escape_timestamp_pattern_templates)
        {
            auto const result{TimestampPattern::create(illegal_timestamp_pattern_template)};
            REQUIRE(result.has_error());
            REQUIRE(ErrorCode{ErrorCodeEnum::InvalidEscapeSequence} == result.error());
        }
    }

    SECTION("Escape sequence accepts valid content.") {
        auto const backlash_pattern_result{TimestampPattern::create(R"(\\)")};
        REQUIRE_FALSE(backlash_pattern_result.has_error());

        std::string generated_pattern;
        REQUIRE_FALSE(
                parse_timestamp(R"(\)", backlash_pattern_result.value(), false, generated_pattern)
                        .has_error()
        );
        REQUIRE(parse_timestamp(R"(\\)", backlash_pattern_result.value(), false, generated_pattern)
                        .has_error());
        REQUIRE_FALSE(
                parse_timestamp(R"(\\)", backlash_pattern_result.value(), true, generated_pattern)
                        .has_error()
        );
        REQUIRE(parse_timestamp(R"(\)", backlash_pattern_result.value(), true, generated_pattern)
                        .has_error());
    }

    SECTION("Format specifiers accept valid content.") {
        auto const two_digit_years{generate_padded_numbers_in_range(0, 99, 2, '0')};
        assert_specifier_accepts_valid_content("y", two_digit_years);

        auto const four_digit_years{generate_padded_numbers_in_range(0, 9999, 4, '0')};
        assert_specifier_accepts_valid_content("Y", four_digit_years);

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
        assert_specifier_accepts_valid_content(
                "B{January,February,March,April,May,June,July,August,September,October,November,"
                "December}",
                months
        );

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
        assert_specifier_accepts_valid_content(
                "B{Jan,Feb,Mar,Apr,May,Jun,Jul,Aug,Sep,Oct,Nov,Dec}",
                abbreviated_months
        );

        auto const two_digit_months{generate_padded_numbers_in_range(1, 12, 2, '0')};
        assert_specifier_accepts_valid_content("m", two_digit_months);

        auto const two_digit_days{generate_padded_numbers_in_range(1, 31, 2, '0')};
        assert_specifier_accepts_valid_content("d", two_digit_days);

        auto const space_padded_days{generate_padded_numbers_in_range(1, 31, 2, ' ')};
        assert_specifier_accepts_valid_content("e", space_padded_days);

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
        constexpr std::string_view cDayInWeekPattern{R"(\d \A{Sun,Mon,Tue,Wed,Thu,Fri,Sat}a)"};
        auto const day_in_week_pattern_result{
                TimestampPattern::create(std::string{cDayInWeekPattern})
        };
        REQUIRE_FALSE(day_in_week_pattern_result.has_error());
        for (auto const& day_in_week_timestamp : abbreviated_day_in_week_timestamps) {
            std::string generated_pattern;
            auto const timestamp{fmt::format("{}a", day_in_week_timestamp)};
            auto const result{parse_timestamp(
                    timestamp,
                    day_in_week_pattern_result.value(),
                    false,
                    generated_pattern
            )};
            REQUIRE_FALSE(result.has_error());
            REQUIRE(result.value().second == cDayInWeekPattern);
        }

        auto const two_digit_hours{generate_padded_numbers_in_range(0, 23, 2, '0')};
        assert_specifier_accepts_valid_content("H", two_digit_hours);

        auto const space_padded_hours{generate_padded_numbers_in_range(0, 23, 2, ' ')};
        assert_specifier_accepts_valid_content("k", space_padded_hours);

        auto const twelve_hour_clock_two_digit_hours{
                generate_padded_numbers_in_range(1, 12, 2, '0')
        };
        auto const twelve_hour_clock_zero_padded_hours{
                generate_padded_numbers_in_range(1, 12, 2, ' ')
        };
        constexpr std::array cPartsOfDay{std::string_view{"AM"}, std::string_view{"PM"}};
        for (auto const part_of_day : cPartsOfDay) {
            std::string generated_pattern;
            auto assert_twelve_hour_clock_accepts_valid_content
                    = [&](char hour_type, std::vector<std::string> const& hours) -> void {
                auto const pattern{fmt::format(R"(\{} \pa)", hour_type)};
                auto const timestamp_pattern_result{TimestampPattern::create(pattern)};
                REQUIRE_FALSE(timestamp_pattern_result.has_error());
                for (auto const& hour : hours) {
                    auto const timestamp{fmt::format("{} {}a", hour, part_of_day)};
                    auto const result{parse_timestamp(
                            timestamp,
                            timestamp_pattern_result.value(),
                            false,
                            generated_pattern
                    )};
                    REQUIRE_FALSE(result.has_error());
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
        assert_specifier_accepts_valid_content("M", two_digit_minutes);

        auto const two_digit_seconds{generate_padded_numbers_in_range(0, 59, 2, '0')};
        assert_specifier_accepts_valid_content("S", two_digit_seconds);

        auto const two_digit_leap_seconds{generate_padded_numbers_in_range(60, 60, 2, '0')};
        assert_specifier_accepts_valid_content("J", two_digit_leap_seconds);

        auto const milliseconds{generate_padded_number_subset(3)};
        assert_specifier_accepts_valid_content("3", milliseconds);

        auto const microseconds{generate_padded_number_subset(6)};
        assert_specifier_accepts_valid_content("6", microseconds);

        auto const nanoseconds{generate_padded_number_subset(9)};
        assert_specifier_accepts_valid_content("9", nanoseconds);

        auto const variable_length_nanoseconds{generate_number_triangles(9)};
        assert_specifier_accepts_valid_content("T", variable_length_nanoseconds);

        auto const epoch_timestamps{generate_number_triangles(15)};
        std::vector<std::string> negative_epoch_timestamps;
        negative_epoch_timestamps.reserve(epoch_timestamps.size());
        for (auto const& timestamp : epoch_timestamps) {
            negative_epoch_timestamps.emplace_back(fmt::format("-{}", timestamp));
        }
        assert_specifier_accepts_valid_content("E", epoch_timestamps);
        assert_specifier_accepts_valid_content("E", negative_epoch_timestamps);
        assert_specifier_accepts_valid_content("L", epoch_timestamps);
        assert_specifier_accepts_valid_content("L", negative_epoch_timestamps);
        assert_specifier_accepts_valid_content("C", epoch_timestamps);
        assert_specifier_accepts_valid_content("C", negative_epoch_timestamps);
        assert_specifier_accepts_valid_content("N", epoch_timestamps);
        assert_specifier_accepts_valid_content("N", negative_epoch_timestamps);

        constexpr std::array cHoursOffsetPattenrs
                = {std::string_view{"+{}"}, std::string_view{"-{}"}, std::string_view{"\u2212{}"}};
        constexpr std::array cHoursMinutesOffsetPatterns
                = {std::string_view{"+{}{}"},
                   std::string_view{"-{}{}"},
                   std::string_view{"\u2212{}{}"},
                   std::string_view{"+{}:{}"},
                   std::string_view{"-{}:{}"},
                   std::string_view{"\u2212{}:{}"}};
        for (auto const& hour : two_digit_hours) {
            std::string generated_pattern;
            for (auto const& hour_offset_pattern : cHoursOffsetPattenrs) {
                auto const hour_offset{
                        fmt::vformat(hour_offset_pattern, fmt::make_format_args(hour))
                };
                auto const hour_offset_specifier{fmt::format("\\z{{{}}}", hour_offset)};
                auto const timestamp_pattern_result(
                        TimestampPattern::create(hour_offset_specifier)
                );
                REQUIRE_FALSE(timestamp_pattern_result.has_error());
                auto const result{parse_timestamp(
                        hour_offset,
                        timestamp_pattern_result.value(),
                        false,
                        generated_pattern
                )};
                REQUIRE_FALSE(result.has_error());
                REQUIRE(result.value().second == hour_offset_specifier);
            }
            for (auto const& minute : two_digit_minutes) {
                for (auto const& hour_minute_offset_pattern : cHoursMinutesOffsetPatterns) {
                    auto const hour_minute_offset{fmt::vformat(
                            hour_minute_offset_pattern,
                            fmt::make_format_args(hour, minute)
                    )};
                    auto const hour_minute_specifier{fmt::format("\\z{{{}}}", hour_minute_offset)};
                    auto const timestamp_pattern_result(
                            TimestampPattern::create(hour_minute_specifier)
                    );
                    REQUIRE_FALSE(timestamp_pattern_result.has_error());

                    auto const result{parse_timestamp(
                            hour_minute_offset,
                            timestamp_pattern_result.value(),
                            false,
                            generated_pattern
                    )};
                    REQUIRE_FALSE(result.has_error());
                    REQUIRE(result.value().second == hour_minute_specifier);
                }
            }
        }
    }

    SECTION("CAT sequences are transformed correctly.") {
        std::string generated_pattern;
        auto assert_transformations_are_expected
                = [&generated_pattern](
                          std::vector<ExpectedCatSequenceTransformation> const& transformations
                  ) -> void {
            for (auto const& transformation : transformations) {
                auto const timestamp_pattern_result{
                        TimestampPattern::create(transformation.cat_sequence)
                };
                REQUIRE_FALSE(timestamp_pattern_result.has_error());
                auto const result{parse_timestamp(
                        transformation.timestamp,
                        timestamp_pattern_result.value(),
                        false,
                        generated_pattern
                )};
                REQUIRE_FALSE(result.has_error());
                REQUIRE(transformation.transformed_pattern == result.value().second);
            }
        };

        std::vector<ExpectedCatSequenceTransformation> const timezone_transformations{
                {"Z", R"(\Z)", R"(Z)"},
                {"-04", R"(\Z)", R"(\z{-04})"},
                {"-04:30", R"(\Z)", R"(\z{-04:30})"},
                {"-0430", R"(\Z)", R"(\z{-0430})"},
                {"−04", R"(\Z)", R"(\z{−04})"},
                {"−04:30", R"(\Z)", R"(\z{−04:30})"},
                {"−0430", R"(\Z)", R"(\z{−0430})"},
                {"+04", R"(\Z)", R"(\z{+04})"},
                {"+04:30", R"(\Z)", R"(\z{+04:30})"},
                {"+0430", R"(\Z)", R"(\z{+0430})"},
                {"UTC+04", R"(\Z)", R"(UTC\z{+04})"},
                {" UTC+04", R"(\Z)", R"( UTC\z{+04})"},
                {"UTC+04Z", R"(\Z)", R"(UTC\z{+04}Z)"},
                {" UTC+04Z", R"(\Z)", R"( UTC\z{+04}Z)"},
                {"+04Z", R"(\Z)", R"(\z{+04}Z)"},
                {" +04Z", R"(\Z)", R"( \z{+04}Z)"},
                {" Z", R"(\Z)", R"( Z)"}
        };
        assert_transformations_are_expected(timezone_transformations);

        std::vector<ExpectedCatSequenceTransformation> const fractional_second_transformations{
                {"123", R"(\?)", R"(\3)"},
                {"123456", R"(\?)", R"(\6)"},
                {"123456789", R"(\?)", R"(\9)"},
                {"12", R"(\?)", R"(\T)"},
                {"1234", R"(\?)", R"(\T)"},
                {"1234567", R"(\?)", R"(\T)"},
                {"12345678", R"(\?)", R"(\T)"}
        };
        assert_transformations_are_expected(fractional_second_transformations);

        std::vector<ExpectedCatSequenceTransformation> const
                unknown_epoch_precision_transformations{
                        {"1763651316", R"(\P)", R"(\E)"},
                        {"1763651316642", R"(\P)", R"(\L)"},
                        {"1763651316642111", R"(\P)", R"(\C)"},
                        {"1763651316642111123", R"(\P)", R"(\N)"},
                        {"-1763651316", R"(\P)", R"(\E)"},
                        {"-1763651316642", R"(\P)", R"(\L)"},
                        {"-1763651316642111", R"(\P)", R"(\C)"},
                        {"-1763651316642111123", R"(\P)", R"(\N)"}
                };
        assert_transformations_are_expected(unknown_epoch_precision_transformations);

        std::vector<ExpectedCatSequenceTransformation> const one_of_literal_transformations{
                {"A", R"(\O{A})", "A"},
                {"AB", R"(\O{BA}\O{AB})", "AB"},
                {"F", R"(\O{ABCDEFGHIJKLMNOP})", "F"}
        };
        assert_transformations_are_expected(one_of_literal_transformations);

        std::vector<ExpectedCatSequenceTransformation> const generic_second_transformations{
                {"00", R"(\s)", R"(\S)"},
                {"01", R"(\s)", R"(\S)"},
                {"58", R"(\s)", R"(\S)"},
                {"59", R"(\s)", R"(\S)"},
                {"60", R"(\s)", R"(\J)"}
        };
        assert_transformations_are_expected(generic_second_transformations);
    }

    SECTION("Default timestamp patterns are valid.") {
        auto const default_date_time_patterns_result{get_default_date_time_timestamp_patterns()};
        REQUIRE_FALSE(default_date_time_patterns_result.has_error());
        auto const default_numeric_timestamp_patterns_result{
                get_default_numeric_timestamp_patterns()
        };
        REQUIRE_FALSE(default_numeric_timestamp_patterns_result.has_error());
        auto const all_default_timestamp_patterns_result{get_all_default_timestamp_patterns()};
        REQUIRE_FALSE(all_default_timestamp_patterns_result.has_error());
        auto const all_default_quoted_timestamp_patterns_result{
                get_all_default_quoted_timestamp_patterns()
        };
        REQUIRE_FALSE(all_default_quoted_timestamp_patterns_result.has_error());
    }

    SECTION("Timestamps are parsed accurately.") {
        std::vector<ExpectedParsingResult> const expected_parsing_results{
                {"2015-02-01T01:02:03.004", R"(\Y-\m-\dT\H:\M:\S.\3)", 1'422'752'523'004'000'000},
                {"2015-02-01T01:02:03.004005",
                 R"(\Y-\m-\dT\H:\M:\S.\6)",
                 1'422'752'523'004'005'000},
                {"2015-02-01T01:02:03.004005006",
                 R"(\Y-\m-\dT\H:\M:\S.\9)",
                 1'422'752'523'004'005'006},
                {"2015-02-01T01:02:03,004", R"(\Y-\m-\dT\H:\M:\S,\3)", 1'422'752'523'004'000'000},
                {"[2015-02-01T01:02:03", R"([\Y-\m-\dT\H:\M:\S)", 1'422'752'523'000'000'000},
                {"[20150201-01:02:03]", R"([\Y\m\d-\H:\M:\S])", 1'422'752'523'000'000'000},
                {"2015-02-01 01:02:03,004", R"(\Y-\m-\d \H:\M:\S,\3)", 1'422'752'523'004'000'000},
                {"2015-02-01 01:02:03.004", R"(\Y-\m-\d \H:\M:\S.\3)", 1'422'752'523'004'000'000},
                {"[2015-02-01 01:02:03,004]",
                 R"([\Y-\m-\d \H:\M:\S,\3])",
                 1'422'752'523'004'000'000},
                {"2015-02-01 01:02:03", R"(\Y-\m-\d \H:\M:\S)", 1'422'752'523'000'000'000},
                {"2015/02/01 01:02:03", R"(\Y/\m/\d \H:\M:\S)", 1'422'752'523'000'000'000},
                {"15/02/01 01:02:03", R"(\y/\m/\d \H:\M:\S)", 1'422'752'523'000'000'000},
                {"150201  1:02:03", R"(\y\m\d \k:\M:\S)", 1'422'752'523'000'000'000},
                {"01 Feb 2015 01:02:03,004",
                 R"(\d \B{Jan,Feb,Mar,Apr,May,Jun,Jul,Aug,Sep,Oct,Nov,Dec} \Y \H:\M:\S,\3)",
                 1'422'752'523'004'000'000},
                {"Feb 01, 2015  1:02:03 AM",
                 R"(\B{Jan,Feb,Mar,Apr,May,Jun,Jul,Aug,Sep,Oct,Nov,Dec} \d, \Y \l:\M:\S \p)",
                 1'422'752'523'000'000'000},
                {"Feb 01, 2015 01:02:03 AM",
                 R"(\B{Jan,Feb,Mar,Apr,May,Jun,Jul,Aug,Sep,Oct,Nov,Dec} \d, \Y \I:\M:\S \p)",
                 1'422'752'523'000'000'000},
                {"Feb 01, 2015 12:02:03 AM",
                 R"(\B{Jan,Feb,Mar,Apr,May,Jun,Jul,Aug,Sep,Oct,Nov,Dec} \d, \Y \l:\M:\S \p)",
                 1'422'748'923'000'000'000},
                {"Feb 01, 2015 12:02:03 PM",
                 R"(\B{Jan,Feb,Mar,Apr,May,Jun,Jul,Aug,Sep,Oct,Nov,Dec} \d, \Y \l:\M:\S \p)",
                 1'422'792'123'000'000'000},
                {"February 01, 2015 01:02",
                 R"(\B{January,February,March,April,May,June,July,August,September,October,November,December} \d, \Y \H:\M)",
                 1'422'752'520'000'000'000},
                {"[01/Feb/2015:01:02:03",
                 R"([\d/\B{Jan,Feb,Mar,Apr,May,Jun,Jul,Aug,Sep,Oct,Nov,Dec}/\Y:\H:\M:\S)",
                 1'422'752'523'000'000'000},
                {"Sun Feb  1 01:02:03 2015",
                 R"(\A{Sun,Mon,Tue,Wed,Thu,Fri,Sat} \B{Jan,Feb,Mar,Apr,May,Jun,Jul,Aug,Sep,Oct,Nov,Dec} \e \H:\M:\S \Y)",
                 1'422'752'523'000'000'000},
                {"<<<2015-02-01 01:02:03:004",
                 R"(<<<\Y-\m-\d \H:\M:\S:\3)",
                 1'422'752'523'004'000'000},
                {"Jan 21 11:56:42",
                 R"(\B{Jan,Feb,Mar,Apr,May,Jun,Jul,Aug,Sep,Oct,Nov,Dec} \d \H:\M:\S)",
                 1'771'002'000'000'000},
                {"01-21 11:56:42.392", R"(\m-\d \H:\M:\S.\3)", 1'771'002'392'000'000},
                {"2015/01/31 15:50:45.123", R"(\Y/\m/\d \H:\M:\S.\3)", 1'422'719'445'123'000'000},
                {"2015/01/31 15:50:45,123", R"(\Y/\m/\d \H:\M:\S,\3)", 1'422'719'445'123'000'000},
                {"2015/01/31T15:50:45", R"(\Y/\m/\dT\H:\M:\S)", 1'422'719'445'000'000'000},
                {"2015/01/31T15:50:45.123", R"(\Y/\m/\dT\H:\M:\S.\3)", 1'422'719'445'123'000'000},
                {"2015/01/31T15:50:45,123", R"(\Y/\m/\dT\H:\M:\S,\3)", 1'422'719'445'123'000'000},
                {"2015-01-31T15:50:45", R"(\Y-\m-\dT\H:\M:\S)", 1'422'719'445'000'000'000},
                {"1762445893", R"(\E)", 1'762'445'893'000'000'000},
                {"1762445893001", R"(\L)", 1'762'445'893'001'000'000},
                {"1762445893001002", R"(\C)", 1'762'445'893'001'002'000},
                {"1762445893001002003", R"(\N)", 1'762'445'893'001'002'003},
                {"1762445893.001", R"(\E.\3)", 1'762'445'893'001'000'000},
                {"1762445893.001002", R"(\E.\6)", 1'762'445'893'001'002'000},
                {"1762445893.001002003", R"(\E.\9)", 1'762'445'893'001'002'003},
                {"1762445893.001002000", R"(\E.\9)", 1'762'445'893'001'002'000},
                {"1762445893.00100201", R"(\E.\T)", 1'762'445'893'001'002'010},
                {"1762445893.1", R"(\E.\T)", 1'762'445'893'100'000'000},
                {"-1762445893", R"(\E)", -1'762'445'893'000'000'000},
                {"-1762445893001", R"(\L)", -1'762'445'893'001'000'000},
                {"-1762445893001002", R"(\C)", -1'762'445'893'001'002'000},
                {"-1762445893001002003", R"(\N)", -1'762'445'893'001'002'003},
                {"-1762445893.001", R"(\E.\3)", -1'762'445'893'001'000'000},
                {"-1762445893.001002", R"(\E.\6)", -1'762'445'893'001'002'000},
                {"-1762445893.001002003", R"(\E.\9)", -1'762'445'893'001'002'003},
                {"-1762445893.001002000", R"(\E.\9)", -1'762'445'893'001'002'000},
                {"-1762445893.00100201", R"(\E.\T)", -1'762'445'893'001'002'010},
                {"-1762445893.1", R"(\E.\T)", -1'762'445'893'100'000'000},
                {"Jan 21 11:56:42Z",
                 R"(\B{Jan,Feb,Mar,Apr,May,Jun,Jul,Aug,Sep,Oct,Nov,Dec} \d \H:\M:\SZ)",
                 1'771'002'000'000'000},
                {"Jan 21 11:56:42 UTC-01",
                 R"(\B{Jan,Feb,Mar,Apr,May,Jun,Jul,Aug,Sep,Oct,Nov,Dec} \d \H:\M:\S UTC\z{-01})",
                 1'774'602'000'000'000},
                {"Jan 21 11:56:42 UTC-01:30",
                 R"(\B{Jan,Feb,Mar,Apr,May,Jun,Jul,Aug,Sep,Oct,Nov,Dec} \d \H:\M:\S UTC\z{-01:30})",
                 1'776'402'000'000'000},
                {"Jan 21 11:56:42 UTC-0130",
                 R"(\B{Jan,Feb,Mar,Apr,May,Jun,Jul,Aug,Sep,Oct,Nov,Dec} \d \H:\M:\S UTC\z{-0130})",
                 1'776'402'000'000'000},
                {"Jan 21 11:56:42 UTC+01",
                 R"(\B{Jan,Feb,Mar,Apr,May,Jun,Jul,Aug,Sep,Oct,Nov,Dec} \d \H:\M:\S UTC\z{+01})",
                 1'767'402'000'000'000},
                {"Jan 21 11:56:42 UTC+01:30",
                 R"(\B{Jan,Feb,Mar,Apr,May,Jun,Jul,Aug,Sep,Oct,Nov,Dec} \d \H:\M:\S UTC\z{+01:30})",
                 1'765'602'000'000'000},
                {"Jan 21 11:56:42 UTC+0130",
                 R"(\B{Jan,Feb,Mar,Apr,May,Jun,Jul,Aug,Sep,Oct,Nov,Dec} \d \H:\M:\S UTC\z{+0130})",
                 1'765'602'000'000'000},
                {"1895-11-20T21:55:46,010", R"(\Y-\m-\dT\H:\M:\S,\3)", -2'338'769'053'990'000'000},
                {"2016-12-31T23:59:59,999Z", R"(\Y-\m-\dT\H:\M:\S,\3Z)", 1'483'228'799'999'000'000},
                {"2016-12-31T23:59:60,999Z", R"(\Y-\m-\dT\H:\M:\J,\3Z)", 1'483'228'799'999'000'000},
                {"2017-01-01T00:00:00,999Z", R"(\Y-\m-\dT\H:\M:\S,\3Z)", 1'483'228'800'999'000'000}
        };

        auto default_patterns_result{get_all_default_timestamp_patterns()};
        REQUIRE_FALSE(default_patterns_result.has_error());
        auto default_quoted_patterns_result{get_all_default_quoted_timestamp_patterns()};
        REQUIRE_FALSE(default_quoted_patterns_result.has_error());
        auto const default_patterns{std::move(default_patterns_result.value())};
        auto const default_quoted_patterns{std::move(default_quoted_patterns_result.value())};
        std::string generated_pattern;
        for (auto const& expected_result : expected_parsing_results) {
            auto const timestamp_pattern_result{TimestampPattern::create(expected_result.pattern)};
            REQUIRE_FALSE(timestamp_pattern_result.has_error());
            auto const expected_quoted_pattern{fmt::format(R"("{}")", expected_result.pattern)};
            auto const quoted_timestamp_pattern_result{
                    TimestampPattern::create(expected_quoted_pattern)
            };
            REQUIRE_FALSE(quoted_timestamp_pattern_result.has_error());
            auto const quoted_timestamp{fmt::format(R"("{}")", expected_result.timestamp)};

            auto const [quoted_content, quoted_pattern] = GENERATE(
                    std::make_pair(false, false),
                    std::make_pair(false, true),
                    std::make_pair(true, true)
            );
            auto const& timestamp{quoted_content ? quoted_timestamp : expected_result.timestamp};
            auto const& expected_marshalled_timestamp{
                    quoted_pattern ? quoted_timestamp : expected_result.timestamp
            };
            auto const& pattern{
                    quoted_pattern ? quoted_timestamp_pattern_result.value()
                                   : timestamp_pattern_result.value()
            };
            auto const& pattern_str{
                    quoted_pattern ? expected_quoted_pattern : expected_result.pattern
            };
            auto const& pattern_list{quoted_pattern ? default_quoted_patterns : default_patterns};
            auto const result{
                    parse_timestamp(timestamp, pattern, quoted_content, generated_pattern)
            };
            REQUIRE_FALSE(result.has_error());
            REQUIRE(expected_result.epoch_timestamp == result.value().first);
            REQUIRE(pattern_str == result.value().second);

            auto const searched_result{search_known_timestamp_patterns(
                    timestamp,
                    pattern_list,
                    quoted_content,
                    generated_pattern
            )};
            REQUIRE(searched_result.has_value());
            // NOLINTBEGIN(bugprone-unchecked-optional-access)
            REQUIRE(expected_result.epoch_timestamp == searched_result.value().first);
            REQUIRE(pattern_str == searched_result.value().second);
            // NOLINTEND(bugprone-unchecked-optional-access)

            std::string marshalled_timestamp;
            auto const marshal_result{marshal_timestamp(
                    expected_result.epoch_timestamp,
                    pattern,
                    marshalled_timestamp
            )};
            REQUIRE_FALSE(marshal_result.has_error());
            REQUIRE(expected_marshalled_timestamp == marshalled_timestamp);
        }
    }

    SECTION("Timestamps containing JSON escape sequences are parsed accurately.") {
        std::vector<ExpectedParsingResult> const expected_parsing_results{
                {R"(2015\\02\\01T01:02:03.004)",
                 R"(\Y\\\m\\\dT\H:\M:\S.\3)",
                 1'422'752'523'004'000'000},
        };

        std::string generated_pattern;
        for (auto const& expected_result : expected_parsing_results) {
            auto const timestamp_pattern_result{TimestampPattern::create(expected_result.pattern)};
            REQUIRE_FALSE(timestamp_pattern_result.has_error());
            auto const parse_result{parse_timestamp(
                    expected_result.timestamp,
                    timestamp_pattern_result.value(),
                    true,
                    generated_pattern
            )};
            REQUIRE_FALSE(parse_result.has_error());
            REQUIRE(expected_result.epoch_timestamp == parse_result.value().first);
            REQUIRE(expected_result.pattern == parse_result.value().second);

            std::string marshalled_timestamp;
            auto const marshal_result{marshal_timestamp(
                    expected_result.epoch_timestamp,
                    timestamp_pattern_result.value(),
                    marshalled_timestamp
            )};
            REQUIRE_FALSE(marshal_result.has_error());
            REQUIRE(expected_result.timestamp == marshalled_timestamp);
        }
    }
}
}  // namespace clp_s::timestamp_parser::test
