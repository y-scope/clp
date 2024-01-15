#ifndef GLT_TIMESTAMPPATTERN_HPP
#define GLT_TIMESTAMPPATTERN_HPP

#include <cstddef>
#include <cstdint>
#include <memory>

#include "Defs.h"
#include "FileWriter.hpp"
#include "TraceableException.hpp"

namespace glt {
/**
 * Class representing a timestamp pattern with methods for both parsing and formatting timestamps
 * using the pattern. A format string contains directives specifying how a string should be parsed
 * into a timestamp or how a timestamp should be formatted into a string. E.g., "[%H:%M:%S]" can
 * parse from or format to "[23:45:19]"
 *
 * The supported directives are the same as strptime except that we require an exact number of
 * spaces/padding digits so that we can reproduce the timestamp exactly. There are also additions
 * beyond what strptime provides.
 *
 * The following directives are supported:
 * - %  Literal %
 * - y  2-digit 0-padded year in century. [69,99] refers to years [1969,1999]. [00,68] refers to
 *      years [2000,2068].
 * - Y  4-digit 0-padded year including century (0000-9999)
 * - B  Full month name (e.g., "January")
 * - b  Abbreviated month name (e.g., "Jan")
 * - m  2-digit 0-padded month (01-12)
 * - d  2-digit 0-padded day in month (01-31)
 * - e  2-character space-padded day in month ( 1-31)
 * - a  Abbreviated day of week (e.g., "Mon")
 * - p  Part of day (AM/PM)
 * - H  2-digit 0-padded hour on 24-hour clock (00-23)
 * - k  2-character space-padded hour on 24-hour clock ( 0-23)
 * - I  2-digit 0-padded hour on 12-hour clock (01-12)
 * - l  2-character space-padded hour on 12-hour clock ( 1-12)
 * - M  2-digit 0-padded minute (00-59)
 * - S  2-digit 0-padded second (00-60) (60 to account for leap seconds)
 * - 3  0-padded millisecond (000-999)
 * - #  A relative timestamp with the unit indicated by the number following.
 *      NOTE: Currently, clp only supports timestamps up to millisecond precision, so microsecond
 *      and nanosecond timestamps will be truncated.
 *      - 3  Milliseconds
 *      - 6  Microseconds
 *      - 9  Nanoseconds
 */
class TimestampPattern {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}

        // Methods
        char const* what() const noexcept override { return "TimestampPattern operation failed"; }
    };

    // Constructors
    TimestampPattern() : m_num_spaces_before_ts(0) {}

    TimestampPattern(uint8_t num_spaces_before_ts, std::string const& format)
            : m_num_spaces_before_ts(num_spaces_before_ts),
              m_format(format) {}

    // Methods
    /**
     * Static initializer for class. This must be called before using the class.
     */
    static void init();

    /**
     * Searches for a known timestamp pattern which can parse the timestamp from the given line, and
     * if found, parses the timestamp
     * @param line
     * @param timestamp Parsed timestamp
     * @param timestamp_begin_pos
     * @param timestamp_end_pos
     * @return pointer to the timestamp pattern if found, nullptr otherwise
     */
    static TimestampPattern const* search_known_ts_patterns(
            std::string const& line,
            epochtime_t& timestamp,
            size_t& timestamp_begin_pos,
            size_t& timestamp_end_pos
    );

    /**
     * Gets the timestamp pattern's format string
     * @return See description
     */
    std::string const& get_format() const;
    /**
     * Gets the number of spaces before the timestamp in a typical message
     * @return See description
     */
    uint8_t get_num_spaces_before_ts() const;
    /**
     * Gets if the timestamp pattern is empty
     * @return true if empty, false otherwise
     */
    bool is_empty() const;

    /**
     * Clears the pattern
     */
    void clear();

    /**
     * Tries to parse the timestamp from the given line
     * @param line
     * @param timestamp Parsed timestamp
     * @param timestamp_begin_pos
     * @param timestamp_end_pos
     * @return true if parsed successfully, false otherwise
     */
    bool parse_timestamp(
            std::string const& line,
            epochtime_t& timestamp,
            size_t& timestamp_begin_pos,
            size_t& timestamp_end_pos
    ) const;
    /**
     * Inserts the timestamp into the given message using this pattern
     * @param timestamp
     * @param msg
     * @throw TimestampPattern::OperationFailed if the the pattern contains unsupported format
     * specifiers or the message cannot fit the timestamp pattern
     */
    void insert_formatted_timestamp(epochtime_t timestamp, std::string& msg) const;

    /**
     * Compares two timestamp patterns for equality
     * @param lhs
     * @param rhs
     * @return true if equal, false otherwise
     */
    friend bool operator==(TimestampPattern const& lhs, TimestampPattern const& rhs);
    /**
     * Compares two timestamp patterns for inequality
     * @param lhs
     * @param rhs
     * @return true if not equal, false otherwise
     */
    friend bool operator!=(TimestampPattern const& lhs, TimestampPattern const& rhs);

private:
    // Variables
    static std::unique_ptr<TimestampPattern[]> m_known_ts_patterns;
    static size_t m_known_ts_patterns_len;

    // The number of spaces before the timestamp in a message
    // E.g. in "localhost - - [01/Jan/2016:15:50:17", there are 3 spaces before the timestamp
    //                   ^ ^ ^
    uint8_t m_num_spaces_before_ts;
    std::string m_format;
};
}  // namespace glt

#endif  // GLT_TIMESTAMPPATTERN_HPP
