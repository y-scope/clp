#ifndef CLP_S_TIMESTAMP_PARSER_TIMESTAMPPARSER_HPP
#define CLP_S_TIMESTAMP_PARSER_TIMESTAMPPARSER_HPP

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

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

private:
    // Constructor
    TimestampPattern(
            std::string_view pattern,
            std::optional<std::pair<size_t, int>> optional_timezone_size_and_offset,
            bool uses_date_type_representation,
            bool uses_twelve_hour_clock
    )
            : m_pattern{pattern},
              m_optional_timezone_size_and_offset{std::move(optional_timezone_size_and_offset)},
              m_uses_date_type_representation{uses_date_type_representation},
              m_uses_twelve_hour_clock{uses_twelve_hour_clock} {}

    // Variables
    std::string m_pattern;
    std::optional<std::pair<size_t, int>> m_optional_timezone_size_and_offset;
    bool m_uses_date_type_representation{false};
    bool m_uses_twelve_hour_clock{false};
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
 * - \B Full month name (e.g., January).
 * - \b Abbreviated month name (e.g., Jan).
 * - \m Zero-padded month (01-12).
 * - \d Zero-padded day in month (01-31).
 * - \e Space-padded day in month( 1-31).
 * - \a Abbreviated day in week (e.g., Mon).
 * - \p Part of day (AM/PM).
 * - \H 24-hour clock, zero-padded hour (00-23).
 * - \k 24-hour clock, space-padded hour ( 0-23).
 * - \I 12-hour clock, zero-padded hour (01-12).
 * - \l 12-hour clock, space-padded hour ( 1-12).
 * - \M Zero-padded minute (00-59).
 * - \S Zero-padded second (00-60) (60 for leap seconds).
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
 *
 * @param timestamp
 * @param pattern A timestamp pattern made up of literals, format specifiers, and potentially CAT
 * sequences.
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
 *   - ErrorCodeEnum::FormatSpecifierNotImplemented if the pattern contains format specifiers that
 *     have not been implemented yet.
 */
[[nodiscard]] auto parse_timestamp(
        std::string_view timestamp,
        TimestampPattern const& pattern,
        std::string& generated_pattern
) -> ystdlib::error_handling::Result<std::pair<epochtime_t, std::string_view>>;
}  // namespace clp_s::timestamp_parser

#endif  // CLP_S_TIMESTAMP_PARSER_TIMESTAMPPARSER_HPP
