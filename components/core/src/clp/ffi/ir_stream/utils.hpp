#ifndef CLP_FFI_IR_STREAM_UTILS_HPP
#define CLP_FFI_IR_STREAM_UTILS_HPP

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include <json/single_include/nlohmann/json.hpp>
#include <outcome/single-header/outcome.hpp>

#include "../../ErrorCode.hpp"
#include "../../ir/types.hpp"
#include "../../ReaderInterface.hpp"
#include "../../type_utils.hpp"
#include "../SchemaTree.hpp"
#include "byteswap.hpp"
#include "decoding_methods.hpp"
#include "encoding_methods.hpp"
#include "protocol_constants.hpp"

namespace clp::ffi::ir_stream {
/**
 * Serializes the given metadata into the IR stream.
 * @param metadata
 * @param output_buf
 * @return Whether serialization succeeded.
 */
[[nodiscard]] auto serialize_metadata(nlohmann::json& metadata, std::vector<int8_t>& output_buf)
        -> bool;

/**
 * Serializes the given integer into the IR stream.
 * @tparam integer_t
 * @param value
 * @param output_buf
 */
template <IntegerType integer_t>
auto serialize_int(integer_t value, std::vector<int8_t>& output_buf) -> void;

/**
 * Deserializes an integer from the given reader
 * @tparam integer_t Type of the integer to deserialize
 * @param reader
 * @param value Returns the deserialized integer
 * @return Whether the reader contained enough data to deserialize.
 */
template <IntegerType integer_t>
[[nodiscard]] auto deserialize_int(ReaderInterface& reader, integer_t& value) -> bool;

/**
 * Serializes a string using CLP's encoding for unstructured text.
 * @tparam encoded_variable_t
 * @param str
 * @param logtype Returns the corresponding logtype.
 * @param output_buf
 * @return Whether serialization succeeded.
 */
template <typename encoded_variable_t>
[[nodiscard]] auto
serialize_clp_string(std::string_view str, std::string& logtype, std::vector<int8_t>& output_buf)
        -> bool;

/**
 * Serializes a string.
 * @param str
 * @param output_buf
 * @return Whether serialization succeeded.
 */
[[nodiscard]] auto serialize_string(std::string_view str, std::vector<int8_t>& output_buf) -> bool;

/**
 * @tparam T
 * @param int_val
 * @return One's complement of `int_val`.
 */
template <IntegerType T>
[[nodiscard]] auto get_ones_complement(T int_val) -> T;

/**
 * Encodes and serializes a schema tree node ID.
 * @tparam is_auto_generated_node Whether the schema tree node ID is from the auto-generated or the
 * user-generated schema tree.
 * @tparam one_byte_length_indicator_tag Tag for one-byte node ID encoding.
 * @tparam two_byte_length_indicator_tag Tag for two-byte node ID encoding.
 * @tparam four_byte_length_indicator_tag Tag for four-byte node ID encoding.
 * @param node_id
 * @param output_buf
 * @return true on success.
 * @return false if the ID exceeds the representable range.
 */
template <
        bool is_auto_generated_node,
        int8_t one_byte_length_indicator_tag,
        int8_t two_byte_length_indicator_tag,
        int8_t four_byte_length_indicator_tag>
[[nodiscard]] auto encode_and_serialize_schema_tree_node_id(
        SchemaTree::Node::id_t node_id,
        std::vector<int8_t>& output_buf
) -> bool;

/**
 * Deserializes and decodes a schema tree node ID.
 * @tparam one_byte_length_indicator_tag Tag for one-byte node ID encoding.
 * @tparam two_byte_length_indicator_tag Tag for two-byte node ID encoding.
 * @tparam four_byte_length_indicator_tag Tag for four-byte node ID encoding.
 * @param length_indicator_tag
 * @param reader
 * @return A result containing a pair or an error code indicating the failure:
 * - The pair:
 *   - Whether the node ID is for an auto-generated node.
 *   - The decoded node ID.
 * - The possible error codes:
 *   - std::errc::protocol_error if the given length indicator is unknown.
 * @return Forwards `size_dependent_deserialize_and_decode_schema_tree_node_id`'s return values.
 */
template <
        int8_t one_byte_length_indicator_tag,
        int8_t two_byte_length_indicator_tag,
        int8_t four_byte_length_indicator_tag>
[[nodiscard]] auto deserialize_and_decode_schema_tree_node_id(
        encoded_tag_t length_indicator_tag,
        ReaderInterface& reader
) -> OUTCOME_V2_NAMESPACE::std_result<std::pair<bool, SchemaTree::Node::id_t>>;

/**
 * @param ir_error_code
 * @return Equivalent `std::errc` code indicating the same error type.
 */
[[nodiscard]] auto ir_error_code_to_errc(IRErrorCode ir_error_code) -> std::errc;

template <IntegerType integer_t>
auto serialize_int(integer_t value, std::vector<int8_t>& output_buf) -> void {
    integer_t value_big_endian{};
    if constexpr (sizeof(value) == 1) {
        output_buf.push_back(bit_cast<int8_t>(value));
        return;
    } else if constexpr (sizeof(value) == 2) {
        value_big_endian = bswap_16(value);
    } else if constexpr (sizeof(value) == 4) {
        value_big_endian = bswap_32(value);
    } else if constexpr (sizeof(value) == 8) {
        value_big_endian = bswap_64(value);
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    std::span<int8_t> const data_view{reinterpret_cast<int8_t*>(&value_big_endian), sizeof(value)};
    output_buf.insert(output_buf.end(), data_view.begin(), data_view.end());
}

template <IntegerType integer_t>
auto deserialize_int(ReaderInterface& reader, integer_t& value) -> bool {
    integer_t value_little_endian;
    if (reader.try_read_numeric_value(value_little_endian) != clp::ErrorCode_Success) {
        return false;
    }

    constexpr auto cReadSize = sizeof(integer_t);
    if constexpr (cReadSize == 1) {
        value = value_little_endian;
    } else if constexpr (cReadSize == 2) {
        value = bswap_16(value_little_endian);
    } else if constexpr (cReadSize == 4) {
        value = bswap_32(value_little_endian);
    } else if constexpr (cReadSize == 8) {
        value = bswap_64(value_little_endian);
    }
    return true;
}

template <typename encoded_variable_t>
[[nodiscard]] auto
serialize_clp_string(std::string_view str, std::string& logtype, std::vector<int8_t>& output_buf)
        -> bool {
    static_assert(
            (std::is_same_v<encoded_variable_t, clp::ir::eight_byte_encoded_variable_t>
             || std::is_same_v<encoded_variable_t, clp::ir::four_byte_encoded_variable_t>)
    );
    bool succeeded{};
    if constexpr (std::is_same_v<encoded_variable_t, clp::ir::four_byte_encoded_variable_t>) {
        output_buf.push_back(cProtocol::Payload::ValueFourByteEncodingClpStr);
        succeeded = four_byte_encoding::serialize_message(str, logtype, output_buf);
    } else {
        output_buf.push_back(cProtocol::Payload::ValueEightByteEncodingClpStr);
        succeeded = eight_byte_encoding::serialize_message(str, logtype, output_buf);
    }
    return succeeded;
}

template <IntegerType T>
auto get_ones_complement(T int_val) -> T {
    // Explicit cast to undo the implicit integer promotion
    return static_cast<T>(~int_val);
}

template <
        bool is_auto_generated_node,
        int8_t one_byte_length_indicator_tag,
        int8_t two_byte_length_indicator_tag,
        int8_t four_byte_length_indicator_tag>
auto encode_and_serialize_schema_tree_node_id(
        SchemaTree::Node::id_t node_id,
        std::vector<int8_t>& output_buf
) -> bool {
    auto size_dependent_encode_and_serialize_schema_tree_node_id
            = [&output_buf,
               &node_id]<SignedIntegerType encoded_node_id_t>(int8_t length_indicator_tag) -> void {
        output_buf.push_back(length_indicator_tag);
        if constexpr (is_auto_generated_node) {
            serialize_int(get_ones_complement(static_cast<encoded_node_id_t>(node_id)), output_buf);
        } else {
            serialize_int(static_cast<encoded_node_id_t>(node_id), output_buf);
        }
    };

    if (node_id <= static_cast<SchemaTree::Node::id_t>(INT8_MAX)) {
        size_dependent_encode_and_serialize_schema_tree_node_id.template operator(
        )<int8_t>(one_byte_length_indicator_tag);
    } else if (node_id <= static_cast<SchemaTree::Node::id_t>(INT16_MAX)) {
        size_dependent_encode_and_serialize_schema_tree_node_id.template operator(
        )<int16_t>(two_byte_length_indicator_tag);
    } else if (node_id <= static_cast<SchemaTree::Node::id_t>(INT32_MAX)) {
        size_dependent_encode_and_serialize_schema_tree_node_id.template operator(
        )<int32_t>(four_byte_length_indicator_tag);
    } else {
        return false;
    }
    return true;
}

template <
        int8_t one_byte_length_indicator_tag,
        int8_t two_byte_length_indicator_tag,
        int8_t four_byte_length_indicator_tag>
auto deserialize_and_decode_schema_tree_node_id(
        encoded_tag_t length_indicator_tag,
        ReaderInterface& reader
) -> OUTCOME_V2_NAMESPACE::std_result<std::pair<bool, SchemaTree::Node::id_t>> {
    auto size_dependent_deserialize_and_decode_schema_tree_node_id
            = [&reader]<SignedIntegerType encoded_node_id_t>(
              ) -> OUTCOME_V2_NAMESPACE::std_result<std::pair<bool, SchemaTree::Node::id_t>> {
        encoded_node_id_t encoded_node_id{};
        if (false == deserialize_int(reader, encoded_node_id)) {
            return std::errc::result_out_of_range;
        }
        if (0 > encoded_node_id) {
            return {true, static_cast<SchemaTree::Node::id_t>(get_ones_complement(encoded_node_id))
            };
        }
        return {false, static_cast<SchemaTree::Node::id_t>(encoded_node_id)};
    };

    if (one_byte_length_indicator_tag == length_indicator_tag) {
        return size_dependent_deserialize_and_decode_schema_tree_node_id.template operator(
        )<int8_t>();
    }
    if (two_byte_length_indicator_tag == length_indicator_tag) {
        return size_dependent_deserialize_and_decode_schema_tree_node_id.template operator(
        )<int16_t>();
    }
    if (four_byte_length_indicator_tag == length_indicator_tag) {
        return size_dependent_deserialize_and_decode_schema_tree_node_id.template operator(
        )<int32_t>();
    }
    return std::errc::protocol_error;
}
}  // namespace clp::ffi::ir_stream
#endif
