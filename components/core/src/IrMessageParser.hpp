
#ifndef IrMessageParser_HPP
#define IrMessageParser_HPP

// C standard libraries

// C++ standard libraries

// Project headers
#include "TraceableException.hpp"
#include "ffi/ir_stream/decoding_methods.hpp"
#include "ParsedIrMessage.hpp"

/*
 * Class representing the parser that parses messages from encoded IR and
 * converts the message into CLP encoding format
 */
class IrMessageParser {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed (ErrorCode error_code, const char* const filename, int line_number) :
                  TraceableException (error_code, filename, line_number) {}

        // Methods
        const char* what () const noexcept override {
            return "IrMessageParser operation failed";
        }
    };
    // Constructor
    IrMessageParser (ReaderInterface& reader);

    // Methods
    static bool is_ir_encoded (size_t sequence_length, const char* data);
    TimestampPattern* get_ts_pattern () { return &m_ts_pattern; }
    const ParsedIrMessage& get_parsed_msg () const { return m_msg; }
    LogTypeDictionaryEntry& get_msg_logtype_entry() { return m_msg.get_logtype_entry(); }
    bool parse_next_encoded_message ();

private:

    bool parse_next_four_bytes_message();
    bool parse_next_eight_bytes_message();
    bool decode_json_preamble (std::string& json_metadata);
    bool is_ir_encoded (ReaderInterface& reader, bool& is_four_bytes_encoded);

    // member variables
    bool m_is_four_bytes_encoded;
    epochtime_t m_reference_timestamp;
    TimestampPattern m_ts_pattern;
    ParsedIrMessage m_msg;
    ReaderInterface& m_reader;
};

#endif // IrMessageParser_HPP