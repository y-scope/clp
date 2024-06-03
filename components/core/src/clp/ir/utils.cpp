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
    constexpr size_t cLogtypeLengthSize = sizeof(int32_t);
    constexpr size_t cTagSize = sizeof(char);
    constexpr size_t cVarSize = sizeof(int32_t);
    constexpr size_t cTimestampSize = sizeof(int64_t);
    constexpr size_t cPlaceHolderSize = sizeof(enum_to_underlying_type(VariablePlaceholder()));

    // sizeof(log type) + sizeof (dict vars) + sizeof(encoded_vars) ~= The size of log message
    auto ir_size = log_message.size();

    // Add the size of placeholders in the original log type
    ir_size += num_encoded_vars * cPlaceHolderSize;

    // Add the tags and encoding length bytes of log type
    ir_size += cTagSize + cLogtypeLengthSize;

    // Add the tags and encoding length bytes for dictionary variables
    // Note here we overestimate the size by assuming that encoded float and int also have length
    // encoding bytes
    ir_size += (cTagSize + cVarSize) * num_encoded_vars;

    // Add the tags and encoding length bytes of log type
    ir_size += cTagSize + cTimestampSize;

    return ir_size;
}

auto get_ir_extension_name() -> std::string {
    return "clp.zst";
}

}  // namespace clp::ir
