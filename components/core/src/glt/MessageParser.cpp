#include "MessageParser.hpp"

#include "Defs.h"
#include "TimestampPattern.hpp"

constexpr char cLineDelimiter = '\n';

namespace glt {
bool MessageParser::parse_next_message(
        bool drain_source,
        size_t buffer_length,
        char const* buffer,
        size_t& buf_pos,
        ParsedMessage& message
) {
    message.clear_except_ts_patt();

    while (true) {
        // Check if the buffer was exhausted
        if (buffer_length == buf_pos) {
            break;
        }

        // Read a line up to the delimiter
        bool found_delim = false;
        for (; false == found_delim && buf_pos < buffer_length; ++buf_pos) {
            auto c = buffer[buf_pos];

            m_line += c;
            if (cLineDelimiter == c) {
                found_delim = true;
            }
        }

        if (false == found_delim && false == drain_source) {
            // No delimiter was found and the source doesn't need to be drained
            return false;
        }

        if (parse_line(message)) {
            return true;
        }
    }

    return false;
}

bool MessageParser::parse_next_message(
        bool drain_source,
        ReaderInterface& reader,
        ParsedMessage& message
) {
    message.clear_except_ts_patt();

    while (true) {
        // Read message
        auto error_code = reader.try_read_to_delimiter(cLineDelimiter, true, true, m_line);
        if (ErrorCode_Success != error_code) {
            if (ErrorCode_EndOfFile != error_code) {
                throw OperationFailed(error_code, __FILENAME__, __LINE__);
            }

            if (m_line.empty()) {
                if (m_buffered_msg.is_empty()) {
                    break;
                } else {
                    message.consume(m_buffered_msg);
                    return true;
                }
            }
        }
        if (false == drain_source && cLineDelimiter != m_line[m_line.length() - 1]) {
            return false;
        }

        if (parse_line(message)) {
            return true;
        }
    }

    return false;
}

/**
 * The general algorithm is as follows:
 * - Try to parse a timestamp from the line.
 * - If the line has a timestamp and...
 *   - ...the buffered message is empty, fill it and continue reading.
 *   - ...the buffered message is not empty, save the line for the next message and return the
 *     buffered message.
 * - Else if the line has no timestamp and...
 *   - ...the buffered message is empty, return the line as a message.
 *   - ...the buffered message is not empty, add the line to the message and continue reading.
 */
bool MessageParser::parse_line(ParsedMessage& message) {
    bool message_completed = false;

    // Parse timestamp and content
    TimestampPattern const* timestamp_pattern = message.get_ts_patt();
    epochtime_t timestamp = 0;
    size_t timestamp_begin_pos;
    size_t timestamp_end_pos;
    if (nullptr == timestamp_pattern
        || false
                   == timestamp_pattern->parse_timestamp(
                           m_line,
                           timestamp,
                           timestamp_begin_pos,
                           timestamp_end_pos
                   ))
    {
        timestamp_pattern = TimestampPattern::search_known_ts_patterns(
                m_line,
                timestamp,
                timestamp_begin_pos,
                timestamp_end_pos
        );
    }

    if (nullptr != timestamp_pattern) {
        // A timestamp was parsed
        if (m_buffered_msg.is_empty()) {
            // Fill message with line
            m_buffered_msg.set(
                    timestamp_pattern,
                    timestamp,
                    m_line,
                    timestamp_begin_pos,
                    timestamp_end_pos
            );
        } else {
            // Move buffered message to message
            message.consume(m_buffered_msg);

            // Save line for next message
            m_buffered_msg.set(
                    timestamp_pattern,
                    timestamp,
                    m_line,
                    timestamp_begin_pos,
                    timestamp_end_pos
            );
            message_completed = true;
        }
    } else {
        // No timestamp was parsed
        if (m_buffered_msg.is_empty()) {
            // Fill message with line
            message.set(
                    timestamp_pattern,
                    timestamp,
                    m_line,
                    timestamp_begin_pos,
                    timestamp_end_pos
            );
            message_completed = true;
        } else {
            // Append line to message
            m_buffered_msg.append_line(m_line);
        }
    }

    m_line.clear();
    return message_completed;
}
}  // namespace glt
