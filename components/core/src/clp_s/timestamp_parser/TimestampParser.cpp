#include "TimestampParser.hpp"

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>

#include <date/date.h>
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
constexpr int cMinTwoDigitYear{0};
constexpr int cTwoDigitYearOffsetBoundary{69};
constexpr int cMaxTwoDigitYear{99};
constexpr int cTwoDigitYearLowOffset{1900};
constexpr int cTwoDigitYearHighOffset{2000};
constexpr int cMinParsedHour24HourClock{0};
constexpr int cMaxParsedHour24HourClock{23};
constexpr int cMinParsedHour12HourClock{1};
constexpr int cMaxParsedHour12HourClock{12};
constexpr int cMinParsedMinute{0};
constexpr int cMaxParsedMinute{59};
constexpr int cMinParsedSecond{0};
constexpr int cMaxParsedSecond{60};
constexpr int cMinParsedSubsecondNanoseconds{0};

constexpr size_t cNumNanosecondPrecisionSubsecondDigits{9ULL};
constexpr size_t cNumMicrosecondPrecisionSubsecondDigits{6ULL};
constexpr size_t cNumMillisecondPrecisionSubsecondDigits{3ULL};
constexpr size_t cNumSecondPrecisionSubsecondDigits{0ULL};

constexpr int cDefaultYear{1970};
constexpr int cDefaultMonth{1};
constexpr int cDefaultDay{1};

constexpr std::array cAbbreviatedDaysOfWeek
        = {std::string_view{"Sun"},
           std::string_view{"Mon"},
           std::string_view{"Tue"},
           std::string_view{"Wed"},
           std::string_view{"Thu"},
           std::string_view{"Fri"},
           std::string_view{"Sat"}};

constexpr std::array cMonthNames
        = {std::string_view{"January"},
           std::string_view{"February"},
           std::string_view{"March"},
           std::string_view{"April"},
           std::string_view{"May"},
           std::string_view{"June"},
           std::string_view{"July"},
           std::string_view{"August"},
           std::string_view{"September"},
           std::string_view{"October"},
           std::string_view{"November"},
           std::string_view{"December"}};

constexpr std::array cAbbreviatedMonthNames
        = {std::string_view{"Jan"},
           std::string_view{"Feb"},
           std::string_view{"Mar"},
           std::string_view{"Apr"},
           std::string_view{"May"},
           std::string_view{"Jun"},
           std::string_view{"Jul"},
           std::string_view{"Aug"},
           std::string_view{"Sep"},
           std::string_view{"Oct"},
           std::string_view{"Nov"},
           std::string_view{"Dec"}};

constexpr std::array cPartsOfDay = {std::string_view{"AM"}, std::string_view{"PM"}};

constexpr std::array cPowersOfTen
        = {1, 10, 100, 1000, 10'000, 100'000, 1'000'000, 10'000'000, 100'000'000, 1'000'000'000};

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
) -> ystdlib::error_handling::Result<std::pair<int, size_t>>;

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

auto convert_padded_string_to_number(std::string_view str, char padding_character)
        -> ystdlib::error_handling::Result<int> {
    if (str.empty()) {
        return ErrorCode{ErrorCodeEnum::IncompatibleTimestampPattern};
    }

    // Leave at least one character for parsing to ensure we actually parse number content.
    size_t i{};
    for (; i < (str.size() - 1) && padding_character == str.at(i); ++i) {}

    int value{};
    if (clp::string_utils::convert_string_to_int(str.substr(i), value)) {
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

auto convert_positive_bounded_variable_length_string_prefix_to_number(
        std::string_view str,
        size_t max_num_digits
) -> ystdlib::error_handling::Result<std::pair<int, size_t>> {
    constexpr int cTen{10};
    if (0ULL == max_num_digits) {
        return ErrorCode{ErrorCodeEnum::InvalidTimestampPattern};
    }
    if (str.empty() || '-' == str.at(0ULL)
        || false == clp::string_utils::is_decimal_digit(str.at(0ULL)))
    {
        return ErrorCode{ErrorCodeEnum::IncompatibleTimestampPattern};
    }

    int converted_value{};
    size_t num_decimal_digits{};
    while (true) {
        char const cur_digit{str.at(num_decimal_digits)};
        converted_value += static_cast<int>(cur_digit - '0');
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

    if (is_negative) {
        converted_value *= -1;
    }
    return std::make_pair(converted_value, num_decimal_digits);
}
}  // namespace

// NOLINTBEGIN(readability-function-cognitive-complexity)
auto parse_timestamp(
        std::string_view timestamp,
        std::string_view pattern,
        [[maybe_unused]] std::string& generated_pattern
) -> ystdlib::error_handling::Result<std::pair<epochtime_t, std::string_view>> {
    size_t pattern_idx{};
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

    int64_t parsed_epoch_nanoseconds{};

    bool date_type_representation{false};
    bool number_type_representation{false};
    bool uses_12_hour_clock{false};

    bool escaped{false};
    for (; pattern_idx < pattern.size() && timestamp_idx < timestamp.size(); ++pattern_idx) {
        if (false == escaped) {
            if ('\\' == pattern[pattern_idx]) {
                escaped = true;
                continue;
            }
            if (pattern[pattern_idx] == timestamp[timestamp_idx]) {
                ++timestamp_idx;
                continue;
            }
            return ErrorCode{ErrorCodeEnum::IncompatibleTimestampPattern};
        }

        escaped = false;
        switch (pattern[pattern_idx]) {
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
                date_type_representation = true;
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
                date_type_representation = true;
                break;
            }
            case 'B': {  // Full month name.
                auto const month_idx{YSTDLIB_ERROR_HANDLING_TRYX(
                        find_first_matching_prefix(timestamp.substr(timestamp_idx), cMonthNames)
                )};
                parsed_month = static_cast<int>(month_idx) + 1;
                timestamp_idx += cMonthNames.at(month_idx).length();
                date_type_representation = true;
                break;
            }
            case 'b': {  // Abbreviated month name.
                auto const month_idx{YSTDLIB_ERROR_HANDLING_TRYX(find_first_matching_prefix(
                        timestamp.substr(timestamp_idx),
                        cAbbreviatedMonthNames
                ))};
                parsed_month = static_cast<int>(month_idx) + 1;
                timestamp_idx += cAbbreviatedMonthNames.at(month_idx).length();
                date_type_representation = true;
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
                date_type_representation = true;
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
                date_type_representation = true;
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
                date_type_representation = true;
                break;
            }
            case 'a': {  // Abbreviated day in week.
                auto const day_idx{YSTDLIB_ERROR_HANDLING_TRYX(find_first_matching_prefix(
                        timestamp.substr(timestamp_idx),
                        cAbbreviatedDaysOfWeek
                ))};
                timestamp_idx += cAbbreviatedDaysOfWeek.at(day_idx).length();
                optional_day_of_week_idx = static_cast<int>(day_idx);
                date_type_representation = true;
                break;
            }
            case 'p': {
                auto const part_of_day_idx{YSTDLIB_ERROR_HANDLING_TRYX(
                        find_first_matching_prefix(timestamp.substr(timestamp_idx), cPartsOfDay)
                )};
                timestamp_idx += cPartsOfDay.at(part_of_day_idx).length();
                optional_part_of_day_idx = static_cast<int>(part_of_day_idx);
                date_type_representation = true;
                break;
            }
            case 'H': {
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
                date_type_representation = true;
                break;
            }
            case 'k': {
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
                date_type_representation = true;
                break;
            }
            case 'I': {
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
                uses_12_hour_clock = true;
                date_type_representation = true;
                break;
            }
            case 'l': {
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
                uses_12_hour_clock = true;
                date_type_representation = true;
                break;
            }
            case 'M': {
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
                date_type_representation = true;
                break;
            }
            case 'S': {
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
                date_type_representation = true;
                break;
            }
            case '3': {
                constexpr size_t cFieldLength{3};
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
            case '6': {
                constexpr size_t cFieldLength{6};
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
            case '9': {
                constexpr size_t cFieldLength{9};
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
            case 'T': {
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
                        = number * cPowersOfTen.at(cMaxFieldLength - num_digits);
                break;
            }
            case 'E': {
                auto const [number, num_digits] = YSTDLIB_ERROR_HANDLING_TRYX(
                        convert_variable_length_string_prefix_to_number(
                                timestamp.substr(timestamp_idx)
                        )
                );
                timestamp_idx += num_digits;
                parsed_epoch_nanoseconds = number
                                           * cPowersOfTen.at(
                                                   cNumNanosecondPrecisionSubsecondDigits
                                                   - cNumSecondPrecisionSubsecondDigits
                                           );
                number_type_representation = true;
                break;
            }
            case 'L': {
                auto const [number, num_digits] = YSTDLIB_ERROR_HANDLING_TRYX(
                        convert_variable_length_string_prefix_to_number(
                                timestamp.substr(timestamp_idx)
                        )
                );
                timestamp_idx += num_digits;
                parsed_epoch_nanoseconds = number
                                           * cPowersOfTen.at(
                                                   cNumNanosecondPrecisionSubsecondDigits
                                                   - cNumMillisecondPrecisionSubsecondDigits
                                           );
                number_type_representation = true;
                break;
            }
            case 'C': {
                auto const [number, num_digits] = YSTDLIB_ERROR_HANDLING_TRYX(
                        convert_variable_length_string_prefix_to_number(
                                timestamp.substr(timestamp_idx)
                        )
                );
                timestamp_idx += num_digits;
                parsed_epoch_nanoseconds = number
                                           * cPowersOfTen.at(
                                                   cNumNanosecondPrecisionSubsecondDigits
                                                   - cNumMicrosecondPrecisionSubsecondDigits
                                           );
                number_type_representation = true;
                break;
            }
            case 'N': {
                auto const [number, num_digits] = YSTDLIB_ERROR_HANDLING_TRYX(
                        convert_variable_length_string_prefix_to_number(
                                timestamp.substr(timestamp_idx)
                        )
                );
                timestamp_idx += num_digits;
                parsed_epoch_nanoseconds = number;
                number_type_representation = true;
                break;
            }
            case 'z':
            case 'Z':
            case '?':
            case 'P':
                return ErrorCode{ErrorCodeEnum::FormatSpecifierNotImplemented};
            case '\\': {
                if ('\\' == timestamp[timestamp_idx]) {
                    ++timestamp_idx;
                    continue;
                }
                return ErrorCode{ErrorCodeEnum::IncompatibleTimestampPattern};
            }
            default:
                return ErrorCode{ErrorCodeEnum::InvalidTimestampPattern};
        }
    }

    if (escaped) {
        return ErrorCode{ErrorCodeEnum::InvalidTimestampPattern};
    }

    // Do not allow mixing format specifiers for date-and-time type timestamps and epoch-number type
    // timestamps.
    if (date_type_representation && number_type_representation) {
        return ErrorCode{ErrorCodeEnum::InvalidTimestampPattern};
    }

    if ((uses_12_hour_clock && false == optional_part_of_day_idx.has_value())
        || (false == uses_12_hour_clock && optional_part_of_day_idx.has_value()))
    {
        return ErrorCode{ErrorCodeEnum::InvalidTimestampPattern};
    }

    // Do not allow trailing unmatched content.
    if (pattern_idx != pattern.size() || timestamp_idx != timestamp.size()) {
        return ErrorCode{ErrorCodeEnum::IncompatibleTimestampPattern};
    }

    if (number_type_representation) {
        epochtime_t epoch_nanoseconds{parsed_epoch_nanoseconds};
        if (epoch_nanoseconds < 0) {
            epoch_nanoseconds -= static_cast<epochtime_t>(parsed_subsecond_nanoseconds);
        } else {
            epoch_nanoseconds += static_cast<epochtime_t>(parsed_subsecond_nanoseconds);
        }
        return {epoch_nanoseconds, pattern};
    }

    if (uses_12_hour_clock) {
        parsed_hour = (parsed_hour % cMaxParsedHour12HourClock)
                      + optional_part_of_day_idx.value() * cMaxParsedHour12HourClock;
    }

    auto const year_month_day{date::year(parsed_year) / parsed_month / parsed_day};
    if (false == year_month_day.ok()) {
        return ErrorCode{ErrorCodeEnum::InvalidDate};
    }

    auto const time_point = date::sys_days(year_month_day) + std::chrono::hours(parsed_hour)
                            + std::chrono::minutes(parsed_minute)
                            + std::chrono::seconds(parsed_second)
                            + std::chrono::nanoseconds(parsed_subsecond_nanoseconds);

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
    return {epoch_nanoseconds, pattern};
}

// NOLINTEND(readability-function-cognitive-complexity)
}  // namespace clp_s::timestamp_parser
