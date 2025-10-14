#ifndef CLP_S_TIMESTAMP_PARSER_TIMESTAMPPARSER_HPP
#define CLP_S_TIMESTAMP_PARSER_TIMESTAMPPARSER_HPP

#include <string>
#include <string_view>

#include <ystdlib/error_handling/Result.hpp>

#include "../Defs.hpp"
#include "ErrorCode.hpp"

namespace clp_s::timestamp_parser {
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
 *                sequences.
 * @param generated_pattern A buffer where a newly-generated timestamp pattern can be written, if
 *                          necessary.
 * @return A result containing a pair, or an error code indicating the failure:
 * - The pair:
 *   - The timestamp in epoch nanoseconds.
 *   - An `std::string_view` of the timestamp pattern that corresponds to the timestamp.
 *     - The lifetime of the `std::string_view` is the least of `pattern` and `generated_pattern`.
 * - The possible error codes from `clp_s::timestamp_parser::ErrorCodeEnum`:
 *   - `InvalidTimestampPattern` if the pattern is illegal, per the specification.
 *   - `IncompatibleTimestampPattern` if the pattern is not able to exactly consume the timestamp.
 *   - `InvalidDate` if parsing was successful, but some components of the timestamp offer
 *     conflicting information about the actual date (e.g., if the parsed day of the week doesn't
 *     match up with the rest of the timestamp information).
 *   - `FormatSpecifierNotImplemented` if the pattern contains format specifiers that have not been
 *     implemented yet.
 */
[[nodiscard]] auto parse_timestamp(
        std::string_view timestamp,
        std::string_view pattern,
        std::string& generated_pattern
) -> ystdlib::error_handling::Result<std::pair<epochtime_t, std::string_view>, ErrorCode>;
}  // namespace clp_s::timestamp_parser

#endif  // CLP_S_TIMESTAMP_PARSER_TIMESTAMPPARSER_HPP
