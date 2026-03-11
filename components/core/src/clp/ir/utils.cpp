#include "utils.hpp"

#include "../BufferReader.hpp"
#include "../ffi/ir_stream/decoding_methods.hpp"

namespace clp::ir {
auto has_ir_stream_magic_number(std::string_view buf) -> bool {
    BufferReader buf_reader{buf.data(), buf.size()};
    bool is_four_bytes_encoded{false};
    return ffi::ir_stream::get_encoding_type(buf_reader, is_four_bytes_encoded).has_value();
}
}  // namespace clp::ir
