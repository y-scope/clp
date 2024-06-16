#ifndef CLP_FFI_IR_STREAM_UTILS_HPP
#define CLP_FFI_IR_STREAM_UTILS_HPP

#include <cstdint>
#include <vector>

#include <json/single_include/nlohmann/json.hpp>

namespace clp::ffi::ir_stream {
/**
 * Serializes the given metadata into the IR stream.
 * @param metadata
 * @param ir_buf
 * @return Whether the serialization succeeded.
 */
[[nodiscard]] auto
serialize_metadata(nlohmann::json& metadata, std::vector<int8_t>& ir_buf) -> bool;

/**
 * Serializes the given integer into the IR stream.
 * @tparam integer_t
 * @param value
 * @param ir_buf
 */
template <typename integer_t>
auto serialize_int(integer_t value, std::vector<int8_t>& ir_buf) -> void;
}  // namespace clp::ffi::ir_stream

#include "utils.inc"

#endif
