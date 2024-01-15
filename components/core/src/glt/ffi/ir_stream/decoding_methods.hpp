#ifndef GLT_FFI_IR_STREAM_DECODING_METHODS_HPP
#define GLT_FFI_IR_STREAM_DECODING_METHODS_HPP

#include <string>
#include <vector>

#include "../../ir/types.hpp"
#include "../../ReaderInterface.hpp"
#include "../encoding_methods.hpp"

namespace glt::ffi::ir_stream {
using encoded_tag_t = int8_t;

typedef enum {
    IRErrorCode_Success,
    IRErrorCode_Decode_Error,
    IRErrorCode_Eof,
    IRErrorCode_Corrupted_IR,
    IRErrorCode_Incomplete_IR,
} IRErrorCode;

typedef enum {
    IRProtocolErrorCode_Supported,
    IRProtocolErrorCode_Too_Old,
    IRProtocolErrorCode_Too_New,
    IRProtocolErrorCode_Invalid,
} IRProtocolErrorCode;

class DecodingException : public TraceableException {
public:
    // Constructors
    DecodingException(
            ErrorCode error_code,
            char const* const filename,
            int line_number,
            std::string message
    )
            : TraceableException(error_code, filename, line_number),
              m_message(std::move(message)) {}

    // Methods
    [[nodiscard]] char const* what() const noexcept override { return m_message.c_str(); }

private:
    std::string m_message;
};

/**
 * Deserializes the IR stream's encoding type
 * @param reader
 * @param is_four_bytes_encoding Returns the encoding type
 * @return ErrorCode_Success on success
 * @return ErrorCode_Corrupted_IR if reader contains invalid IR
 * @return ErrorCode_Incomplete_IR if reader doesn't contain enough data to decode
 */
IRErrorCode get_encoding_type(ReaderInterface& reader, bool& is_four_bytes_encoding);

/**
 * Deserializes a log event from the given stream
 * @tparam encoded_variable_t
 * @param reader
 * @param logtype Returns the logtype
 * @param encoded_vars Returns the encoded variables
 * @param dict_vars Returns the dictionary variables
 * @param timestamp_or_timestamp_delta Returns the timestamp (in the eight-byte encoding case) or
 * the timestamp delta (in the four-byte encoding case)
 * @return IRErrorCode_Success on success
 * @return IRErrorCode_Corrupted_IR if reader contains invalid IR
 * @return IRErrorCode_Incomplete_IR if reader doesn't contain enough data
 * @return IRErrorCode_Eof on reaching the end of the stream
 */
template <typename encoded_variable_t>
auto deserialize_log_event(
        ReaderInterface& reader,
        std::string& logtype,
        std::vector<encoded_variable_t>& encoded_vars,
        std::vector<std::string>& dict_vars,
        ir::epoch_time_ms_t& timestamp_or_timestamp_delta
) -> IRErrorCode;

/**
 * Decodes the IR message calls the given methods to handle each component of the message
 * @tparam unescape_logtype Whether to remove the escape characters from the logtype before calling
 * \p ConstantHandler
 * @tparam encoded_variable_t Type of the encoded variable
 * @tparam ConstantHandler Method to handle constants in the logtype.
 * Signature: (const std::string&, size_t, size_t) -> void
 * @tparam EncodedIntHandler Method to handle encoded integers.
 * Signature: (encoded_variable_t) -> void
 * @tparam EncodedFloatHandler Method to handle encoded floats.
 * Signature: (encoded_variable_t) -> void
 * @tparam DictVarHandler Method to handle dictionary variables.
 * Signature: (const std::string&) -> void
 * @param logtype
 * @param encoded_vars
 * @param dict_vars
 * @param constant_handler
 * @param encoded_int_handler
 * @param encoded_float_handler
 * @param dict_var_handler
 * @throw DecodingException if the message can not be decoded properly
 */
template <
        bool unescape_logtype,
        typename encoded_variable_t,
        typename ConstantHandler,
        typename EncodedIntHandler,
        typename EncodedFloatHandler,
        typename DictVarHandler>
void generic_decode_message(
        std::string const& logtype,
        std::vector<encoded_variable_t> const& encoded_vars,
        std::vector<std::string> const& dict_vars,
        ConstantHandler constant_handler,
        EncodedIntHandler encoded_int_handler,
        EncodedFloatHandler encoded_float_handler,
        DictVarHandler dict_var_handler
);

/**
 * Deserializes the preamble for an IR stream.
 * @param reader
 * @param metadata_type Returns the type of the metadata deserialized from the IR
 * @param metadata_pos Returns the starting position of the metadata in reader
 * @param metadata_size Returns the size of the metadata deserialized from the IR
 * @return IRErrorCode_Success on success
 * @return IRErrorCode_Corrupted_IR if reader contains invalid IR
 * @return IRErrorCode_Incomplete_IR if reader doesn't contain enough data to deserialize
 */
IRErrorCode deserialize_preamble(
        ReaderInterface& reader,
        encoded_tag_t& metadata_type,
        size_t& metadata_pos,
        uint16_t& metadata_size
);

/**
 * Deserializes the preamble for an IR stream.
 * @param reader
 * @param metadata_type Returns the type of the metadata deserialized from the IR
 * @param metadata Returns the metadata in the given vector
 * @return IRErrorCode_Success on success
 * @return IRErrorCode_Corrupted_IR if reader contains invalid IR
 * @return IRErrorCode_Incomplete_IR if reader doesn't contain enough data to deserialize
 */
IRErrorCode deserialize_preamble(
        ReaderInterface& reader,
        encoded_tag_t& metadata_type,
        std::vector<int8_t>& metadata
);

/**
 * Validates whether the given protocol version can be supported by the current build.
 * @param protocol_version
 * @return IRProtocolErrorCode_Supported if the protocol version is supported.
 * @return IRProtocolErrorCode_Too_Old if the protocol version is no longer supported by this
 * build's protocol version.
 * @return IRProtocolErrorCode_Too_New if the protocol version is newer than this build's protocol
 * version.
 * @return IRProtocolErrorCode_Invalid if the protocol version does not follow the SemVer
 * specification.
 */
IRProtocolErrorCode validate_protocol_version(std::string_view protocol_version);

namespace eight_byte_encoding {
/**
 * Deserializes the next log event from an eight-byte encoding IR stream.
 * @param reader
 * @param message Returns the deserialized message
 * @param timestamp Returns the deserialized timestamp
 * @return ErrorCode_Success on success
 * @return ErrorCode_Corrupted_IR if reader contains invalid IR
 * @return ErrorCode_Decode_Error if the log event cannot be properly deserialized
 * @return ErrorCode_Incomplete_IR if reader doesn't contain enough data to deserialize
 * @return ErrorCode_End_of_IR if the IR ends
 */
IRErrorCode deserialize_log_event(
        ReaderInterface& reader,
        std::string& message,
        ir::epoch_time_ms_t& timestamp
);
}  // namespace eight_byte_encoding

namespace four_byte_encoding {
/**
 * Deserializes the next log event from a four-byte encoding IR stream.
 * @param reader
 * @param message Returns the deserialized message
 * @param timestamp_delta Returns the deserialized timestamp delta
 * @return ErrorCode_Success on success
 * @return ErrorCode_Corrupted_IR if reader contains invalid IR
 * @return ErrorCode_Decode_Error if the log event cannot be properly deserialized
 * @return ErrorCode_Incomplete_IR if reader doesn't contain enough data to deserialize
 * @return ErrorCode_End_of_IR if the IR ends
 */
IRErrorCode deserialize_log_event(
        ReaderInterface& reader,
        std::string& message,
        ir::epoch_time_ms_t& timestamp_delta
);
}  // namespace four_byte_encoding
}  // namespace glt::ffi::ir_stream

#include "decoding_methods.inc"

#endif  // GLT_FFI_IR_STREAM_DECODING_METHODS_HPP
