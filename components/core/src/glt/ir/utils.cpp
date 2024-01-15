#include "utils.hpp"

#include "../BufferReader.hpp"
#include "../ffi/ir_stream/decoding_methods.hpp"

namespace glt::ir {
auto has_ir_stream_magic_number(std::string_view buf) -> bool {
    BufferReader buf_reader{buf.data(), buf.size()};
    bool is_four_bytes_encoded{false};
    return ffi::ir_stream::IRErrorCode_Success
           == ffi::ir_stream::get_encoding_type(buf_reader, is_four_bytes_encoded);
}
}  // namespace glt::ir
