#include "TimestampParser.hpp"

#include <array>
#include <chrono>
#include <cstddef>
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
    std::optional<int> optional_day_of_week_idx;

    bool date_type_representation{false};
    bool const number_type_representation{false};

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
            case 'p':
            case 'H':
            case 'k':
            case 'I':
            case 'l':
            case 'M':
            case 'S':
            case '3':
            case '6':
            case '9':
            case 'T':
            case 'E':
            case 'L':
            case 'C':
            case 'N':
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

    // Do not allow trailing unmatched content.
    if (pattern_idx != pattern.size() || timestamp_idx != timestamp.size()) {
        return ErrorCode{ErrorCodeEnum::IncompatibleTimestampPattern};
    }

    if (number_type_representation) {
        return ErrorCode{ErrorCodeEnum::FormatSpecifierNotImplemented};
    }

    auto const year_month_day{date::year(parsed_year) / parsed_month / parsed_day};
    if (false == year_month_day.ok()) {
        return ErrorCode{ErrorCodeEnum::InvalidDate};
    }

    auto const time_point = date::sys_days(year_month_day) + std::chrono::hours(0)
                            + std::chrono::minutes(0) + std::chrono::seconds(0)
                            + std::chrono::nanoseconds(0);

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
