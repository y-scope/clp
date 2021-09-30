#ifndef MESSAGEPARSER_HPP
#define MESSAGEPARSER_HPP

// C++ standard libraries
#include <string>

// Project headers
#include "ErrorCode.hpp"
#include "ParsedMessage.hpp"
#include "ReaderInterface.hpp"
#include "TraceableException.hpp"

/**
 * Class to parse log messages
 */
class MessageParser {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed (ErrorCode error_code, const char* const filename, int line_number) : TraceableException (error_code, filename, line_number) {}

        // Methods
        const char* what () const noexcept override {
            return "MessageParser operation failed";
        }
    };

    // Methods
    /**
     * Parses the next message from the given buffer. Messages are delimited either by i) a timestamp or ii) a line break if no timestamp is found.
     * @param drain_source Whether to drain all content from the file or just lines with endings
     * @param buffer_length
     * @param buffer
     * @param buf_pos
     * @param message
     * @return true if message parsed, false otherwise
     */
    bool parse_next_message (bool drain_source, size_t buffer_length, const char* buffer, size_t& buf_pos, ParsedMessage& message);
    /**
     * Parses the next message from the given reader. Messages are delimited either by i) a timestamp or ii) a line break if no timestamp is found.
     * @param drain_source Whether to drain all content from the reader or just lines with endings
     * @param reader
     * @param message
     * @return true if message parsed, false otherwise
     */
    bool parse_next_message (bool drain_source, ReaderInterface& reader, ParsedMessage& message);

private:
    // Methods
    /**
     * Parses the line and adds it either to the buffered message if incomplete, or the given message if complete
     * @param message
     * @return Whether a complete message has been parsed
     */
    bool parse_line (ParsedMessage& message);

    // Variables
    std::string m_line;
    ParsedMessage m_buffered_msg;
};

#endif // MESSAGEPARSER_HPP
