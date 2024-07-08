#ifndef CLP_FFI_IR_STREAM_UTILS_HPP
#define CLP_FFI_IR_STREAM_UTILS_HPP

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <json/single_include/nlohmann/json.hpp>

#include "../../ir/types.hpp"
#include "byteswap.hpp"
#include "encoding_methods.hpp"
#include "protocol_constants.hpp"

namespace clp::ffi::ir_stream {
/**
 * Serializes the given metadata into the IR stream.
 * @param metadata
 * @param ir_buf
 * @return Whether serialization succeeded.
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

/**
 * Serializes a string using clp encoding.
 * @tparam encoded_variable_t
 * @param str
 * @param buf Outputs the serialized byte sequence.
 * @return Whether serialization succeeded.
 */
template <typename encoded_variable_t>
[[nodiscard]] auto serialize_clp_string(std::string_view str, std::vector<int8_t>& buf) -> bool;

/**
 * Serializes a string packet.
 * @param str
 * @param buf Outputs the serialized byte sequence.
 * @return Whether the serialization succeeded.
 */
[[nodiscard]] auto serialize_string(std::string_view str, std::vector<int8_t>& buf) -> bool;

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

template <typename encoded_variable_t>
[[nodiscard]] auto serialize_clp_string(std::string_view str, std::vector<int8_t>& buf) -> bool {
    static_assert(
            (std::is_same_v<encoded_variable_t, clp::ir::eight_byte_encoded_variable_t>
             || std::is_same_v<encoded_variable_t, clp::ir::four_byte_encoded_variable_t>)
    );
    std::string logtype;
    bool error{};
    if constexpr (std::is_same_v<encoded_variable_t, clp::ir::four_byte_encoded_variable_t>) {
        buf.push_back(cProtocol::Payload::ValueFourByteEncodingClpStr);
        error = four_byte_encoding::serialize_message(str, logtype, buf);
    } else {
        buf.push_back(cProtocol::Payload::ValueEightByteEncodingClpStr);
        error = eight_byte_encoding::serialize_message(str, logtype, buf);
    }
    return error;
}
}  // namespace clp::ffi::ir_stream
#endif
