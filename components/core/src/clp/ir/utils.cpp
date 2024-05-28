#include "utils.hpp"

#include "../BufferReader.hpp"
#include "../ffi/ir_stream/decoding_methods.hpp"
#include "types.hpp"

namespace clp::ir {
auto has_ir_stream_magic_number(std::string_view buf) -> bool {
    BufferReader buf_reader{buf.data(), buf.size()};
    bool is_four_bytes_encoded{false};
    return ffi::ir_stream::IRErrorCode_Success
           == ffi::ir_stream::get_encoding_type(buf_reader, is_four_bytes_encoded);
}

auto get_approximated_ir_size(std::string_view log_message, size_t num_encoded_vars) -> size_t {

    size_t const tag_size = sizeof(char);

    // sizeof(log type) + sizeof (dict vars) + sizeof(encoded_vars) ~= The size of log message
    auto ir_size = log_message.size();

    // Add the size of placeholders in the original log type
    ir_size += num_encoded_vars * sizeof(enum_to_underlying_type(VariablePlaceholder()));

    // Add the tags and encoding length bytes of log type
    size_t const logtype_length_encoding_size = sizeof(int32_t);
    ir_size += tag_size + logtype_length_encoding_size;

    // Add the tags and encoding length bytes for dictionary variables
    // Note here we overestimate the size by assuming that encoded float
    // and int also have length encoding bytes
    size_t const var_encoding_size = sizeof(int32_t);
    ir_size += (tag_size + var_encoding_size) * num_encoded_vars;

    // Add the tags and encoding length bytes of log type
    size_t const timestamp_length_encoding_size = sizeof(int64_t);
    ir_size += tag_size + timestamp_length_encoding_size;

    return ir_size;
}


}  // namespace clp::ir