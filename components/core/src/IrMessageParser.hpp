#ifndef IrMessageParser_HPP
#define IrMessageParser_HPP

#include "ffi/ir_stream/decoding_methods.hpp"
#include "ParsedIrMessage.hpp"
#include "TraceableException.hpp"

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
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}

        // Methods
        [[nodiscard]] auto what() const noexcept -> char const* override {
            return "IrMessageParser operation failed";
        }
    };

    // Constructor
    explicit IrMessageParser(ReaderInterface& reader);

    // Methods
    auto get_ts_pattern() -> TimestampPattern* { return &m_ts_pattern; }

    [[nodiscard]] auto get_parsed_msg() const -> ParsedIrMessage const& { return m_msg; }

    auto get_msg_logtype_entry() -> LogTypeDictionaryEntry& { return m_msg.get_logtype_entry(); }

    [[nodiscard]] auto parse_next_encoded_message() -> bool;
    static auto is_ir_encoded(size_t sequence_length, char const* data) -> bool;

private:
    [[nodiscard]] auto parse_next_four_bytes_message() -> bool;
    [[nodiscard]] auto parse_next_eight_bytes_message() -> bool;
    [[nodiscard]] auto decode_json_preamble(std::string& json_metadata) -> bool;

    // member variables
    bool m_is_four_bytes_encoded{false};
    epochtime_t m_reference_timestamp;
    TimestampPattern m_ts_pattern;
    ParsedIrMessage m_msg;
    ReaderInterface& m_reader;
};

#endif  // IrMessageParser_HPP
