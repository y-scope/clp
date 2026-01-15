#include "TimestampParser.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <date/date.h>
#include <fmt/format.h>
#include <string_utils/string_utils.hpp>
#include <ystdlib/error_handling/Result.hpp>

#include "../Defs.hpp"
#include "ErrorCode.hpp"

namespace clp_s::timestamp_parser {
namespace {
constexpr int cMinParsedDay{1};
constexpr int cMaxParsedDay{31};
constexpr int cMinParsedMonth{1};
constexpr int cMaxParsedMonth{12};
constexpr int cMinParsedYear{0};
constexpr int cMaxParsedYear{9999};
constexpr int cTwoDigitYearOffsetBoundary{69};
constexpr int cTwoDigitYearLowOffset{1900};
constexpr int cTwoDigitYearHighOffset{2000};
constexpr int cMinParsedHour24HourClock{0};
constexpr int cMaxParsedHour24HourClock{23};
constexpr int cMinParsedHour12HourClock{1};
constexpr int cMaxParsedHour12HourClock{12};
constexpr int cMinParsedMinute{0};
constexpr int cMaxParsedMinute{59};
constexpr int cMinParsedSecond{0};
constexpr int cMaxParsedSecond{59};
constexpr int cLeapSecond{60};
constexpr int cMinParsedSubsecondNanoseconds{0};
constexpr int cMinTimezoneOffsetHour{0};
constexpr int cMaxTimezoneOffsetHour{23};
constexpr int cMinTimezoneOffsetMinute{0};
constexpr int cMaxTimezoneOffsetMinute{59};

constexpr size_t cNumNanosecondPrecisionSubsecondDigits{9ULL};
constexpr size_t cNumMicrosecondPrecisionSubsecondDigits{6ULL};
constexpr size_t cNumMillisecondPrecisionSubsecondDigits{3ULL};
constexpr size_t cNumSecondPrecisionSubsecondDigits{0ULL};

constexpr int cMinutesInHour{60};
constexpr size_t cDaysInWeek{7};
constexpr size_t cMonthsInYear{12};

constexpr int cDefaultYear{1970};
constexpr int cDefaultMonth{1};
constexpr int cDefaultDay{1};

constexpr int64_t cEpochMilliseconds1971{31'536'000'000};
constexpr int64_t cEpochMicroseconds1971{31'536'000'000'000};
constexpr int64_t cEpochNanoseconds1971{31'536'000'000'000'000};

constexpr std::array cPartsOfDay = {std::string_view{"AM"}, std::string_view{"PM"}};

constexpr std::array<int64_t, 10ULL> cPowersOfTen
        = {1, 10, 100, 1000, 10'000, 100'000, 1'000'000, 10'000'000, 100'000'000, 1'000'000'000};

constexpr std::array cPlusMinus
        = {std::string_view{"+"}, std::string_view{"-"}, std::string_view{"\u2212"}};

constexpr std::string_view cUtc{"UTC"};
constexpr std::string_view cSpace{" "};
constexpr std::string_view cZulu{"Z"};

constexpr std::array cDefaultDateTimePatterns{
        std::string_view{R"(\Y\O{-/}\m\O{-/}\d\O{T }\H:\M:\s\O{,.}\?\Z)"},
        std::string_view{R"(\Y\O{-/}\m\O{-/}\d\O{T }\H:\M:\s\Z)"},
        std::string_view{R"(\Y\O{-/}\m\O{-/}\d\O{T }\H:\M:\s\O{,.}\?)"},
        std::string_view{R"(\Y\O{-/}\m\O{-/}\d\O{T }\H:\M:\s)"},
        std::string_view{R"([\Y\O{-/}\m\O{-/}\d\O{T }\H:\M:\s\O{,.}\?])"},
        std::string_view{R"([\Y\O{-/}\m\O{-/}\d\O{T }\H:\M:\s])"},
        std::string_view{R"([\Y\O{-/}\m\O{-/}\d\O{T }\H:\M:\s)"},
        std::string_view{R"(<<<\Y\O{-/}\m\O{-/}\d\O{T }\H:\M:\s:\?)"},
        std::string_view{
                R"(\d \B{Jan,Feb,Mar,Apr,May,Jun,Jul,Aug,Sep,Oct,Nov,Dec} \Y \H:\M:\s\O{,.}\?)"
        },
        std::string_view{R"([\Y\m\d-\H:\M:\s])"},
        std::string_view{R"(\y\O{-/}\m\O{-/}\d\O{T }\H:\M:\s)"},
        std::string_view{R"(\y\m\d\O{T }\k:\M:\s)"},
        std::string_view{
                R"(\B{Jan,Feb,Mar,Apr,May,Jun,Jul,Aug,Sep,Oct,Nov,Dec} \d, \Y \l:\M:\s \p)"
        },
        std::string_view{
                R"(\B{Jan,Feb,Mar,Apr,May,Jun,Jul,Aug,Sep,Oct,Nov,Dec} \d, \Y \I:\M:\s \p)"
        },
        std::string_view{
                R"(\B{January,February,March,April,May,June,July,August,September,October,)"
                R"(November,December} \d, \Y \H:\M)"
        },
        std::string_view{
                R"([\d\O{-/}\B{Jan,Feb,Mar,Apr,May,Jun,Jul,Aug,Sep,Oct,Nov,Dec}\O{-/}\Y:\H:\M:\s)"
        },
        std::string_view{
                R"(\A{Sun,Mon,Tue,Wed,Thu,Fri,Sat} \B{Jan,Feb,Mar,Apr,May,Jun,Jul,Aug,Sep,Oct,Nov,)"
                R"(Dec} \e \H:\M:\s \Y)"
        },
        std::string_view{R"(\B{Jan,Feb,Mar,Apr,May,Jun,Jul,Aug,Sep,Oct,Nov,Dec} \d \H:\M:\s)"},
        std::string_view{R"(\B{Jan,Feb,Mar,Apr,May,Jun,Jul,Aug,Sep,Oct,Nov,Dec} \d \H:\M:\s\Z)"},
        std::string_view{R"(\m\O{- }\d \H:\M:\s\O{,.}\?)"}
};

constexpr std::array cDefaultNumericPatterns{
        std::string_view{R"(\P)"},
        std::string_view{R"(\E.\?)"}
};

constexpr std::string_view cJsonEscapedBackslash{R"(\\)"};

struct CatSequenceReplacement {
public:
    CatSequenceReplacement(size_t start_idx, size_t length, std::string replacement)
            : start_idx{start_idx},
              length{length},
              replacement{std::move(replacement)} {}

    size_t start_idx;
    size_t length;
    std::string replacement;
};

/**
 * Converts a padded decimal integer string to an integer.
 * @param str Substring containing the padded decimal integer string.
 * @param padding_character Padding character which may prefix the integer.
 * @return A result containing the integer value, or an error code indicating the failure:
 * - ErrorCodeEnum::IncompatibleTimestampPattern if the substring can not be converted to an
 *   integer.
 */
[[nodiscard]] auto convert_padded_string_to_number(std::string_view str, char padding_character)
        -> ystdlib::error_handling::Result<int>;

/**
 * Finds the first matching prefix from a list of candidates.
 * @param str Substring with a prefix potentially matching one of the candidates.
 * @param candidates Candidate prefixes that will be matched against `str`.
 * @return A result containing the index of the matching prefix in the candidates array, or an error
 * code indicating the failure:
 * - ErrorCodeEnum::IncompatibleTimestampPattern if no candidates match the prefix of `str`.
 */
[[nodiscard]] auto
find_first_matching_prefix(std::string_view str, std::span<std::string_view const> candidates)
        -> ystdlib::error_handling::Result<size_t>;

/**
 * Finds the first matching prefix from a list of candidates.
 * @param str Substring with a prefix potentially matching one of the candidates.
 * @param candidates Candidate prefixes, as a comma-separated list.
 * @param candidate_substrings Offsets and lengths describing candidate strings in `candidates`.
 * @return A result containing the index of the matching prefix in the candidates array, or an error
 * code indicating the failure:
 * - ErrorCodeEnum::IncompatibleTimestampPattern if no candidates match the prefix of `str`.
 */
[[nodiscard]] auto find_first_matching_prefix(
        std::string_view str,
        std::string_view candidates,
        std::vector<std::pair<uint16_t, uint16_t>> const& candidate_substrings
) -> ystdlib::error_handling::Result<size_t>;

/**
 * Converts the prefix of a string to a positive number up to a maximum number of digits.
 * @param str Substring with a prefix potentially corresponding to a number.
 * @param max_num_digits The maximum number of digits to convert to a number.
 * @return A result containing a pair holding the integer value and number of digits consumed, or an
 * error code indicating the failure:
 * - ErrorCodeEnum::InvalidTimestampPattern if `max_num_digits` is zero.
 * - ErrorCodeEnum::IncompatibleTimestampPattern if the prefix of the string is negative or doesn't
 *   correspond to a number.
 */
[[nodiscard]] auto convert_positive_bounded_variable_length_string_prefix_to_number(
        std::string_view str,
        size_t max_num_digits
) -> ystdlib::error_handling::Result<std::pair<int64_t, size_t>>;

/**
 * Converts the prefix of a string to a number.
 * @param str Substring with a prefix potentially corresponding to a number.
 * @return A result containing a pair holding the integer value and number of digits consumed, or an
 * error code indicating the failure:
 * - ErrorCodeEnum::IncompatibleTimestampPattern if the prefix of the string doesn't correspond to a
 *   number.
 */
[[nodiscard]] auto convert_variable_length_string_prefix_to_number(std::string_view str)
        -> ystdlib::error_handling::Result<std::pair<int64_t, size_t>>;

/**
 * Extracts a bracket pattern delimited by `{` and `}` from the prefix of a string.
 *
 * For simplicity, the `\` character is not allowed to appear between brackets. Note that since we
 * completely disallow `\` in the current implementation, it is possible to add support for escape
 * sequences within brackets in a backwards-compatible way.
 *
 * @param str Substring with a prefix potentially corresponding to a bracket pattern.
 * @return A result containing a string_view starting with the opening bracket and ending with the
 * closing bracket, or an error code indicating the failure:
 * - ErrorCodeEnum::InvalidTimestampPattern if:
 *     - There is no opening or closing bracket.
 *     - There is a `\` character between the brackets.
 *     - There are no characters between `{` and `}`.
 */
[[nodiscard]] auto extract_bracket_pattern(std::string_view str)
        -> ystdlib::error_handling::Result<std::string_view>;

/**
 * Extracts a timezone offset and determines its value in minutes from the prefix of a string.
 * @param str Substring with a prefix potentially corresponding to a timezone offset.
 * @return A result containing a pair holding the prefix corresponding to the extracted timezone
 * offset and the offset in minutes, or an error code indicating the failure:
 * - ErrorCodeEnum::InvalidTimezoneOffset if the prefix of the string doesn't correspond to a
 *   timezone offset.
 */
[[nodiscard]] auto extract_timezone_offset_in_minutes(std::string_view str)
        -> ystdlib::error_handling::Result<std::pair<std::string_view, int>>;

/**
 * Extracts the elements of a comma separated list from a bracket pattern.
 * @param str The content between the brackets of the bracket pattern.
 * @return A results containing the offsets and lengths of every element in the list, or an error
 * code indicating the failure:
 * - ErrorCodeEnum::InvalidTimestampPattern if:
 *     - Any element of the list contains the character '\\' or ' '.
 *     - Any element of the list has length zero.
 *     - The total length of the content can not be represented by a `uint16_t`.
 */
[[nodiscard]] auto extract_bracket_pattern_list(std::string_view str)
        -> ystdlib::error_handling::Result<std::vector<std::pair<uint16_t, uint16_t>>>;

/**
 * @return The absolute value of the subsecond fractional component of a timestamp.
 */
[[nodiscard]] auto extract_absolute_subsecond_nanoseconds(epochtime_t timestamp) -> epochtime_t;

/**
 * Estimates the precision of an epoch timestamp based on its proximity to 1971 in different
 * precisions.
 *
 * This heuristic works because one year in epoch nanoseconds is approximately 1000 years in epoch
 * microseconds, and so on. Note that this heuristic can not distinguish the precision of timestamps
 * with absolute value sufficiently close to zero.
 *
 * @param timestamp
 * @return A pair containing the scaling factor needed to convert the timestamp into nanosecond
 * precision, and a format specifier indicating the precision of the timestamp.
 */
[[nodiscard]] auto estimate_timestamp_precision(int64_t timestamp) -> std::pair<int64_t, char>;

/**
 * Marshals a date-time timestamp according to a timestamp pattern.
 * @param timestamp
 * @param pattern
 * @param buffer The buffer that the marshalled timestamp is appended to.
 * @return A void result on success, or an error code indicating the failure:
 * - ErrorCodeEnum::InvalidTimestampPattern if `pattern` contains format malformed format
 *   specifiers, or format specifiers that aren't supported in date-time timestamps.
 * - ErrorCodeEnum::IncompatibleTimestampPattern if `timestamp` can not be represented by `pattern`.
 * - ErrorCodeEnum::InvalidEscapeSequence `pattern` contains an unsupported escape sequence.
 */
[[nodiscard]] auto marshal_date_time_timestamp(
        epochtime_t timestamp,
        TimestampPattern const& pattern,
        std::string& buffer
) -> ystdlib::error_handling::Result<void>;

/**
 * Marshals a numeric timestamp according to a timestamp pattern.
 * @param timestamp
 * @param pattern
 * @param buffer The buffer that the marshalled timestamp is appended to.
 * @return A void result on success, or an error code indicating the failure:
 * - ErrorCodeEnum::IncompatibleTimestampPattern if `timestamp` can not be represented by `pattern`.
 * - ErrorCodeEnum::InvalidEscapeSequence `pattern` contains an unsupported escape sequence.
 */
[[nodiscard]] auto marshal_numeric_timestamp(
        epochtime_t timestamp,
        TimestampPattern const& pattern,
        std::string& buffer
) -> ystdlib::error_handling::Result<void>;

/**
 * @param c
 * @return Whether `c` indicates an unsupported JSON escape sequence when treated as an escaped
 * character.
 */
[[nodiscard]] auto is_unsupported_json_escape_sequence(char c) -> bool;

/**
 * @param c
 * @return Whether `c` is an unsupported character.
 */
[[nodiscard]] auto is_unsupported_character(char c) -> bool;

/**
 * @param c
 * @return Whether `c` is a repeatable escape sequence.
 */
[[nodiscard]] auto is_repeatable_escape_sequence(char c) -> bool;

auto convert_padded_string_to_number(std::string_view str, char padding_character)
        -> ystdlib::error_handling::Result<int> {
    if (str.empty()) {
        return ErrorCode{ErrorCodeEnum::IncompatibleTimestampPattern};
    }

    // Leave at least one character for parsing to ensure we actually parse number content.
    size_t i{};
    for (; i < (str.size() - 1) && padding_character == str.at(i); ++i) {}

    // Check if the string is zero-padded after stripping `padding_character`, since
    // `convert_string_to_int` allows zero-padding.
    auto const stripped_str{str.substr(i)};
    if (stripped_str.size() > 1ULL && '0' == stripped_str.front()) {
        return ErrorCode{ErrorCodeEnum::IncompatibleTimestampPattern};
    }

    int value{};
    if (clp::string_utils::convert_string_to_int(stripped_str, value)) {
        return value;
    }
    return ErrorCode{ErrorCodeEnum::IncompatibleTimestampPattern};
}

auto find_first_matching_prefix(std::string_view str, std::span<std::string_view const> candidates)
        -> ystdlib::error_handling::Result<size_t> {
    for (size_t candidate_idx{0ULL}; candidate_idx < candidates.size(); ++candidate_idx) {
        auto const& candidate{candidates[candidate_idx]};
        if (str.starts_with(candidate)) {
            return candidate_idx;
        }
    }
    return ErrorCode{ErrorCodeEnum::IncompatibleTimestampPattern};
}

auto find_first_matching_prefix(
        std::string_view str,
        std::string_view candidates,
        std::vector<std::pair<uint16_t, uint16_t>> const& candidate_substrings
) -> ystdlib::error_handling::Result<size_t> {
    for (size_t candidate_idx{0ULL}; candidate_idx < candidate_substrings.size(); ++candidate_idx) {
        auto const [substring_offset, substring_length] = candidate_substrings[candidate_idx];
        if (str.starts_with(candidates.substr(substring_offset, substring_length))) {
            return candidate_idx;
        }
    }
    return ErrorCode{ErrorCodeEnum::IncompatibleTimestampPattern};
}

auto convert_positive_bounded_variable_length_string_prefix_to_number(
        std::string_view str,
        size_t max_num_digits
) -> ystdlib::error_handling::Result<std::pair<int64_t, size_t>> {
    constexpr int64_t cTen{10};
    if (0ULL == max_num_digits) {
        return ErrorCode{ErrorCodeEnum::InvalidTimestampPattern};
    }
    if (str.empty() || '-' == str.at(0ULL)
        || false == clp::string_utils::is_decimal_digit(str.at(0ULL)))
    {
        return ErrorCode{ErrorCodeEnum::IncompatibleTimestampPattern};
    }

    int64_t converted_value{};
    size_t num_decimal_digits{};
    while (true) {
        char const cur_digit{str.at(num_decimal_digits)};
        converted_value += static_cast<int64_t>(cur_digit - '0');
        ++num_decimal_digits;

        if (num_decimal_digits >= str.length() || num_decimal_digits >= max_num_digits
            || false == clp::string_utils::is_decimal_digit(str.at(num_decimal_digits)))
        {
            break;
        }
        converted_value *= cTen;
    }
    return std::make_pair(converted_value, num_decimal_digits);
}

auto convert_variable_length_string_prefix_to_number(std::string_view str)
        -> ystdlib::error_handling::Result<std::pair<int64_t, size_t>> {
    constexpr int64_t cTen{10};
    if (str.empty()) {
        return ErrorCode{ErrorCodeEnum::IncompatibleTimestampPattern};
    }

    bool const is_negative{'-' == str.at(0ULL)};
    size_t num_decimal_digits{is_negative ? 1ULL : 0ULL};
    if (num_decimal_digits >= str.length()
        || false == clp::string_utils::is_decimal_digit(str.at(num_decimal_digits)))
    {
        return ErrorCode{ErrorCodeEnum::IncompatibleTimestampPattern};
    }

    bool const first_digit_zero{'0' == str.at(num_decimal_digits)};
    if (first_digit_zero && is_negative) {
        return ErrorCode{ErrorCodeEnum::IncompatibleTimestampPattern};
    }

    int64_t converted_value{};
    while (true) {
        char const cur_digit{str.at(num_decimal_digits)};
        converted_value += static_cast<int64_t>(cur_digit - '0');
        ++num_decimal_digits;

        if (num_decimal_digits >= str.length()
            || false == clp::string_utils::is_decimal_digit(str.at(num_decimal_digits)))
        {
            break;
        }
        converted_value *= cTen;
    }

    if (first_digit_zero && num_decimal_digits > 1) {
        return ErrorCode{ErrorCodeEnum::IncompatibleTimestampPattern};
    }

    if (is_negative) {
        converted_value *= -1;
    }
    return std::make_pair(converted_value, num_decimal_digits);
}

auto extract_bracket_pattern(std::string_view str)
        -> ystdlib::error_handling::Result<std::string_view> {
    if (str.empty() || '{' != str.front()) {
        return ErrorCode{ErrorCodeEnum::InvalidTimestampPattern};
    }

    for (size_t pattern_idx{1ULL}; pattern_idx < str.size(); ++pattern_idx) {
        if ('\\' == str.at(pattern_idx)) {
            return ErrorCode{ErrorCodeEnum::InvalidTimestampPattern};
        }
        if ('}' == str.at(pattern_idx)) {
            if (1ULL == pattern_idx) {
                return ErrorCode{ErrorCodeEnum::InvalidTimestampPattern};
            }
            return str.substr(0ULL, pattern_idx + 1ULL);
        }
    }
    return ErrorCode{ErrorCodeEnum::InvalidTimestampPattern};
}

auto extract_timezone_offset_in_minutes(std::string_view str)
        -> ystdlib::error_handling::Result<std::pair<std::string_view, int>> {
    auto const plus_minus_result{find_first_matching_prefix(str, cPlusMinus)};
    if (plus_minus_result.has_error()) {
        return ErrorCode{ErrorCodeEnum::InvalidTimezoneOffset};
    }
    int const sign_factor{0 == plus_minus_result.value() ? 1 : -1};
    size_t num_timezone_bytes{cPlusMinus.at(plus_minus_result.value()).length()};

    constexpr size_t cHoursFieldLength{2ULL};
    if (num_timezone_bytes + cHoursFieldLength > str.size()) {
        return ErrorCode{ErrorCodeEnum::InvalidTimezoneOffset};
    }
    auto const hours_result{
            convert_padded_string_to_number(str.substr(num_timezone_bytes, cHoursFieldLength), '0')
    };
    if (hours_result.has_error()) {
        return ErrorCode{ErrorCodeEnum::InvalidTimezoneOffset};
    }
    num_timezone_bytes += cHoursFieldLength;
    auto const hours_offset{hours_result.value()};
    if (hours_offset < cMinTimezoneOffsetHour || hours_offset > cMaxTimezoneOffsetHour) {
        return ErrorCode{ErrorCodeEnum::InvalidTimezoneOffset};
    }
    int offset{hours_offset * cMinutesInHour};
    auto const hours_only_offset{
            std::make_pair(str.substr(0ULL, num_timezone_bytes), sign_factor * offset)
    };
    if (str.size() == num_timezone_bytes) {
        return hours_only_offset;
    }

    if (':' == str.at(num_timezone_bytes)) {
        ++num_timezone_bytes;
    }

    constexpr size_t cMinutesFieldLength{2ULL};
    if (num_timezone_bytes + cMinutesFieldLength > str.size()) {
        return hours_only_offset;
    }
    auto const minutes_result{convert_padded_string_to_number(
            str.substr(num_timezone_bytes, cMinutesFieldLength),
            '0'
    )};
    if (minutes_result.has_error()) {
        return hours_only_offset;
    }
    auto const minutes_offset{minutes_result.value()};
    if (minutes_offset < cMinTimezoneOffsetMinute || minutes_offset > cMaxTimezoneOffsetMinute) {
        return hours_only_offset;
    }
    offset += minutes_offset;
    num_timezone_bytes += cMinutesFieldLength;
    return std::make_pair(str.substr(0ULL, num_timezone_bytes), sign_factor * offset);
}

auto extract_bracket_pattern_list(std::string_view str)
        -> ystdlib::error_handling::Result<std::vector<std::pair<uint16_t, uint16_t>>> {
    if (std::numeric_limits<uint16_t>::max() < str.size()) {
        return ErrorCode{ErrorCodeEnum::InvalidTimestampPattern};
    }

    size_t last_offset{};
    std::vector<std::pair<uint16_t, uint16_t>> entry_offsets_and_sizes;
    for (size_t i{}; i < str.size(); ++i) {
        switch (str.at(i)) {
            case ' ':
            case '\\':
                return ErrorCode{ErrorCodeEnum::InvalidTimestampPattern};
            case ',': {
                if (i <= last_offset) {
                    return ErrorCode{ErrorCodeEnum::InvalidTimestampPattern};
                }
                entry_offsets_and_sizes.emplace_back(last_offset, i - last_offset);
                last_offset = i + 1;
                break;
            }
            default:
                break;
        }
    }

    if (last_offset >= str.size()) {
        return ErrorCode{ErrorCodeEnum::InvalidTimestampPattern};
    }
    entry_offsets_and_sizes.emplace_back(last_offset, str.size() - last_offset);
    return entry_offsets_and_sizes;
}

auto estimate_timestamp_precision(int64_t timestamp) -> std::pair<int64_t, char> {
    auto const abs_timestamp = timestamp < 0 ? -timestamp : timestamp;
    if (abs_timestamp > cEpochNanoseconds1971) {
        return std::make_pair(1LL, 'N');
    }
    if (abs_timestamp > cEpochMicroseconds1971) {
        constexpr auto cFactor{cPowersOfTen
                                       [cNumNanosecondPrecisionSubsecondDigits
                                        - cNumMicrosecondPrecisionSubsecondDigits]};
        return std::make_pair(cFactor, 'C');
    }
    if (abs_timestamp > cEpochMilliseconds1971) {
        constexpr auto cFactor{cPowersOfTen
                                       [cNumNanosecondPrecisionSubsecondDigits
                                        - cNumMillisecondPrecisionSubsecondDigits]};
        return std::make_pair(cFactor, 'L');
    }
    constexpr auto cFactor{
            cPowersOfTen
                    [cNumNanosecondPrecisionSubsecondDigits - cNumSecondPrecisionSubsecondDigits]
    };
    return std::make_pair(cFactor, 'E');
}

auto extract_absolute_subsecond_nanoseconds(epochtime_t timestamp) -> epochtime_t {
    constexpr auto cFactor{
            cPowersOfTen
                    [cNumNanosecondPrecisionSubsecondDigits - cNumSecondPrecisionSubsecondDigits]
    };
    auto const subsecond_nanoseconds{timestamp % cFactor};
    return subsecond_nanoseconds < 0 ? -subsecond_nanoseconds : subsecond_nanoseconds;
}

// NOLINTBEGIN(readability-function-cognitive-complexity)
auto marshal_date_time_timestamp(
        epochtime_t timestamp,
        TimestampPattern const& pattern,
        std::string& buffer
) -> ystdlib::error_handling::Result<void> {
    auto const timestamp_point{
            date::sys_days(date::year(cDefaultYear) / cDefaultMonth / cDefaultDay)
            + std::chrono::nanoseconds(timestamp)
    };
    auto const timezone_minutes_offset{
            pattern.get_optional_timezone_size_and_offset().value_or(std::pair{0ULL, 0}).second
    };
    auto const timezone_adjusted_timestamp_point{
            timestamp_point + std::chrono::minutes(timezone_minutes_offset)
    };
    auto const timestamp_date{date::floor<date::days>(timezone_adjusted_timestamp_point)};
    auto const year_month_day{date::year_month_day(timestamp_date)};
    auto const time_of_day_duration{timezone_adjusted_timestamp_point - timestamp_date};
    auto const time_of_day{date::make_time(time_of_day_duration)};

    bool escaped{false};
    auto const raw_pattern{pattern.get_pattern()};
    for (size_t pattern_idx{0ULL}; pattern_idx < raw_pattern.size(); ++pattern_idx) {
        auto const c{raw_pattern.at(pattern_idx)};
        if (false == escaped && '\\' == c) {
            escaped = true;
            continue;
        }
        if (false == escaped) {
            buffer.push_back(c);
            continue;
        }
        escaped = false;
        switch (c) {
            case 'y': {  // Zero-padded 2-digit year in century.
                auto const year{year_month_day.year().operator int()};
                if (year >= cTwoDigitYearHighOffset) {
                    buffer.append(fmt::format("{:0>2d}", year - cTwoDigitYearHighOffset));
                } else {
                    buffer.append(fmt::format("{:0>2d}", year - cTwoDigitYearLowOffset));
                }
                break;
            }
            case 'Y': {  // Zero-padded 4-digit year.
                auto const year{year_month_day.year().operator int()};
                buffer.append(fmt::format("{:0>4d}", year));
                break;
            }
            case 'B': {  // Month name.
                auto const month_idx{year_month_day.month().operator unsigned int() - 1};
                buffer.append(YSTDLIB_ERROR_HANDLING_TRYX(
                        pattern.get_month_and_advance_pattern_idx(month_idx, pattern_idx)
                ));
                break;
            }
            case 'm': {  // Zero-padded month.
                auto const month{year_month_day.month().operator unsigned int()};
                buffer.append(fmt::format("{:0>2d}", month));
                break;
            }
            case 'd': {  // Zero-padded day in month.
                auto const day{year_month_day.day().operator unsigned int()};
                buffer.append(fmt::format("{:0>2d}", day));
                break;
            }
            case 'e': {  // Space-padded day in month.
                auto const day{year_month_day.day().operator unsigned int()};
                buffer.append(fmt::format("{: >2d}", day));
                break;
            }
            case 'A': {  // Day in week.
                auto const weekday_idx{
                        (date::year_month_weekday(timestamp_date).weekday_indexed().weekday()
                         - date::Sunday)
                                .count()
                };
                buffer.append(YSTDLIB_ERROR_HANDLING_TRYX(
                        pattern.get_weekday_and_advance_pattern_idx(weekday_idx, pattern_idx)
                ));
                break;
            }
            case 'p': {  // Part of day (AM/PM).
                auto const part_of_day_idx{
                        time_of_day.hours().count() >= cMaxParsedHour12HourClock ? 1ULL : 0ULL
                };
                buffer.append(cPartsOfDay.at(part_of_day_idx));
                break;
            }
            case 'H': {  // 24-hour clock, zero-padded hour.
                auto const hours{time_of_day.hours().count()};
                buffer.append(fmt::format("{:0>2d}", hours));
                break;
            }
            case 'k': {  // 24-hour clock, space-padded hour.
                auto const hours{time_of_day.hours().count()};
                buffer.append(fmt::format("{: >2d}", hours));
                break;
            }
            case 'I': {  // 12-hour clock, zero-padded hour.
                auto const hours{time_of_day.hours().count()};
                auto const hours_mod_twelve{hours % cMaxParsedHour12HourClock};
                auto const twelve_hour_clock_hours{
                        0 == hours_mod_twelve ? cMaxParsedHour12HourClock : hours_mod_twelve
                };
                buffer.append(fmt::format("{:0>2d}", twelve_hour_clock_hours));
                break;
            }
            case 'l': {  // 12-hour clock, space-padded hour.
                auto const hours{time_of_day.hours().count()};
                auto const hours_mod_twelve{hours % cMaxParsedHour12HourClock};
                auto const twelve_hour_clock_hours{
                        0 == hours_mod_twelve ? cMaxParsedHour12HourClock : hours_mod_twelve
                };
                buffer.append(fmt::format("{: >2d}", twelve_hour_clock_hours));
                break;
            }
            case 'M': {  // Zero-padded minute.
                auto const minutes{time_of_day.minutes().count()};
                buffer.append(fmt::format("{:0>2d}", minutes));
                break;
            }
            case 'S': {  // Zero-padded non-leap second.
                auto const seconds{time_of_day.seconds().count()};
                buffer.append(fmt::format("{:0>2d}", seconds));
                break;
            }
            case 'J': {  // Leap second.
                buffer.append(fmt::format("{:0>2d}", cLeapSecond));
                break;
            }
            case '3': {  // Zero-padded 3-digit milliseconds.
                constexpr auto cFactor{cPowersOfTen
                                               [cNumNanosecondPrecisionSubsecondDigits
                                                - cNumMillisecondPrecisionSubsecondDigits]};
                auto const subsecond_nanoseconds{time_of_day.subseconds().count()};
                auto const subsecond_milliseconds{subsecond_nanoseconds / cFactor};
                buffer.append(fmt::format("{:0>3d}", subsecond_milliseconds));
                break;
            }
            case '6': {  // Zero-padded 6-digit microseconds.
                constexpr auto cFactor{cPowersOfTen
                                               [cNumNanosecondPrecisionSubsecondDigits
                                                - cNumMicrosecondPrecisionSubsecondDigits]};
                auto const subsecond_nanoseconds{time_of_day.subseconds().count()};
                auto const subsecond_microseconds{subsecond_nanoseconds / cFactor};
                buffer.append(fmt::format("{:0>6d}", subsecond_microseconds));
                break;
            }
            case '9': {  // Zero-padded 9-digit nanoseconds.
                auto const subsecond_nanoseconds{time_of_day.subseconds().count()};
                buffer.append(fmt::format("{:0>9d}", subsecond_nanoseconds));
                break;
            }
            case 'T': {  // Zero-padded fractional seconds without trailing zeroes, max 9-digits.
                auto const subsecond_nanoseconds{time_of_day.subseconds().count()};
                auto const subsecond_nanoseconds_str{fmt::format("{:0>9d}", subsecond_nanoseconds)};
                size_t num_digits_before_zero{subsecond_nanoseconds_str.size()};
                while (num_digits_before_zero > 0
                       && '0' == subsecond_nanoseconds_str.at(num_digits_before_zero - 1))
                {
                    --num_digits_before_zero;
                }
                if (0 == num_digits_before_zero) {
                    return ErrorCode{ErrorCodeEnum::IncompatibleTimestampPattern};
                }
                buffer.append(
                        std::string_view{subsecond_nanoseconds_str}
                                .substr(0ULL, num_digits_before_zero)
                );
                break;
            }
            case 'z': {  // Timezone offset.
                auto const timezone_offset{pattern.get_optional_timezone_size_and_offset()};
                if (false == timezone_offset.has_value()) {
                    return ErrorCode{ErrorCodeEnum::InvalidTimestampPattern};
                }
                auto const timezone_pattern_size{timezone_offset.value().first};
                buffer.append(raw_pattern.substr(pattern_idx + 2ULL, timezone_pattern_size));
                pattern_idx += timezone_pattern_size + 2ULL;
                break;
            }
            case '\\': {
                buffer.append(cJsonEscapedBackslash);
                break;
            }
            default:
                return ErrorCode{ErrorCodeEnum::InvalidEscapeSequence};
        }
    }
    return ystdlib::error_handling::success();
}

// NOLINTEND(readability-function-cognitive-complexity)

auto marshal_numeric_timestamp(
        epochtime_t timestamp,
        TimestampPattern const& pattern,
        std::string& buffer
) -> ystdlib::error_handling::Result<void> {
    bool escaped{false};
    for (auto const c : pattern.get_pattern()) {
        if (false == escaped && '\\' == c) {
            escaped = true;
            continue;
        }
        if (false == escaped) {
            buffer.push_back(c);
            continue;
        }
        escaped = false;
        switch (c) {
            case 'E': {  // Epoch seconds.
                constexpr auto cFactor{cPowersOfTen
                                               [cNumNanosecondPrecisionSubsecondDigits
                                                - cNumSecondPrecisionSubsecondDigits]};
                int64_t const epoch_second_timestamp{timestamp / cFactor};
                buffer.append(fmt::format("{}", epoch_second_timestamp));
                break;
            }
            case 'L': {  // Epoch milliseconds.
                constexpr auto cFactor{cPowersOfTen
                                               [cNumNanosecondPrecisionSubsecondDigits
                                                - cNumMillisecondPrecisionSubsecondDigits]};
                int64_t const epoch_millisecond_timestamp{timestamp / cFactor};
                buffer.append(fmt::format("{}", epoch_millisecond_timestamp));
                break;
            }
            case 'C': {  // Epoch microseconds.
                constexpr auto cFactor{cPowersOfTen
                                               [cNumNanosecondPrecisionSubsecondDigits
                                                - cNumMicrosecondPrecisionSubsecondDigits]};
                int64_t const epoch_microsecond_timestamp{timestamp / cFactor};
                buffer.append(fmt::format("{}", epoch_microsecond_timestamp));
                break;
            }
            case 'N': {  // Epoch nanoseconds.
                buffer.append(fmt::format("{}", timestamp));
                break;
            }
            case '3': {  // Zero-padded 3-digit milliseconds.
                constexpr auto cFactor{cPowersOfTen
                                               [cNumNanosecondPrecisionSubsecondDigits
                                                - cNumMillisecondPrecisionSubsecondDigits]};
                auto const subsecond_nanoseconds{extract_absolute_subsecond_nanoseconds(timestamp)};
                auto const subsecond_milliseconds{subsecond_nanoseconds / cFactor};
                buffer.append(fmt::format("{:0>3d}", subsecond_milliseconds));
                break;
            }
            case '6': {  // Zero-padded 6-digit microseconds.
                constexpr auto cFactor{cPowersOfTen
                                               [cNumNanosecondPrecisionSubsecondDigits
                                                - cNumMicrosecondPrecisionSubsecondDigits]};
                auto const subsecond_nanoseconds{extract_absolute_subsecond_nanoseconds(timestamp)};
                auto const subsecond_microseconds{subsecond_nanoseconds / cFactor};
                buffer.append(fmt::format("{:0>6d}", subsecond_microseconds));
                break;
            }
            case '9': {  // Zero-padded 9-digit nanoseconds.
                auto const subsecond_nanoseconds{extract_absolute_subsecond_nanoseconds(timestamp)};
                buffer.append(fmt::format("{:0>9d}", subsecond_nanoseconds));
                break;
            }
            case 'T': {  // Zero-padded fractional seconds without trailing zeroes, max 9-digits.
                auto const subsecond_nanoseconds{extract_absolute_subsecond_nanoseconds(timestamp)};
                auto const subsecond_nanoseconds_str{fmt::format("{:0>9d}", subsecond_nanoseconds)};
                size_t num_digits_before_zero{subsecond_nanoseconds_str.size()};
                while (num_digits_before_zero > 0
                       && '0' == subsecond_nanoseconds_str.at(num_digits_before_zero - 1))
                {
                    --num_digits_before_zero;
                }
                if (0 == num_digits_before_zero) {
                    return ErrorCode{ErrorCodeEnum::IncompatibleTimestampPattern};
                }
                buffer.append(
                        std::string_view{subsecond_nanoseconds_str}
                                .substr(0ULL, num_digits_before_zero)
                );
                break;
            }
            case '\\': {
                buffer.append(cJsonEscapedBackslash);
                break;
            }
            default:
                return ErrorCode{ErrorCodeEnum::InvalidEscapeSequence};
        }
    }
    return ystdlib::error_handling::success();
}

auto is_unsupported_json_escape_sequence(char c) -> bool {
    switch (c) {
        case '"':
        case 'b':
        case 'f':
        case 'n':
        case 'r':
        case 't':
        case 'u':
            return true;
        default:
            return false;
    }
}

auto is_unsupported_character(char c) -> bool {
    return '"' == c || ('\x00' <= c && c <= '\x1f');
}

auto is_repeatable_escape_sequence(char c) -> bool {
    switch (c) {
        case 'O':
        case '\\':
            return true;
        default:
            return false;
    }
}
}  // namespace

// NOLINTBEGIN(readability-function-cognitive-complexity)
auto TimestampPattern::create(std::string_view pattern)
        -> ystdlib::error_handling::Result<TimestampPattern> {
    std::vector<bool> format_specifiers(std::numeric_limits<unsigned char>::max() + 1ULL, false);
    bool uses_date_type_representation{false};
    bool uses_number_type_representation{false};
    bool has_part_of_day{false};
    bool uses_twelve_hour_clock{false};
    std::optional<std::pair<size_t, int>> optional_timezone_size_and_offset{std::nullopt};
    std::vector<std::pair<uint16_t, uint16_t>> month_name_offsets_and_lengths;
    std::vector<std::pair<uint16_t, uint16_t>> weekday_name_offsets_and_lengths;
    uint16_t month_name_bracket_pattern_length{};
    uint16_t weekday_name_bracket_pattern_length{};

    bool const is_quoted_pattern{
            pattern.size() >= 2 && '"' == pattern.front() && '"' == pattern.back()
    };
    size_t const start_idx{is_quoted_pattern ? 1ULL : 0ULL};
    size_t const end_idx{is_quoted_pattern ? pattern.size() - 1ULL : pattern.size()};
    bool escaped{false};
    for (size_t pattern_idx{start_idx}; pattern_idx < end_idx; ++pattern_idx) {
        auto const cur_format_specifier{pattern.at(pattern_idx)};
        if (is_unsupported_character(cur_format_specifier)) {
            return ErrorCode{ErrorCodeEnum::InvalidCharacter};
        }

        if (false == escaped) {
            if ('\\' == cur_format_specifier) {
                escaped = true;
            }
            continue;
        }

        if (is_unsupported_json_escape_sequence(cur_format_specifier)) {
            return ErrorCode{ErrorCodeEnum::InvalidEscapeSequence};
        }

        auto unsigned_cur_format_specifier = static_cast<unsigned char>(cur_format_specifier);
        if (false == is_repeatable_escape_sequence(cur_format_specifier)
            && format_specifiers.at(unsigned_cur_format_specifier))
        {
            return ErrorCode{ErrorCodeEnum::InvalidTimestampPattern};
        }
        format_specifiers[unsigned_cur_format_specifier] = true;

        escaped = false;
        switch (cur_format_specifier) {
            case 'y':  // Zero-padded 2-digit year in century.
            case 'Y':  // Zero-padded 4-digit year.
                uses_date_type_representation = true;
                break;
            case 'B': {  // Month name.
                auto const month_name_bracket_pattern{YSTDLIB_ERROR_HANDLING_TRYX(
                        extract_bracket_pattern(pattern.substr(pattern_idx + 1ULL))
                )};
                auto const month_names_str{month_name_bracket_pattern.substr(
                        1ULL,
                        month_name_bracket_pattern.size() - 2ULL
                )};
                month_name_offsets_and_lengths = YSTDLIB_ERROR_HANDLING_TRYX(
                        extract_bracket_pattern_list(month_names_str)
                );
                if (cMonthsInYear != month_name_offsets_and_lengths.size()) {
                    return ErrorCode{ErrorCodeEnum::InvalidTimestampPattern};
                }
                month_name_bracket_pattern_length = month_names_str.size();
                pattern_idx += month_name_bracket_pattern.size();
                uses_date_type_representation = true;
                break;
            }
            case 'm':  // Zero-padded month.
            case 'd':  // Zero-padded day in month.
            case 'e':  // Space-padded day in month.
                uses_date_type_representation = true;
                break;
            case 'A': {  // Day in week.
                auto const weekday_name_bracket_pattern{YSTDLIB_ERROR_HANDLING_TRYX(
                        extract_bracket_pattern(pattern.substr(pattern_idx + 1ULL))
                )};
                auto const weekday_names_str{weekday_name_bracket_pattern.substr(
                        1ULL,
                        weekday_name_bracket_pattern.size() - 2ULL
                )};
                weekday_name_offsets_and_lengths = YSTDLIB_ERROR_HANDLING_TRYX(
                        extract_bracket_pattern_list(weekday_names_str)
                );
                if (cDaysInWeek != weekday_name_offsets_and_lengths.size()) {
                    return ErrorCode{ErrorCodeEnum::InvalidTimestampPattern};
                }
                weekday_name_bracket_pattern_length = weekday_names_str.size();
                pattern_idx += weekday_name_bracket_pattern.size();
                uses_date_type_representation = true;
                break;
            }
            case 'p':  // Part of day (AM/PM).
                uses_date_type_representation = true;
                has_part_of_day = true;
                break;
            case 'H':  // 24-hour clock, zero-padded hour.
            case 'k':  // 24-hour clock, space-padded hour.
                uses_date_type_representation = true;
                break;
            case 'I':  // 12-hour clock, zero-padded hour.
            case 'l':  // 12-hour clock, space-padded hour.
                uses_twelve_hour_clock = true;
                uses_date_type_representation = true;
                break;
            case 'M':  // Zero-padded minute.
            case 'S':  // Zero-padded non-leap second.
            case 'J':  // Leap second.
                uses_date_type_representation = true;
                break;
            case '3':  // Zero-padded 3-digit milliseconds.
            case '6':  // Zero-padded 6-digit microseconds.
            case '9':  // Zero-padded 9-digit nanoseconds.
            case 'T':  // Zero-padded fractional seconds without trailing zeroes, max 9-digits.
                break;
            case 'E':  // Epoch seconds.
            case 'L':  // Epoch milliseconds.
            case 'C':  // Epoch microseconds.
            case 'N':  // Epoch nanoseconds.
                uses_number_type_representation = true;
                break;
            case 'z': {  // Timezone offset.
                auto const timezone_bracket_pattern{YSTDLIB_ERROR_HANDLING_TRYX(
                        extract_bracket_pattern(pattern.substr(pattern_idx + 1ULL))
                )};
                auto const timezone_str{timezone_bracket_pattern.substr(
                        1ULL,
                        timezone_bracket_pattern.size() - 2ULL
                )};

                auto const [extracted_timezone_str, extracted_timezone_offset]
                        = YSTDLIB_ERROR_HANDLING_TRYX(
                                extract_timezone_offset_in_minutes(timezone_str)
                        );
                if (extracted_timezone_str.size() != timezone_str.size()) {
                    return ErrorCode{ErrorCodeEnum::InvalidTimestampPattern};
                }

                optional_timezone_size_and_offset.emplace(
                        extracted_timezone_str.size(),
                        extracted_timezone_offset
                );
                pattern_idx += timezone_bracket_pattern.size();
                uses_date_type_representation = true;
                break;
            }
            case 'Z':  // Generic timezone.
                uses_date_type_representation = true;
                break;
            case '?':  // Generic fractional second.
                break;
            case 'P':  // Unknown-precision epoch time.
                uses_number_type_representation = true;
                break;
            case 'O': {  // One of several literal characters.
                auto const bracket_pattern{YSTDLIB_ERROR_HANDLING_TRYX(
                        extract_bracket_pattern(pattern.substr(pattern_idx + 1ULL))
                )};
                pattern_idx += bracket_pattern.size();
                break;
            }
            case 's':  // Generic zero-padded second.
                uses_date_type_representation = true;
                break;
            case '\\':
                break;
            default:
                return ErrorCode{ErrorCodeEnum::InvalidEscapeSequence};
        }
    }

    if (escaped) {
        return ErrorCode{ErrorCodeEnum::InvalidTimestampPattern};
    }

    if (uses_date_type_representation && uses_number_type_representation) {
        return ErrorCode{ErrorCodeEnum::InvalidTimestampPattern};
    }

    if (uses_twelve_hour_clock != has_part_of_day) {
        return ErrorCode{ErrorCodeEnum::InvalidTimestampPattern};
    }

    return TimestampPattern{
            std::string{pattern},
            optional_timezone_size_and_offset,
            month_name_offsets_and_lengths,
            weekday_name_offsets_and_lengths,
            month_name_bracket_pattern_length,
            weekday_name_bracket_pattern_length,
            uses_date_type_representation,
            uses_twelve_hour_clock,
            is_quoted_pattern
    };
}

// NOLINTEND(readability-function-cognitive-complexity)

auto TimestampPattern::find_first_matching_month_and_advance_pattern_idx(
        std::string_view timestamp,
        size_t& pattern_idx
) const -> ystdlib::error_handling::Result<std::pair<size_t, size_t>> {
    auto const month_names{std::string_view{m_pattern}.substr(
            pattern_idx + 2ULL,
            m_month_name_bracket_pattern_length
    )};
    auto const month_idx{YSTDLIB_ERROR_HANDLING_TRYX(
            find_first_matching_prefix(timestamp, month_names, m_month_name_offsets_and_lengths)
    )};
    pattern_idx += m_month_name_bracket_pattern_length + 2ULL;
    return {month_idx, m_month_name_offsets_and_lengths.at(month_idx).second};
}

auto TimestampPattern::find_first_matching_weekday_and_advance_pattern_idx(
        std::string_view timestamp,
        size_t& pattern_idx
) const -> ystdlib::error_handling::Result<std::pair<size_t, size_t>> {
    auto const weekday_names{std::string_view{m_pattern}.substr(
            pattern_idx + 2ULL,
            m_weekday_name_bracket_pattern_length
    )};
    auto const weekday_idx{YSTDLIB_ERROR_HANDLING_TRYX(
            find_first_matching_prefix(timestamp, weekday_names, m_weekday_name_offsets_and_lengths)
    )};
    pattern_idx += m_weekday_name_bracket_pattern_length + 2ULL;
    return {weekday_idx, m_weekday_name_offsets_and_lengths.at(weekday_idx).second};
}

auto
TimestampPattern::get_month_and_advance_pattern_idx(size_t month_idx, size_t& pattern_idx) const
        -> ystdlib::error_handling::Result<std::string_view> {
    if (month_idx >= m_month_name_offsets_and_lengths.size()) {
        return ErrorCode{ErrorCodeEnum::IncompatibleTimestampPattern};
    }
    auto const month_names{std::string_view{m_pattern}.substr(
            pattern_idx + 2ULL,
            m_month_name_bracket_pattern_length
    )};
    auto const [month_offset, month_length] = m_month_name_offsets_and_lengths.at(month_idx);
    pattern_idx += m_month_name_bracket_pattern_length + 2ULL;
    return month_names.substr(month_offset, month_length);
}

auto
TimestampPattern::get_weekday_and_advance_pattern_idx(size_t weekday_idx, size_t& pattern_idx) const
        -> ystdlib::error_handling::Result<std::string_view> {
    if (weekday_idx >= m_weekday_name_offsets_and_lengths.size()) {
        return ErrorCode{ErrorCodeEnum::IncompatibleTimestampPattern};
    }
    auto const weekday_names{std::string_view{m_pattern}.substr(
            pattern_idx + 2ULL,
            m_weekday_name_bracket_pattern_length
    )};
    auto const [weekday_offset, weekday_length]
            = m_weekday_name_offsets_and_lengths.at(weekday_idx);
    pattern_idx += m_weekday_name_bracket_pattern_length + 2ULL;
    return weekday_names.substr(weekday_offset, weekday_length);
}

// NOLINTBEGIN(readability-function-cognitive-complexity)
auto parse_timestamp(
        std::string_view timestamp,
        TimestampPattern const& pattern,
        bool is_json_literal,
        std::string& generated_pattern
) -> ystdlib::error_handling::Result<std::pair<epochtime_t, std::string_view>> {
    size_t timestamp_idx{};

    int parsed_year{cDefaultYear};
    int parsed_month{cDefaultMonth};
    int parsed_day{cDefaultDay};
    int parsed_hour{};
    int parsed_minute{};
    int parsed_second{};
    int parsed_subsecond_nanoseconds{};
    std::optional<int> optional_day_of_week_idx;
    std::optional<int> optional_part_of_day_idx;
    std::optional<int> optional_timezone_offset_in_minutes;

    int64_t parsed_epoch_nanoseconds{};

    std::vector<CatSequenceReplacement> cat_sequence_replacements;

    bool escaped{false};
    auto const raw_pattern{pattern.get_pattern()};
    bool const should_skip_quotes{pattern.is_quoted_pattern() && false == is_json_literal};
    size_t pattern_idx{should_skip_quotes ? 1ULL : 0ULL};
    size_t const pattern_end_idx{raw_pattern.length() - (should_skip_quotes ? 1ULL : 0ULL)};
    for (; pattern_idx < pattern_end_idx && timestamp_idx < timestamp.size(); ++pattern_idx) {
        if (false == escaped) {
            if ('\\' == raw_pattern[pattern_idx]) {
                escaped = true;
                continue;
            }
            if (raw_pattern[pattern_idx] == timestamp[timestamp_idx]) {
                ++timestamp_idx;
                continue;
            }
            return ErrorCode{ErrorCodeEnum::IncompatibleTimestampPattern};
        }

        escaped = false;
        switch (raw_pattern[pattern_idx]) {
            case 'y': {  // Zero-padded 2-digit year in century.
                constexpr size_t cFieldLength{2};
                if (timestamp_idx + cFieldLength > timestamp.size()) {
                    return ErrorCode{ErrorCodeEnum::IncompatibleTimestampPattern};
                }

                auto const two_digit_year{
                        YSTDLIB_ERROR_HANDLING_TRYX(convert_padded_string_to_number(
                                timestamp.substr(timestamp_idx, cFieldLength),
                                '0'
                        ))
                };

                if (two_digit_year >= cTwoDigitYearOffsetBoundary) {
                    parsed_year = two_digit_year + cTwoDigitYearLowOffset;
                } else {
                    parsed_year = two_digit_year + cTwoDigitYearHighOffset;
                }
                timestamp_idx += cFieldLength;
                break;
            }
            case 'Y': {  // Zero-padded 4-digit year.
                constexpr size_t cFieldLength{4};
                if (timestamp_idx + cFieldLength > timestamp.size()) {
                    return ErrorCode{ErrorCodeEnum::IncompatibleTimestampPattern};
                }

                parsed_year = YSTDLIB_ERROR_HANDLING_TRYX(convert_padded_string_to_number(
                        timestamp.substr(timestamp_idx, cFieldLength),
                        '0'
                ));

                if (parsed_year < cMinParsedYear || parsed_year > cMaxParsedYear) {
                    return ErrorCode{ErrorCodeEnum::IncompatibleTimestampPattern};
                }

                timestamp_idx += cFieldLength;
                break;
            }
            case 'B': {  // Month name.
                auto const [month_idx, month_length] = YSTDLIB_ERROR_HANDLING_TRYX(
                        pattern.find_first_matching_month_and_advance_pattern_idx(
                                timestamp.substr(timestamp_idx),
                                pattern_idx
                        )
                );
                parsed_month = static_cast<int>(month_idx) + 1;
                timestamp_idx += month_length;
                break;
            }
            case 'm': {  // Zero-padded month.
                constexpr size_t cFieldLength{2};
                if (timestamp_idx + cFieldLength > timestamp.size()) {
                    return ErrorCode{ErrorCodeEnum::IncompatibleTimestampPattern};
                }

                parsed_month = YSTDLIB_ERROR_HANDLING_TRYX(convert_padded_string_to_number(
                        timestamp.substr(timestamp_idx, cFieldLength),
                        '0'
                ));

                if (parsed_month < cMinParsedMonth || parsed_month > cMaxParsedMonth) {
                    return ErrorCode{ErrorCodeEnum::IncompatibleTimestampPattern};
                }

                timestamp_idx += cFieldLength;
                break;
            }
            case 'd': {  // Zero-padded day in month.
                constexpr size_t cFieldLength{2};
                if (timestamp_idx + cFieldLength > timestamp.size()) {
                    return ErrorCode{ErrorCodeEnum::IncompatibleTimestampPattern};
                }

                parsed_day = YSTDLIB_ERROR_HANDLING_TRYX(convert_padded_string_to_number(
                        timestamp.substr(timestamp_idx, cFieldLength),
                        '0'
                ));

                if (parsed_day < cMinParsedDay || parsed_day > cMaxParsedDay) {
                    return ErrorCode{ErrorCodeEnum::IncompatibleTimestampPattern};
                }

                timestamp_idx += cFieldLength;
                break;
            }
            case 'e': {  // Space-padded day in month.
                constexpr size_t cFieldLength{2};
                if (timestamp_idx + cFieldLength > timestamp.size()) {
                    return ErrorCode{ErrorCodeEnum::IncompatibleTimestampPattern};
                }

                parsed_day = YSTDLIB_ERROR_HANDLING_TRYX(convert_padded_string_to_number(
                        timestamp.substr(timestamp_idx, cFieldLength),
                        ' '
                ));

                if (parsed_day < cMinParsedDay || parsed_day > cMaxParsedDay) {
                    return ErrorCode{ErrorCodeEnum::IncompatibleTimestampPattern};
                }

                timestamp_idx += cFieldLength;
                break;
            }
            case 'A': {  // Day in week.
                auto const [weekday_idx, weekday_length] = YSTDLIB_ERROR_HANDLING_TRYX(
                        pattern.find_first_matching_weekday_and_advance_pattern_idx(
                                timestamp.substr(timestamp_idx),
                                pattern_idx
                        )
                );
                timestamp_idx += weekday_length;
                optional_day_of_week_idx = static_cast<int>(weekday_idx);
                break;
            }
            case 'p': {  // Part of day (AM/PM).
                auto const part_of_day_idx{YSTDLIB_ERROR_HANDLING_TRYX(
                        find_first_matching_prefix(timestamp.substr(timestamp_idx), cPartsOfDay)
                )};
                timestamp_idx += cPartsOfDay.at(part_of_day_idx).length();
                optional_part_of_day_idx = static_cast<int>(part_of_day_idx);
                break;
            }
            case 'H': {  // 24-hour clock, zero-padded hour.
                constexpr size_t cFieldLength{2};
                if (timestamp_idx + cFieldLength > timestamp.size()) {
                    return ErrorCode{ErrorCodeEnum::IncompatibleTimestampPattern};
                }

                parsed_hour = YSTDLIB_ERROR_HANDLING_TRYX(convert_padded_string_to_number(
                        timestamp.substr(timestamp_idx, cFieldLength),
                        '0'
                ));

                if (parsed_hour < cMinParsedHour24HourClock
                    || parsed_hour > cMaxParsedHour24HourClock)
                {
                    return ErrorCode{ErrorCodeEnum::IncompatibleTimestampPattern};
                }

                timestamp_idx += cFieldLength;
                break;
            }
            case 'k': {  // 24-hour clock, space-padded hour.
                constexpr size_t cFieldLength{2};
                if (timestamp_idx + cFieldLength > timestamp.size()) {
                    return ErrorCode{ErrorCodeEnum::IncompatibleTimestampPattern};
                }

                parsed_hour = YSTDLIB_ERROR_HANDLING_TRYX(convert_padded_string_to_number(
                        timestamp.substr(timestamp_idx, cFieldLength),
                        ' '
                ));

                if (parsed_hour < cMinParsedHour24HourClock
                    || parsed_hour > cMaxParsedHour24HourClock)
                {
                    return ErrorCode{ErrorCodeEnum::IncompatibleTimestampPattern};
                }

                timestamp_idx += cFieldLength;
                break;
            }
            case 'I': {  // 12-hour clock, zero-padded hour.
                constexpr size_t cFieldLength{2};
                if (timestamp_idx + cFieldLength > timestamp.size()) {
                    return ErrorCode{ErrorCodeEnum::IncompatibleTimestampPattern};
                }

                parsed_hour = YSTDLIB_ERROR_HANDLING_TRYX(convert_padded_string_to_number(
                        timestamp.substr(timestamp_idx, cFieldLength),
                        '0'
                ));

                if (parsed_hour < cMinParsedHour12HourClock
                    || parsed_hour > cMaxParsedHour12HourClock)
                {
                    return ErrorCode{ErrorCodeEnum::IncompatibleTimestampPattern};
                }

                timestamp_idx += cFieldLength;
                break;
            }
            case 'l': {  // 12-hour clock, space-padded hour.
                constexpr size_t cFieldLength{2};
                if (timestamp_idx + cFieldLength > timestamp.size()) {
                    return ErrorCode{ErrorCodeEnum::IncompatibleTimestampPattern};
                }

                parsed_hour = YSTDLIB_ERROR_HANDLING_TRYX(convert_padded_string_to_number(
                        timestamp.substr(timestamp_idx, cFieldLength),
                        ' '
                ));

                if (parsed_hour < cMinParsedHour12HourClock
                    || parsed_hour > cMaxParsedHour12HourClock)
                {
                    return ErrorCode{ErrorCodeEnum::IncompatibleTimestampPattern};
                }

                timestamp_idx += cFieldLength;
                break;
            }
            case 'M': {  // Zero-padded minute.
                constexpr size_t cFieldLength{2};
                if (timestamp_idx + cFieldLength > timestamp.size()) {
                    return ErrorCode{ErrorCodeEnum::IncompatibleTimestampPattern};
                }

                parsed_minute = YSTDLIB_ERROR_HANDLING_TRYX(convert_padded_string_to_number(
                        timestamp.substr(timestamp_idx, cFieldLength),
                        '0'
                ));

                if (parsed_minute < cMinParsedMinute || parsed_minute > cMaxParsedMinute) {
                    return ErrorCode{ErrorCodeEnum::IncompatibleTimestampPattern};
                }

                timestamp_idx += cFieldLength;
                break;
            }
            case 'S': {  // Zero-padded non-leap second.
                constexpr size_t cFieldLength{2};
                if (timestamp_idx + cFieldLength > timestamp.size()) {
                    return ErrorCode{ErrorCodeEnum::IncompatibleTimestampPattern};
                }

                parsed_second = YSTDLIB_ERROR_HANDLING_TRYX(convert_padded_string_to_number(
                        timestamp.substr(timestamp_idx, cFieldLength),
                        '0'
                ));

                if (parsed_second < cMinParsedSecond || parsed_second > cMaxParsedSecond) {
                    return ErrorCode{ErrorCodeEnum::IncompatibleTimestampPattern};
                }

                timestamp_idx += cFieldLength;
                break;
            }
            case 'J': {  // Leap second.
                constexpr size_t cFieldLength{2};
                if (timestamp_idx + cFieldLength > timestamp.size()) {
                    return ErrorCode{ErrorCodeEnum::IncompatibleTimestampPattern};
                }

                if (cLeapSecond
                    != YSTDLIB_ERROR_HANDLING_TRYX(convert_padded_string_to_number(
                            timestamp.substr(timestamp_idx, cFieldLength),
                            '0'
                    )))
                {
                    return ErrorCode{ErrorCodeEnum::IncompatibleTimestampPattern};
                }
                parsed_second = cMaxParsedSecond;

                timestamp_idx += cFieldLength;
                break;
            }
            case '3': {  // Zero-padded 3-digit milliseconds.
                constexpr size_t cFieldLength{3};
                if (timestamp_idx + cFieldLength > timestamp.size()) {
                    return ErrorCode{ErrorCodeEnum::IncompatibleTimestampPattern};
                }
                if (false
                    == clp::string_utils::convert_string_to_int(
                            timestamp.substr(timestamp_idx, cFieldLength),
                            parsed_subsecond_nanoseconds
                    ))
                {
                    return ErrorCode{ErrorCodeEnum::IncompatibleTimestampPattern};
                }
                if (parsed_subsecond_nanoseconds < cMinParsedSubsecondNanoseconds) {
                    return ErrorCode{ErrorCodeEnum::IncompatibleTimestampPattern};
                }
                constexpr auto cFactor{cPowersOfTen
                                               [cNumNanosecondPrecisionSubsecondDigits
                                                - cNumMillisecondPrecisionSubsecondDigits]};
                parsed_subsecond_nanoseconds *= cFactor;
                timestamp_idx += cFieldLength;
                break;
            }
            case '6': {  // Zero-padded 6-digit microseconds.
                constexpr size_t cFieldLength{6};
                if (timestamp_idx + cFieldLength > timestamp.size()) {
                    return ErrorCode{ErrorCodeEnum::IncompatibleTimestampPattern};
                }
                if (false
                    == clp::string_utils::convert_string_to_int(
                            timestamp.substr(timestamp_idx, cFieldLength),
                            parsed_subsecond_nanoseconds
                    ))
                {
                    return ErrorCode{ErrorCodeEnum::IncompatibleTimestampPattern};
                }
                if (parsed_subsecond_nanoseconds < cMinParsedSubsecondNanoseconds) {
                    return ErrorCode{ErrorCodeEnum::IncompatibleTimestampPattern};
                }
                constexpr auto cFactor{cPowersOfTen
                                               [cNumNanosecondPrecisionSubsecondDigits
                                                - cNumMicrosecondPrecisionSubsecondDigits]};
                parsed_subsecond_nanoseconds *= cFactor;
                timestamp_idx += cFieldLength;
                break;
            }
            case '9': {  // Zero-padded 9-digit nanoseconds.
                constexpr size_t cFieldLength{9};
                if (timestamp_idx + cFieldLength > timestamp.size()) {
                    return ErrorCode{ErrorCodeEnum::IncompatibleTimestampPattern};
                }
                if (false
                    == clp::string_utils::convert_string_to_int(
                            timestamp.substr(timestamp_idx, cFieldLength),
                            parsed_subsecond_nanoseconds
                    ))
                {
                    return ErrorCode{ErrorCodeEnum::IncompatibleTimestampPattern};
                }
                if (parsed_subsecond_nanoseconds < cMinParsedSubsecondNanoseconds) {
                    return ErrorCode{ErrorCodeEnum::IncompatibleTimestampPattern};
                }
                timestamp_idx += cFieldLength;
                break;
            }
            case 'T': {  // Zero-padded fractional seconds without trailing zeroes, max 9-digits.
                constexpr size_t cMaxFieldLength{9};
                auto const remaining_unparsed_content{timestamp.substr(timestamp_idx)};
                auto const [number, num_digits] = YSTDLIB_ERROR_HANDLING_TRYX(
                        convert_positive_bounded_variable_length_string_prefix_to_number(
                                remaining_unparsed_content,
                                cMaxFieldLength
                        )
                );
                if ('0' == remaining_unparsed_content.at(num_digits - 1ULL)) {
                    return ErrorCode{ErrorCodeEnum::IncompatibleTimestampPattern};
                }
                timestamp_idx += num_digits;
                parsed_subsecond_nanoseconds
                        = static_cast<int>(number * cPowersOfTen.at(cMaxFieldLength - num_digits));
                break;
            }
            case 'E': {  // Epoch seconds.
                auto const [number, num_digits] = YSTDLIB_ERROR_HANDLING_TRYX(
                        convert_variable_length_string_prefix_to_number(
                                timestamp.substr(timestamp_idx)
                        )
                );
                timestamp_idx += num_digits;
                constexpr auto cFactor{cPowersOfTen
                                               [cNumNanosecondPrecisionSubsecondDigits
                                                - cNumSecondPrecisionSubsecondDigits]};
                parsed_epoch_nanoseconds = number * cFactor;
                break;
            }
            case 'L': {  // Epoch milliseconds.
                auto const [number, num_digits] = YSTDLIB_ERROR_HANDLING_TRYX(
                        convert_variable_length_string_prefix_to_number(
                                timestamp.substr(timestamp_idx)
                        )
                );
                timestamp_idx += num_digits;
                constexpr auto cFactor{cPowersOfTen
                                               [cNumNanosecondPrecisionSubsecondDigits
                                                - cNumMillisecondPrecisionSubsecondDigits]};
                parsed_epoch_nanoseconds = number * cFactor;
                break;
            }
            case 'C': {  // Epoch microseconds.
                auto const [number, num_digits] = YSTDLIB_ERROR_HANDLING_TRYX(
                        convert_variable_length_string_prefix_to_number(
                                timestamp.substr(timestamp_idx)
                        )
                );
                timestamp_idx += num_digits;
                constexpr auto cFactor{cPowersOfTen
                                               [cNumNanosecondPrecisionSubsecondDigits
                                                - cNumMicrosecondPrecisionSubsecondDigits]};
                parsed_epoch_nanoseconds = number * cFactor;
                break;
            }
            case 'N': {  // Epoch nanoseconds.
                auto const [number, num_digits] = YSTDLIB_ERROR_HANDLING_TRYX(
                        convert_variable_length_string_prefix_to_number(
                                timestamp.substr(timestamp_idx)
                        )
                );
                timestamp_idx += num_digits;
                parsed_epoch_nanoseconds = number;
                break;
            }
            case 'z': {  // Timezone offset.
                auto const& optional_timezone_size_and_offset{
                        pattern.get_optional_timezone_size_and_offset()
                };
                if (false == optional_timezone_size_and_offset.has_value()) {
                    return ErrorCode{ErrorCodeEnum::InvalidTimestampPattern};
                }

                auto const [extracted_timezone_size, extracted_timezone_offset]
                        = optional_timezone_size_and_offset.value();
                if (false
                    == timestamp.substr(timestamp_idx)
                               .starts_with(raw_pattern.substr(
                                       pattern_idx + 2ULL,
                                       extracted_timezone_size
                               )))
                {
                    return ErrorCode{ErrorCodeEnum::IncompatibleTimestampPattern};
                }

                optional_timezone_offset_in_minutes = extracted_timezone_offset;
                timestamp_idx += extracted_timezone_size;
                pattern_idx += extracted_timezone_size + 2ULL;
                break;
            }
            case 'Z': {  // Generic timezone.
                std::string timezone_pattern;
                auto remaining_unparsed_content{timestamp.substr(timestamp_idx)};
                if (remaining_unparsed_content.empty()) {
                    return ErrorCode{ErrorCodeEnum::IncompatibleTimestampPattern};
                }

                if (remaining_unparsed_content.starts_with(cSpace)) {
                    timezone_pattern.append(cSpace);
                    timestamp_idx += cSpace.size();
                    remaining_unparsed_content = remaining_unparsed_content.substr(cSpace.size());
                }
                if (remaining_unparsed_content.starts_with(cUtc)) {
                    timezone_pattern.append(cUtc);
                    timestamp_idx += cUtc.size();
                    remaining_unparsed_content = remaining_unparsed_content.substr(cUtc.size());
                }

                auto const extracted_timezone_result{
                        extract_timezone_offset_in_minutes(remaining_unparsed_content)
                };
                if (false == extracted_timezone_result.has_error()) {
                    auto const [extracted_timezone_str, extracted_timezone_offset]
                            = extracted_timezone_result.value();
                    timestamp_idx += extracted_timezone_str.size();
                    timezone_pattern.append(fmt::format(R"(\z{{{}}})", extracted_timezone_str));
                    remaining_unparsed_content
                            = remaining_unparsed_content.substr(extracted_timezone_str.size());
                    optional_timezone_offset_in_minutes = extracted_timezone_offset;
                }

                if (remaining_unparsed_content.starts_with(cZulu)) {
                    timezone_pattern.append(cZulu);
                    timestamp_idx += cZulu.size();
                    optional_timezone_offset_in_minutes
                            = optional_timezone_offset_in_minutes.value_or(0);
                }

                if (false == optional_timezone_offset_in_minutes.has_value()) {
                    return ErrorCode{ErrorCodeEnum::IncompatibleTimestampPattern};
                }
                cat_sequence_replacements
                        .emplace_back(pattern_idx - 1, 2ULL, std::move(timezone_pattern));
                break;
            }
            case '?': {  // Generic fractional second.
                constexpr size_t cMaxFieldLength{9};
                auto const remaining_unparsed_content{timestamp.substr(timestamp_idx)};
                auto const [number, num_digits] = YSTDLIB_ERROR_HANDLING_TRYX(
                        convert_positive_bounded_variable_length_string_prefix_to_number(
                                remaining_unparsed_content,
                                cMaxFieldLength
                        )
                );

                if (cNumNanosecondPrecisionSubsecondDigits == num_digits) {
                    cat_sequence_replacements.emplace_back(pattern_idx, 1ULL, "9");
                } else if (cNumMicrosecondPrecisionSubsecondDigits == num_digits) {
                    cat_sequence_replacements.emplace_back(pattern_idx, 1ULL, "6");
                } else if (cNumMillisecondPrecisionSubsecondDigits == num_digits) {
                    cat_sequence_replacements.emplace_back(pattern_idx, 1ULL, "3");
                } else if ('0' != remaining_unparsed_content.at(num_digits - 1)) {
                    cat_sequence_replacements.emplace_back(pattern_idx, 1ULL, "T");
                } else {
                    return ErrorCode{ErrorCodeEnum::IncompatibleTimestampPattern};
                }

                timestamp_idx += num_digits;
                parsed_subsecond_nanoseconds
                        = static_cast<int>(number * cPowersOfTen.at(cMaxFieldLength - num_digits));
                break;
            }
            case 'P': {  // Unknown-precision epoch time.
                auto const [number, num_digits] = YSTDLIB_ERROR_HANDLING_TRYX(
                        convert_variable_length_string_prefix_to_number(
                                timestamp.substr(timestamp_idx)
                        )
                );
                auto const [factor, precision_specifier] = estimate_timestamp_precision(number);
                cat_sequence_replacements
                        .emplace_back(pattern_idx, 1ULL, std::string{precision_specifier});
                parsed_epoch_nanoseconds = factor * number;
                timestamp_idx += num_digits;
                break;
            }
            case 'O': {  // One of several literal characters.
                constexpr size_t cFieldLength{1};
                if (timestamp_idx + cFieldLength > timestamp.size()) {
                    return ErrorCode{ErrorCodeEnum::IncompatibleTimestampPattern};
                }
                auto const bracket_pattern{YSTDLIB_ERROR_HANDLING_TRYX(
                        extract_bracket_pattern(raw_pattern.substr(pattern_idx + 1ULL))
                )};

                auto const possible_chars{bracket_pattern.substr(1, bracket_pattern.size() - 2)};
                auto const char_to_match{timestamp.at(timestamp_idx)};
                if (possible_chars.cend() == std::ranges::find(possible_chars, char_to_match)) {
                    return ErrorCode{ErrorCodeEnum::IncompatibleTimestampPattern};
                }

                cat_sequence_replacements.emplace_back(
                        pattern_idx - 1,
                        2ULL + bracket_pattern.size(),
                        std::string{char_to_match}
                );
                pattern_idx += bracket_pattern.size();
                timestamp_idx += cFieldLength;
                break;
            }
            case 's': {  // Generic zero-padded second.
                constexpr size_t cFieldLength{2};
                if (timestamp_idx + cFieldLength > timestamp.size()) {
                    return ErrorCode{ErrorCodeEnum::IncompatibleTimestampPattern};
                }

                parsed_second = YSTDLIB_ERROR_HANDLING_TRYX(convert_padded_string_to_number(
                        timestamp.substr(timestamp_idx, cFieldLength),
                        '0'
                ));
                if (parsed_second < cMinParsedSecond || parsed_second > cLeapSecond) {
                    return ErrorCode{ErrorCodeEnum::IncompatibleTimestampPattern};
                }

                if (cLeapSecond == parsed_second) {
                    parsed_second = cMaxParsedSecond;
                    cat_sequence_replacements.emplace_back(pattern_idx, 1ULL, "J");
                } else {
                    cat_sequence_replacements.emplace_back(pattern_idx, 1ULL, "S");
                }

                timestamp_idx += cFieldLength;
                break;
            }
            case '\\': {
                auto const expected_backslash{is_json_literal ? cJsonEscapedBackslash : "\\"};
                if (false == timestamp.substr(timestamp_idx).starts_with(expected_backslash)) {
                    return ErrorCode{ErrorCodeEnum::IncompatibleTimestampPattern};
                }
                timestamp_idx += expected_backslash.size();
                break;
            }
            default:
                return ErrorCode{ErrorCodeEnum::InvalidEscapeSequence};
        }
    }

    // Do not allow trailing unmatched content.
    if (pattern_idx != pattern_end_idx || timestamp_idx != timestamp.size()) {
        return ErrorCode{ErrorCodeEnum::IncompatibleTimestampPattern};
    }

    if (false == cat_sequence_replacements.empty()) {
        generated_pattern.clear();
        size_t last_pattern_idx{};
        for (auto const& replacement : cat_sequence_replacements) {
            generated_pattern.append(
                    raw_pattern.substr(last_pattern_idx, replacement.start_idx - last_pattern_idx)
            );
            last_pattern_idx = replacement.start_idx + replacement.length;
            generated_pattern.append(replacement.replacement);
        }
        generated_pattern.append(raw_pattern.substr(last_pattern_idx));
    }
    std::string_view const returned_pattern{
            cat_sequence_replacements.empty() ? raw_pattern : std::string_view{generated_pattern}
    };

    if (false == pattern.uses_date_type_representation()) {
        epochtime_t epoch_nanoseconds{parsed_epoch_nanoseconds};
        if (epoch_nanoseconds < 0) {
            epoch_nanoseconds -= static_cast<epochtime_t>(parsed_subsecond_nanoseconds);
        } else {
            epoch_nanoseconds += static_cast<epochtime_t>(parsed_subsecond_nanoseconds);
        }
        return {epoch_nanoseconds, returned_pattern};
    }

    if (pattern.uses_twelve_hour_clock()) {
        parsed_hour = (parsed_hour % cMaxParsedHour12HourClock)
                      + (optional_part_of_day_idx.value_or(0) * cMaxParsedHour12HourClock);
    }

    auto const year_month_day{date::year(parsed_year) / parsed_month / parsed_day};
    if (false == year_month_day.ok()) {
        return ErrorCode{ErrorCodeEnum::InvalidDate};
    }

    auto const time_point = date::sys_days{year_month_day} + std::chrono::hours{parsed_hour}
                            + std::chrono::minutes{parsed_minute}
                            + std::chrono::seconds{parsed_second}
                            + std::chrono::nanoseconds{parsed_subsecond_nanoseconds}
                            - std::chrono::minutes{optional_timezone_offset_in_minutes.value_or(0)};

    if (optional_day_of_week_idx.has_value()) {
        auto const actual_day_of_week_idx{(date::year_month_weekday(date::sys_days(year_month_day))
                                                   .weekday_indexed()
                                                   .weekday()
                                           - date::Sunday)
                                                  .count()};
        if (actual_day_of_week_idx != optional_day_of_week_idx.value()) {
            return ErrorCode{ErrorCodeEnum::InvalidDate};
        }
    }

    epochtime_t const epoch_nanoseconds{
            std::chrono::duration_cast<std::chrono::nanoseconds>(time_point.time_since_epoch())
                    .count()
    };
    return {epoch_nanoseconds, returned_pattern};
}

// NOLINTEND(readability-function-cognitive-complexity)

auto marshal_timestamp(epochtime_t timestamp, TimestampPattern const& pattern, std::string& buffer)
        -> ystdlib::error_handling::Result<void> {
    if (pattern.uses_date_type_representation()) {
        YSTDLIB_ERROR_HANDLING_TRYV(marshal_date_time_timestamp(timestamp, pattern, buffer));
    } else {
        YSTDLIB_ERROR_HANDLING_TRYV(marshal_numeric_timestamp(timestamp, pattern, buffer));
    }
    return ystdlib::error_handling::success();
}

[[nodiscard]] auto search_known_timestamp_patterns(
        std::string_view timestamp,
        std::vector<TimestampPattern> const& patterns,
        bool is_json_literal,
        std::string& generated_pattern
) -> std::optional<std::pair<epochtime_t, std::string_view>> {
    for (auto const& pattern : patterns) {
        auto const result{parse_timestamp(timestamp, pattern, is_json_literal, generated_pattern)};
        if (false == result.has_error()) {
            return result.value();
        }
    }
    return std::nullopt;
}

auto get_default_date_time_timestamp_patterns()
        -> ystdlib::error_handling::Result<std::vector<TimestampPattern>> {
    std::vector<TimestampPattern> timestamp_patterns;
    timestamp_patterns.reserve(cDefaultDateTimePatterns.size());
    for (auto const pattern : cDefaultDateTimePatterns) {
        timestamp_patterns.emplace_back(
                YSTDLIB_ERROR_HANDLING_TRYX(TimestampPattern::create(pattern))
        );
    }
    return timestamp_patterns;
}

auto get_default_numeric_timestamp_patterns()
        -> ystdlib::error_handling::Result<std::vector<TimestampPattern>> {
    std::vector<TimestampPattern> timestamp_patterns;
    timestamp_patterns.reserve(cDefaultNumericPatterns.size());
    for (auto const pattern : cDefaultNumericPatterns) {
        timestamp_patterns.emplace_back(
                YSTDLIB_ERROR_HANDLING_TRYX(TimestampPattern::create(pattern))
        );
    }
    return timestamp_patterns;
}

auto get_all_default_timestamp_patterns()
        -> ystdlib::error_handling::Result<std::vector<TimestampPattern>> {
    std::vector<TimestampPattern> timestamp_patterns;
    auto const date_time_patterns{
            YSTDLIB_ERROR_HANDLING_TRYX(get_default_date_time_timestamp_patterns())
    };
    auto const numeric_patterns{
            YSTDLIB_ERROR_HANDLING_TRYX(get_default_numeric_timestamp_patterns())
    };
    timestamp_patterns.reserve(date_time_patterns.size() + numeric_patterns.size());
    timestamp_patterns.insert(
            timestamp_patterns.cend(),
            date_time_patterns.cbegin(),
            date_time_patterns.cend()
    );
    timestamp_patterns
            .insert(timestamp_patterns.cend(), numeric_patterns.cbegin(), numeric_patterns.cend());
    return timestamp_patterns;
}

auto get_all_default_quoted_timestamp_patterns()
        -> ystdlib::error_handling::Result<std::vector<TimestampPattern>> {
    std::vector<TimestampPattern> timestamp_patterns;
    for (auto const& pattern : YSTDLIB_ERROR_HANDLING_TRYX(get_all_default_timestamp_patterns())) {
        timestamp_patterns.emplace_back(YSTDLIB_ERROR_HANDLING_TRYX(
                TimestampPattern::create(fmt::format(R"("{}")", pattern.get_pattern()))
        ));
    }
    return timestamp_patterns;
}
}  // namespace clp_s::timestamp_parser
