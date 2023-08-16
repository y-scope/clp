#ifndef FFI_IR_STREAM_DECODING_METHODS_HPP
#define FFI_IR_STREAM_DECODING_METHODS_HPP

#include <string>
#include <vector>

#include "../../ReaderInterface.hpp"
#include "../encoding_methods.hpp"

namespace ffi::ir_stream {
using encoded_tag_t = int8_t;

typedef enum {
    IRErrorCode_Success,
    IRErrorCode_Decode_Error,
    IRErrorCode_Eof,
    IRErrorCode_Corrupted_IR,
    IRErrorCode_Incomplete_IR,
} IRErrorCode;

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
 * Decodes the encoding type for the encoded IR stream
 * @param reader
 * @param is_four_bytes_encoding Returns the encoding type
 * @return ErrorCode_Success on success
 * @return ErrorCode_Corrupted_IR if reader contains invalid IR
 * @return ErrorCode_Incomplete_IR if reader doesn't contain enough data to
 * decode
 */
IRErrorCode get_encoding_type(ReaderInterface& reader, bool& is_four_bytes_encoding);

/**
 * Parse logtypes, dictionary variables and encoded variables
 * from the next encoded IR message. Returns the parsed tokens by
 * reference
 * @tparam encoded_variable_t
 * @param reader
 * @param logtype
 * @param encoded_vars
 * @param dict_vars
 * @param timestamp
 * @return IRErrorCode_Success on success
 * @return IRErrorCode_Corrupted_IR if reader contains invalid IR
 * @return IRErrorCode_Incomplete_IR if reader doesn't contain enough data
 */
template <typename encoded_variable_t>
auto generic_parse_tokens(
        ReaderInterface& reader,
        std::string& logtype,
        std::vector<encoded_variable_t>& encoded_vars,
        std::vector<std::string>& dict_vars,
        epoch_time_ms_t& timestamp
) -> IRErrorCode;

/**
 * Decodes the message consists of the tokens and calls the given methods
 * to handle specific components of the message.
 * @tparam encoded_variable_t Type of the encoded variable
 * @tparam ConstantHandler Method to handle constants in the logtypes.
 * Signature: (const std::string&, size_t, size_t) -> void
 * @tparam ConstantRemainderHandler Method to handle the last constant in the
 * logtypes. Signature: (const std::string&, size_t) -> void
 * @tparam EncodedIntHandler Method to handle encoded integers.
 * Signature: (encoded_variable_t) -> void
 * @tparam EncodedFloatHandler Method to handle encoded float.
 * Signature: (encoded_variable_t) -> void
 * @tparam DictVarHandler Method to handle dictionary variables.
 * Signature: (const std::string&) -> void
 * @param logtype
 * @param encoded_vars
 * @param dict_vars
 * @param constant_handler
 * @param constant_remainder_handler
 * @param encoded_int_handler
 * @param encoded_float_handler
 * @param dict_var_handler
 * @throw DecodingException if the message can not be decoded properly
 */
template <
        typename encoded_variable_t,
        typename ConstantHandler,
        typename ConstantRemainderHandler,
        typename EncodedIntHandler,
        typename EncodedFloatHandler,
        typename DictVarHandler>
void generic_decode_message(
        std::string const& logtype,
        std::vector<encoded_variable_t> const& encoded_vars,
        std::vector<std::string> const& dict_vars,
        ConstantHandler constant_handler,
        ConstantRemainderHandler constant_remainder_handler,
        EncodedIntHandler encoded_int_handler,
        EncodedFloatHandler encoded_float_handler,
        DictVarHandler dict_var_handler
);

/**
 * Decodes the preamble for an IR stream.
 * @param reader
 * @param metadata_type Returns the type of the metadata found in the IR
 * @param metadata_pos Returns the starting position of the metadata in reader
 * @param metadata_size Returns the size of the metadata written in the IR
 * @return IRErrorCode_Success on success
 * @return IRErrorCode_Corrupted_IR if reader contains invalid IR
 * @return IRErrorCode_Incomplete_IR if reader doesn't contain enough data to
 * decode
 */
IRErrorCode decode_preamble(
        ReaderInterface& reader,
        encoded_tag_t& metadata_type,
        size_t& metadata_pos,
        uint16_t& metadata_size
);

/**
 * Decodes the preamble for an IR stream.
 * @param reader
 * @param metadata_type Returns the type of the metadata found in the IR
 * @param metadata Returns the metadata in the given vector
 * @return IRErrorCode_Success on success
 * @return IRErrorCode_Corrupted_IR if reader contains invalid IR
 * @return IRErrorCode_Incomplete_IR if reader doesn't contain enough
 * data to decode
 */
IRErrorCode decode_preamble(
        ReaderInterface& reader,
        encoded_tag_t& metadata_type,
        std::vector<int8_t>& metadata
);

namespace eight_byte_encoding {
    /**
     * Decodes the next message for the eight-byte encoding IR stream.
     * @param reader
     * @param message Returns the decoded message
     * @param timestamp Returns the decoded timestamp
     * @return ErrorCode_Success on success
     * @return ErrorCode_Corrupted_IR if reader contains invalid IR
     * @return ErrorCode_Decode_Error if the encoded message cannot be properly
     * decoded
     * @return ErrorCode_Incomplete_IR if reader doesn't contain enough data to
     * decode
     * @return ErrorCode_End_of_IR if the IR ends
     */
    IRErrorCode
    decode_next_message(ReaderInterface& reader, std::string& message, epoch_time_ms_t& timestamp);
}  // namespace eight_byte_encoding

namespace four_byte_encoding {
    /**
     * Decodes the next message for the four-byte encoding IR stream.
     * @param reader
     * @param message Returns the decoded message
     * @param timestamp_delta Returns the decoded timestamp delta
     * @return ErrorCode_Success on success
     * @return ErrorCode_Corrupted_IR if reader contains invalid IR
     * @return ErrorCode_Decode_Error if the encoded message cannot be properly
     * decoded
     * @return ErrorCode_Incomplete_IR if reader doesn't contain enough data to
     * decode
     * @return ErrorCode_End_of_IR if the IR ends
     */
    IRErrorCode decode_next_message(
            ReaderInterface& reader,
            std::string& message,
            epoch_time_ms_t& timestamp_delta
    );
}  // namespace four_byte_encoding
}  // namespace ffi::ir_stream

#include "decoding_methods.tpp"

#endif  // FFI_IR_STREAM_DECODING_METHODS_HPP
