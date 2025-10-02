#include "TimestampParser.hpp"

#include <array>
#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <date/date.h>
#include <string_utils/string_utils.hpp>
#include <ystdlib/error_handling/Result.hpp>

#include "TimestampParserErrorCode.hpp"

namespace clp_s {
namespace {
constexpr std::array < std::string_view >> cAbbreviatedDaysOfWeek
        = {std::string_view{"Sun"},
           std::string_view{"Mon"},
           std::string_view{"Tue"},
           std::string_view{"Wed"},
           std::string_view{"Thu"},
           std::string_view{"Fri"},
           std::string_view{"Sat"}};

constexpr std::array < std::string_view >> cMonthNames
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

constexpr std::array < std::string_view >> cAbbreviatedMonthNames
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
 * @param str Substring containg the padded decimal integer string.
 * @param padding_character Padding character which may prefix the integer.
 * @param value The integer value, returned by reference.
 * @return Whether conversion was successful.
 */
auto convert_padded_string_to_number(std::string_view str, char padding_character, int& value);

/**
 * Finds the first matching prefix from a list of candidates.
 * @param str Substring with a prefix potentially matching one of the candidates.
 * @param candidates Candidate prefixes that will be matched against `str`.
 * @return The index of the matching prefix in the candidates array, or
 * `TimestampPatternErrorCodeEnum::IncompatibleTimestampPattern` on error.
 */
auto
find_first_matching_prefix(std::string_view str, std::array < std::string_view >> const& candidates)
        -> ystdlib::error_handling::Result<size_t, TimestampParserErrorCode>;

auto convert_padded_string_to_number(std::string_view str, char padding_character, int& value) {
    size_t i{};
    for (; i < str.size() && padding_character == str[i]; ++i) {}
    if (0ULL == (str.size() - i)) {
        return false;
    }
    return clp::string_utils::convert_string_to_int(str.substr(i), value);
}

auto
find_first_matching_prefix(std::string_view str, std::array < std::string_view >> const& candidates)
        -> ystdlib::error_handling::Result<size_t, TimestampParserErrorCode> {
    for (size_t candidate_idx{0ULL}; candidate_idx < candidates.size(); ++candidate_idx) {
        auto const& candidate{candidates[candidate_idx]};
        if (candidate == str.substr(0ULL, candidate.size())) {
            return candidate_idx;
        }
    }
    return TimestampParserErrorCodeEnum::IncompatibleTimestampPattern;
}
}  // namespace

auto TimestampParser::parse_timestamp(
        std::string_view timestamp,
        std::string_view pattern,
        std::string& generated_pattern
) -> ystdlib::error_handling::
        Result<std::pair<epochtime_t, std::string_view>, TimestampParserErrorCode> {
    std::vector<std::pair<size_t, std::string>> cat_sequence_resolutions;

    size_t pattern_idx{};
    size_t timestamp_idx{};

    int parsed_year{1970};
    int parsed_month{1};
    int parsed_day{1};

    bool date_type_representation{false};
    bool number_type_representation{false};

    bool escaped{false};
    for (; pattern_idx < pattern.size() && timestamp_idx < timestamp.size(); ++pattern_idx) {
        if (false == escaped && '\\' == pattern[pattern_idx]) {
            escaped = true;
            continue;
        }
        if (false == escaped) {
            if (pattern[pattern_idx] == timestamp[timestamp_idx]) {
                ++timestamp_idx;
                continue;
            }
            return TimestampParserErrorCodeEnum::IncompatibleTimestampPattern;
        }

        switch (pattern[pattern_idx]) {
            case 'y': {  // Zero-padded 2-digit year in century.
                constexpr size_t cFieldLength = 2;
                if (timestamp_idx + cFieldLength > timestamp.size()) {
                    return TimestampParserErrorCodeEnum::IncompatibleTimestampPattern;
                }

                int two_digit_year{};
                if (false
                    == convert_padded_string_to_number(
                            timestamp.substr(timestamp_idx, cFieldLength),
                            '0',
                            two_digit_year
                    ))
                {
                    return TimestampParserErrorCodeEnum::IncompatibleTimestampPattern;
                }
                if (two_digit_year < 0 || two_digit_year > 99) {
                    return TimestampParserErrorCodeEnum::IncompatibleTimestampPattern;
                }

                if (two_digit_year >= 69) {
                    parsed_year = two_digit_year + 1900;
                } else {
                    parsed_year = two_digit_year + 2000;
                }
                timestamp_idx += cFieldLength;
                date_type_representation = true;
                break;
            }
            case 'Y': {  // Zero-padded 4-digit year.
                constexpr size_t cFieldLength = 4;
                if (timestamp_idx + cFieldLength > timestamp.size()) {
                    return TimestampParserErrorCodeEnum::IncompatibleTimestampPattern;
                }

                if (false
                    == convert_padded_string_to_number(
                            timestamp.substr(timestamp_idx, cFieldLength),
                            '0',
                            parsed_year
                    ))
                {
                    return TimestampParserErrorCodeEnum::IncompatibleTimestampPattern;
                }
                if (parsed_year < 0 || parsed_year > 9999) {
                    return TimestampParserErrorCodeEnum::IncompatibleTimestampPattern;
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
                timestamp_idx += cMonthNames[month_idx].length();
                date_type_representation = true;
                break;
            }
            case 'b': {  // Abbreviated month name.
                auto const month_idx{YSTDLIB_ERROR_HANDLING_TRYX(find_first_matching_prefix(
                        timestamp.substr(timestamp_idx),
                        cAbbreviatedMonthNames
                ))};
                parsed_month = static_cast<int>(month_idx) + 1;
                timestamp_idx += cAbbreviatedMonthNames[month_idx].length();
                date_type_representation = true;
                break;
            }
            case 'm': {  // Zero-padded month.
                constexpr size_t cFieldLength = 2;
                if (timestamp_idx + cFieldLength > timestamp.size()) {
                    return TimestampParserErrorCodeEnum::IncompatibleTimestampPattern;
                }

                if (false
                    == convert_padded_string_to_number(
                            timestamp.substr(timestamp_idx, cFieldLength),
                            '0',
                            parsed_month
                    ))
                {
                    return TimestampParserErrorCodeEnum::IncompatibleTimestampPattern;
                }
                if (parsed_month < 1 || parsed_month > 12) {
                    return TimestampParserErrorCodeEnum::IncompatibleTimestampPattern;
                }

                timestamp_idx += cFieldLength;
                date_type_representation = true;
                break;
            }
            case 'd': {  // Zero-padded day in month.
                constexpr size_t cFieldLength = 2;
                if (timestamp_idx + cFieldLength > timestamp.size()) {
                    return TimestampParserErrorCodeEnum::IncompatibleTimestampPattern;
                }

                if (false
                    == convert_padded_string_to_number(
                            timestamp.substr(timestamp_idx, cFieldLength),
                            '0',
                            parsed_day
                    ))
                {
                    return TimestampParserErrorCodeEnum::IncompatibleTimestampPattern;
                }
                if (parsed_day < 1 || parsed_day > 31) {
                    return TimestampParserErrorCodeEnum::IncompatibleTimestampPattern;
                }

                timestamp_idx += cFieldLength;
                date_type_representation = true;
                break;
            }
            case 'e': {  // Space-padded day in month.
                constexpr size_t cFieldLength = 2;
                if (timestamp_idx + cFieldLength > timestamp.size()) {
                    return TimestampParserErrorCodeEnum::IncompatibleTimestampPattern;
                }

                if (false
                    == convert_padded_string_to_number(
                            timestamp.substr(timestamp_idx, cFieldLength),
                            ' ',
                            parsed_day
                    ))
                {
                    return TimestampParserErrorCodeEnum::IncompatibleTimestampPattern;
                }
                if (parsed_day < 1 || parsed_day > 31) {
                    return TimestampParserErrorCodeEnum::IncompatibleTimestampPattern;
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
                timestamp_idx += cAbbreviatedDaysOfWeek[day_idx].length();
                // Weekday is not necessary for determining absolute timestamp, so we discard it.
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
                return TimestampParserErrorCodeEnum::FormatSpecifierNotImplemented;
            case '\\': {
                if ('\\' == timestamp[timestamp_idx]) {
                    ++timestamp_idx;
                    continue;
                }
                return TimestampParserErrorCodeEnum::IncompatibleTimestampPattern;
            }
            default:
                return TimestampParserErrorCodeEnum::InvalidTimestampPattern;
        }
    }

    // Do not allow trailing unmatched content.
    if (false == (pattern_idx == pattern.size() && timestamp_idx == timestamp.size())) {
        return TimestampParserErrorCodeEnum::IncompatibleTimestampPattern;
    }

    // Do not allow mixing format specifiers for date-and-time type timestamps and epoch-number type
    // timestamps.
    if (date_type_representation && number_type_representation) {
        return TimestampParserErrorCodeEnum::InvalidTimestampPattern;
    }

    if (number_type_representation) {
        return TimestampParserErrorCodeEnum::FormatSpecifierNotImplemented;
    }

    auto year_month_day{date::year(parsed_year) / parsed_month / parsed_day};
    if (false == year_month_day.ok()) {
        return TimestampParserErrorCodeEnum::InvalidDate;
    }

    auto time_point = date::sys_days(year_month_day) + std::chrono::hours(0)
                      + std::chrono::minutes(0) + std::chrono::seconds(0)
                      + std::chrono::nanoseconds(0);

    epochtime_t epoch_nanoseconds{
            std::chrono::duration_cast<std::chrono::nanoseconds>(time_point.time_since_epoch())
                    .count()
    };
    return {epoch_nanoseconds, pattern};
}
}  // namespace clp_s
