#ifndef CLP_FFI_IR_STREAM_UTILS_INC
#define CLP_FFI_IR_STREAM_UTILS_INC

#include <cstdint>
#include <span>
#include <vector>

#include "byteswap.hpp"

namespace clp::ffi::ir_stream {
template <typename integer_t>
auto serialize_int(integer_t value, std::vector<int8_t>& ir_buf) -> void {
    integer_t value_big_endian{};
    static_assert(sizeof(integer_t) == 2 || sizeof(integer_t) == 4 || sizeof(integer_t) == 8);
    if constexpr (sizeof(value) == 2) {
        value_big_endian = bswap_16(value);
    } else if constexpr (sizeof(value) == 4) {
        value_big_endian = bswap_32(value);
    } else if constexpr (sizeof(value) == 8) {
        value_big_endian = bswap_64(value);
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    std::span<int8_t> const data_view{reinterpret_cast<int8_t*>(&value_big_endian), sizeof(value)};
    ir_buf.insert(ir_buf.end(), data_view.begin(), data_view.end());
}
}  // namespace clp::ffi::ir_stream

#endif
