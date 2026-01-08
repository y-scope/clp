#ifndef CLP_S_TIMESTAMP_PARSER_TIMESTAMPPARSER_HPP
#define CLP_S_TIMESTAMP_PARSER_TIMESTAMPPARSER_HPP

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <ystdlib/error_handling/Result.hpp>

#include "../Defs.hpp"

namespace clp_s::timestamp_parser {
/**
 * A class representing a validated timestamp pattern.
 */
class TimestampPattern {
public:
    // Factory functions
    /**
     * @param pattern
     * @return A result containing a `TimestampPattern`, or an error code indicating the failure:
     * - ErrorCodeEnum::InvalidTimestampPattern if `pattern` is not a valid timestamp pattern.
     * - ErrorCodeEnum::InvalidTimezone if `pattern` contains a \z{} format specifier with an
     *   invalid timezone.
     * - ErrorCodeEnum::InvalidEscapeSequence if `pattern` contains an unsupported escape sequence.
     * - ErrorCodeEnum::InvalidCharacter if `pattern` contains an unsupported character.
     */
    [[nodiscard]] static auto create(std::string_view pattern)
            -> ystdlib::error_handling::Result<TimestampPattern>;

    // Default copy & move constructors and assignment operators
    TimestampPattern(TimestampPattern const&) = default;
    auto operator=(TimestampPattern const&) -> TimestampPattern& = default;
    TimestampPattern(TimestampPattern&&) noexcept = default;
    auto operator=(TimestampPattern&&) noexcept -> TimestampPattern& = default;

    // Destructor
    ~TimestampPattern() = default;

    // Methods
    [[nodiscard]] auto get_pattern() const -> std::string_view { return m_pattern; }

    [[nodiscard]] auto get_optional_timezone_size_and_offset() const
            -> std::optional<std::pair<size_t, int>> const& {
        return m_optional_timezone_size_and_offset;
    }

    [[nodiscard]] auto uses_date_type_representation() const -> bool {
        return m_uses_date_type_representation;
    }

    [[nodiscard]] auto uses_twelve_hour_clock() const -> bool { return m_uses_twelve_hour_clock; }

    [[nodiscard]] auto is_quoted_pattern() const -> bool { return m_is_quoted_pattern; }

    /**
     * Finds the first matching month as a prefix of `timestamp`.
     * @param timestamp
     * @param pattern_idx An index into the raw pattern starting at the "B" in a `\B{}` format
     * specifier. On success, `pattern_idx` is advanced to the closing bracket in the raw pattern.
     * @return A result containing the index and length of the first matching month, or an error
     * code indicating the failure:
     * - Forwards `find_first_matching_prefix`'s return values on failure.
     */
    [[nodiscard]] auto find_first_matching_month_and_advance_pattern_idx(
            std::string_view timestamp,
            size_t& pattern_idx
    ) const -> ystdlib::error_handling::Result<std::pair<size_t, size_t>>;

    /**
     * Finds the first matching weekday as a prefix of `timestamp`.
     * @param timestamp
     * @param pattern_idx An index into the raw pattern starting at the "A" in a `\A{}` format
     * specifier. On success, `pattern_idx` is advanced to the closing bracket in the raw pattern.
     * @return A result containing the index and length of the first matching weekday, or an error
     * code indicating the failure:
     * - Forwards `find_first_matching_prefix`'s return values on failure.
     */
    [[nodiscard]] auto find_first_matching_weekday_and_advance_pattern_idx(
            std::string_view timestamp,
            size_t& pattern_idx
    ) const -> ystdlib::error_handling::Result<std::pair<size_t, size_t>>;

    /**
     * Gets the month at `month_idx` in a month name format specifier, and advances `pattern_idx` to
     * the closing bracket of the month name format specifier if found.
     * @param month_idx An index into the months in a `\B{}` format specifier.
     * @param pattern_idx An index into the raw pattern starting at the "B" in a `\B{}` format
     * specifier. On success, `pattern_idx` is advanced to the closing bracket in the raw pattern.
     * @return A result containing the month, or an error code indicating the failure:
     * - ErrorCodeEnum::IncompatibleTimestampPattern if `month_idx` is out of bounds.
     */
    [[nodiscard]] auto
    get_month_and_advance_pattern_idx(size_t month_idx, size_t& pattern_idx) const
            -> ystdlib::error_handling::Result<std::string_view>;

    /**
     * Gets the weekday at `weekday_idx` in a weekday name format specifier, and advances
     * `pattern_idx` to the closing bracket of the weekday name format specifier if found.
     * @param weekday_idx An index into the weekdays in a `\A{}` format specifier.
     * @param pattern_idx An index into the raw pattern starting at the "A" in a `\A{}` format
     * specifier. On success, `pattern_idx` is advanced to the closing bracket in the raw pattern.
     * @return A result containing the weekday, or an error code indicating the failure:
     * - ErrorCodeEnum::IncompatibleTimestampPattern if `weekday_idx` is out of bounds.
     */
    [[nodiscard]] auto
    get_weekday_and_advance_pattern_idx(size_t weekday_idx, size_t& pattern_idx) const
            -> ystdlib::error_handling::Result<std::string_view>;

private:
    // Constructor
    TimestampPattern(
            std::string_view pattern,
            std::optional<std::pair<size_t, int>> optional_timezone_size_and_offset,
            std::vector<std::pair<uint16_t, uint16_t>> month_name_offsets_and_lengths,
            std::vector<std::pair<uint16_t, uint16_t>> weekday_name_offsets_and_lengths,
            uint16_t month_name_bracket_pattern_length,
            uint16_t weekday_name_bracket_pattern_length,
            bool uses_date_type_representation,
            bool uses_twelve_hour_clock,
            bool is_quoted_pattern
    )
            : m_pattern{pattern},
              m_optional_timezone_size_and_offset{std::move(optional_timezone_size_and_offset)},
              m_month_name_offsets_and_lengths{std::move(month_name_offsets_and_lengths)},
              m_weekday_name_offsets_and_lengths{std::move(weekday_name_offsets_and_lengths)},
              m_month_name_bracket_pattern_length{month_name_bracket_pattern_length},
              m_weekday_name_bracket_pattern_length{weekday_name_bracket_pattern_length},
              m_uses_date_type_representation{uses_date_type_representation},
              m_uses_twelve_hour_clock{uses_twelve_hour_clock},
              m_is_quoted_pattern{is_quoted_pattern} {}

    // Variables
    std::string m_pattern;
    std::optional<std::pair<size_t, int>> m_optional_timezone_size_and_offset;
    std::vector<std::pair<uint16_t, uint16_t>> m_month_name_offsets_and_lengths;
    std::vector<std::pair<uint16_t, uint16_t>> m_weekday_name_offsets_and_lengths;
    uint16_t m_month_name_bracket_pattern_length{};
    uint16_t m_weekday_name_bracket_pattern_length{};
    bool m_uses_date_type_representation{false};
    bool m_uses_twelve_hour_clock{false};
    bool m_is_quoted_pattern{false};
};

/**
 * Parses a timestamp, as described by a timestamp pattern.
 *
 * Timestamp patterns are composed of literal characters, as well as escape sequences which belong
 * to two classes:
 *
 * 1. Regular format specifiers which capture a well-defined feature in a timestamp (e.g., 3-digit
 *    milliseconds, 6-digit microseconds, etc.).
 * 2. Capture And Transform (CAT) sequences, which attempt to capture any of a class of features
 *    (e.g., sub-second component of epoch timestamp) and produce a more specific timestamp pattern
 *    depending on the actual content of the timestamp (e.g., see ".123", and determine that the
 *    sub-second component of the timestamp is 3-digit milliseconds). After parsing, all CAT
 *    sequences are resolved to regular format specifiers.
 *
 * The general motivation for CAT sequences is to ease maintenance burden for specifying timestamp
 * patterns. By allowing users to capture classes of features, they can avoid manually specifying
 * every possible combination of features that can appear in several similar classes of timestamp.
 *
 * We support the following format specifiers:
 *
 * - \y Zero-padded year in century (69-99 -> 1969-1999, 00-68 -> 2000-2068).
 * - \Y Zero-padded year (0000-9999).
 * - \B{January,...,December} Month in a year. The names inside `{}` must be provided in order
 *   starting from January (indexed at 0) and include exactly twelve entries (January through
 *   December) splitting by `,`.
 * - \m Zero-padded month (01-12).
 * - \d Zero-padded day in month (01-31).
 * - \e Space-padded day in month( 1-31).
 * - \A{Sunday,...,Saturday} Day in a week. The names inside `{}` must be provided in order starting
 *   from Sunday (indexed at 0) and include exactly seven entries (Sunday through Saturday)
 *   splitting by `,`.
 * - \p Part of day (AM/PM).
 * - \H 24-hour clock, zero-padded hour (00-23).
 * - \k 24-hour clock, space-padded hour ( 0-23).
 * - \I 12-hour clock, zero-padded hour (01-12).
 * - \l 12-hour clock, space-padded hour ( 1-12).
 * - \M Zero-padded minute (00-59).
 * - \S Zero-padded non-leap second (00-59).
 * - \J Leap second (60).
 * - \3 Zero-padded millisecond (000-999).
 * - \6 Zero-padded microsecond (000000-999999).
 * - \9 Zero-padded nanosecond (000000000-999999999).
 * - \T Zero-padded fractional second, up to nanoseconds, without trailing zeroes.
 * - \E Epoch seconds.
 * - \L Epoch miLliseconds.
 * - \C Epoch miCroseconds.
 * - \N Epoch Nanoseconds.
 * - \z{...} Specific timezone, described by content between {}.
 * - \\ Literal backslash.
 *
 * We also support the following CAT sequences:
 *
 * - \Z Generic timezone -- resolves to literal content, and potentially \z{...}.
 * - \? Generic fractional second -- resolves to \3, \6, \9, or \T.
 * - \P Unknown-precision epoch time -- resolves to \E, \L, \C, or \N based on a heuristic.
 * - \O{...} One of several literal characters, described by content between {}.
 * - \s Generic zero-padded second (00-60) -- resolves to \S or \J.
 *
 * As well as the following escape sequences:
 *
 * - \\ Literal backslash.
 *
 * Any escape sequence not listed above is invalid. In addition, patterns must not contain:
 *
 * - ASCII control characters (U+0000 through U+001F).
 * - The double-quote character `"` (except for the surrounding quotes if the pattern is provided as
 *   a JSON string literal, e.g., `"<pattern>"`).
 *
 * @param timestamp
 * @param pattern A timestamp pattern made up of literals, format specifiers, and potentially CAT
 * sequences.
 * @param is_json_literal Whether the timestamp is a JSON literal or a raw UTF-8 string.
 * @param generated_pattern A buffer where a newly-generated timestamp pattern can be written, if
 * necessary.
 * @return A result containing a pair, or an error code indicating the failure:
 * - The pair:
 *   - The timestamp in epoch nanoseconds.
 *   - An `std::string_view` of the timestamp pattern that corresponds to the timestamp.
 *     - The lifetime of the `std::string_view` is the least of `pattern` and `generated_pattern`.
 * - The possible error codes:
 *   - ErrorCodeEnum::InvalidTimestampPattern if the pattern is illegal, per the specification.
 *   - ErrorCodeEnum::IncompatibleTimestampPattern if the pattern is not able to exactly consume the
 *     timestamp.
 *   - ErrorCodeEnum::InvalidDate if parsing was successful, but some components of the timestamp
 *     offer conflicting information about the actual date (e.g., if the parsed day of the week
 *     doesn't match up with the rest of the timestamp information).
 *   - ErrorCodeEnum::InvalidEscapeSequence if the pattern contains an unsupported escape sequence.
 */
[[nodiscard]] auto parse_timestamp(
        std::string_view timestamp,
        TimestampPattern const& pattern,
        bool is_json_literal,
        std::string& generated_pattern
) -> ystdlib::error_handling::Result<std::pair<epochtime_t, std::string_view>>;

/**
 * Marshals a timestamp as a JSON literal according to a timestamp pattern.
 * @param timestamp
 * @param pattern
 * @param buffer The buffer that the marshalled timestamp is appended to.
 * @return A void result on success, or an error code indicating the failure:
 * - Forwards `marshal_date_time_timestamp`'s return values.
 * - Forwards `marshal_numeric_timestamp`'s return values.
 */
[[nodiscard]] auto
marshal_timestamp(epochtime_t timestamp, TimestampPattern const& pattern, std::string& buffer)
        -> ystdlib::error_handling::Result<void>;

/**
 * Parses a timestamp according to the first matching pattern in a list of patterns.
 * @param timestamp
 * @param patterns A list of timestamp patterns.
 * @param is_json_literal Whether the timestamp is a JSON literal or a raw UTF-8 string.
 * @param generated_pattern A buffer where a newly-generated timestamp pattern can be written, if
 * necessary.
 * @return A pair containing:
 *   - The timestamp in epoch nanoseconds.
 *   - An `std::string_view` of the timestamp pattern that corresponds to the timestamp.
 *     - The lifetime of the `std::string_view` is the least of `patterns` and `generated_pattern`.
 * @return std::nullopt if no pattern can be used to parse the timestamp.
 */
[[nodiscard]] auto search_known_timestamp_patterns(
        std::string_view timestamp,
        std::vector<TimestampPattern> const& patterns,
        bool is_json_literal,
        std::string& generated_pattern
) -> std::optional<std::pair<epochtime_t, std::string_view>>;

/**
 * @return A result containing a vector of date-time timestamp patterns, or an error code indicating
 * the failure:
 * - Forwards `TimestampPattern::create`'s return values on failure.
 */
[[nodiscard]] auto get_default_date_time_timestamp_patterns()
        -> ystdlib::error_handling::Result<std::vector<TimestampPattern>>;

/**
 * @return A result containing a vector of numeric timestamp patterns, or an error code indicating
 * the failure:
 * - Forwards `TimestampPattern::create`'s return values on failure.
 */
[[nodiscard]] auto get_default_numeric_timestamp_patterns()
        -> ystdlib::error_handling::Result<std::vector<TimestampPattern>>;

/**
 * @return A result containing a vector of numeric and date-time timestamp patterns, or an error
 * code indicating the failure:
 * - Forwards `get_default_date_time_timestamp_patterns`'s return values on failure.
 * - Forwards `get_default_numeric_timestamp_patterns`'s return values on failure.
 */
[[nodiscard]] auto get_all_default_timestamp_patterns()
        -> ystdlib::error_handling::Result<std::vector<TimestampPattern>>;

/**
 * @return A result containing a vector of quoted numeric and date-time timestamp patterns, or an
 * error code indicating the failure:
 * - Forwards `get_all_default_timestamp_patterns`'s return values on failure.
 * - Forwards `TimestampPattern::create`'s return values on failure.
 */
[[nodiscard]] auto get_all_default_quoted_timestamp_patterns()
        -> ystdlib::error_handling::Result<std::vector<TimestampPattern>>;
}  // namespace clp_s::timestamp_parser

#endif  // CLP_S_TIMESTAMP_PARSER_TIMESTAMPPARSER_HPP
