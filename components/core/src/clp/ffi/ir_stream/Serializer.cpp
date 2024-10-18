#include "Serializer.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include <json/single_include/nlohmann/json.hpp>
#include <msgpack.hpp>
#include <outcome/single-header/outcome.hpp>

#include "../../ir/types.hpp"
#include "../../time_types.hpp"
#include "../../type_utils.hpp"
#include "../encoding_methods.hpp"
#include "../SchemaTree.hpp"
#include "encoding_methods.hpp"
#include "protocol_constants.hpp"
#include "utils.hpp"

using std::optional;
using std::span;
using std::string;
using std::string_view;
using std::vector;

using clp::ir::eight_byte_encoded_variable_t;
using clp::ir::four_byte_encoded_variable_t;

namespace clp::ffi::ir_stream {
namespace {
/**
 * Class for iterating the kv-pairs of a MessagePack map.
 */
class MsgpackMapIterator {
public:
    // Types
    using Child = msgpack::object_kv;

    // Constructors
    MsgpackMapIterator(SchemaTree::Node::id_t schema_tree_node_id, span<Child> children)
            : m_schema_tree_node_id{schema_tree_node_id},
              m_children{children},
              m_curr_child_it{m_children.begin()} {}

    // Methods
    /**
     * @return This map's ID in the schema tree.
     */
    [[nodiscard]] auto get_schema_tree_node_id() const -> SchemaTree::Node::id_t {
        return m_schema_tree_node_id;
    }

    /**
     * @return Whether there are more children to traverse.
     */
    [[nodiscard]] auto has_next_child() const -> bool {
        return m_curr_child_it != m_children.end();
    }

    /**
     * Gets the next child and advances the underlying child idx.
     * @return The next child to traverse.
     */
    [[nodiscard]] auto get_next_child() -> Child const& { return *(m_curr_child_it++); }

private:
    SchemaTree::Node::id_t m_schema_tree_node_id;
    span<Child> m_children;
    span<Child>::iterator m_curr_child_it;
};

/**
 * Gets the schema-tree node type that corresponds with a given MessagePack value.
 * @param val
 * @return The corresponding schema-tree node type.
 * @return std::nullopt if the value doesn't match any of the supported schema-tree node types.
 */
[[nodiscard]] auto get_schema_tree_node_type_from_msgpack_val(msgpack::object const& val
) -> optional<SchemaTree::Node::Type>;

/**
 * Serializes an empty object.
 * @param output_buf
 */
auto serialize_value_empty_object(vector<int8_t>& output_buf) -> void;

/**
 * Serializes an integer.
 * @param val
 * @param output_buf
 * @return Whether serialization succeeded.
 */
auto serialize_value_int(int64_t val, vector<int8_t>& output_buf) -> void;

/**
 * Serializes a float.
 * @param val
 * @param output_buf
 */
auto serialize_value_float(double val, vector<int8_t>& output_buf) -> void;

/**
 * Serializes a boolean.
 * @param val
 * @param output_buf
 */
auto serialize_value_bool(bool val, vector<int8_t>& output_buf) -> void;

/**
 * Serializes a null.
 * @param output_buf
 */
auto serialize_value_null(vector<int8_t>& output_buf) -> void;

/**
 * Serializes a string.
 * @tparam encoded_variable_t
 * @param val
 * @param logtype_buf
 * @param output_buf
 * @return Whether serialization succeeded.
 */
template <typename encoded_variable_t>
[[nodiscard]] auto
serialize_value_string(string_view val, string& logtype_buf, vector<int8_t>& output_buf) -> bool;

/**
 * Serializes a MessagePack array as a JSON string, using CLP's encoding for unstructured text.
 * @tparam encoded_variable_t
 * @param val
 * @param logtype_buf
 * @param output_buf
 * @return Whether serialization succeeded.
 */
template <typename encoded_variable_t>
[[nodiscard]] auto serialize_value_array(
        msgpack::object const& val,
        string& logtype_buf,
        vector<int8_t>& output_buf
) -> bool;

auto get_schema_tree_node_type_from_msgpack_val(msgpack::object const& val
) -> optional<SchemaTree::Node::Type> {
    optional<SchemaTree::Node::Type> ret_val;
    switch (val.type) {
        case msgpack::type::POSITIVE_INTEGER:
        case msgpack::type::NEGATIVE_INTEGER:
            ret_val.emplace(SchemaTree::Node::Type::Int);
            break;
        case msgpack::type::FLOAT32:
        case msgpack::type::FLOAT64:
            ret_val.emplace(SchemaTree::Node::Type::Float);
            break;
        case msgpack::type::STR:
            ret_val.emplace(SchemaTree::Node::Type::Str);
            break;
        case msgpack::type::BOOLEAN:
            ret_val.emplace(SchemaTree::Node::Type::Bool);
            break;
        case msgpack::type::NIL:
        case msgpack::type::MAP:
            ret_val.emplace(SchemaTree::Node::Type::Obj);
            break;
        case msgpack::type::ARRAY:
            ret_val.emplace(SchemaTree::Node::Type::UnstructuredArray);
            break;
        default:
            return std::nullopt;
    }
    return ret_val;
}

auto serialize_value_empty_object(vector<int8_t>& output_buf) -> void {
    output_buf.push_back(cProtocol::Payload::ValueEmpty);
}

auto serialize_value_int(int64_t val, vector<int8_t>& output_buf) -> void {
    if (INT8_MIN <= val && val <= INT8_MAX) {
        output_buf.push_back(cProtocol::Payload::ValueInt8);
        output_buf.push_back(static_cast<int8_t>(val));
    } else if (INT16_MIN <= val && val <= INT16_MAX) {
        output_buf.push_back(cProtocol::Payload::ValueInt16);
        serialize_int(static_cast<int16_t>(val), output_buf);
    } else if (INT32_MIN <= val && val <= INT32_MAX) {
        output_buf.push_back(cProtocol::Payload::ValueInt32);
        serialize_int(static_cast<int32_t>(val), output_buf);
    } else {  // (INT64_MIN <= val && val <= INT64_MAX)
        output_buf.push_back(cProtocol::Payload::ValueInt64);
        serialize_int(val, output_buf);
    }
}

auto serialize_value_float(double val, vector<int8_t>& output_buf) -> void {
    output_buf.push_back(cProtocol::Payload::ValueFloat);
    serialize_int(bit_cast<uint64_t>(val), output_buf);
}

auto serialize_value_bool(bool val, vector<int8_t>& output_buf) -> void {
    output_buf.push_back(val ? cProtocol::Payload::ValueTrue : cProtocol::Payload::ValueFalse);
}

auto serialize_value_null(vector<int8_t>& output_buf) -> void {
    output_buf.push_back(cProtocol::Payload::ValueNull);
}

template <typename encoded_variable_t>
auto serialize_value_string(string_view val, string& logtype_buf, vector<int8_t>& output_buf)
        -> bool {
    if (string_view::npos == val.find(' ')) {
        return serialize_string(val, output_buf);
    }
    logtype_buf.clear();
    return serialize_clp_string<encoded_variable_t>(val, logtype_buf, output_buf);
}

template <typename encoded_variable_t>
auto serialize_value_array(
        msgpack::object const& val,
        string& logtype_buf,
        vector<int8_t>& output_buf
) -> bool {
    std::ostringstream oss;
    oss << val;
    logtype_buf.clear();
    return serialize_clp_string<encoded_variable_t>(oss.str(), logtype_buf, output_buf);
}
}  // namespace

template <typename encoded_variable_t>
auto Serializer<encoded_variable_t>::create(
) -> OUTCOME_V2_NAMESPACE::std_result<Serializer<encoded_variable_t>> {
    static_assert(
            (std::is_same_v<encoded_variable_t, eight_byte_encoded_variable_t>
             || std::is_same_v<encoded_variable_t, four_byte_encoded_variable_t>)
    );

    Serializer<encoded_variable_t> serializer;
    auto& ir_buf{serializer.m_ir_buf};
    constexpr BufferView cMagicNumber{
            static_cast<int8_t const*>(
                    std::is_same_v<encoded_variable_t, eight_byte_encoded_variable_t>
                            ? cProtocol::EightByteEncodingMagicNumber
                            : cProtocol::FourByteEncodingMagicNumber
            ),
            cProtocol::MagicNumberLength
    };
    ir_buf.insert(ir_buf.cend(), cMagicNumber.begin(), cMagicNumber.end());

    nlohmann::json metadata;
    metadata.emplace(cProtocol::Metadata::VersionKey, cProtocol::Metadata::BetaVersionValue);
    metadata.emplace(cProtocol::Metadata::VariablesSchemaIdKey, cVariablesSchemaVersion);
    metadata.emplace(
            cProtocol::Metadata::VariableEncodingMethodsIdKey,
            cVariableEncodingMethodsVersion
    );
    if (false == serialize_metadata(metadata, ir_buf)) {
        return std::errc::protocol_error;
    }

    return serializer;
}

template <typename encoded_variable_t>
auto Serializer<encoded_variable_t>::change_utc_offset(UtcOffset utc_offset) -> void {
    if (utc_offset != m_curr_utc_offset) {
        m_curr_utc_offset = utc_offset;
    }
    serialize_utc_offset_change(m_curr_utc_offset, m_ir_buf);
}

template <typename encoded_variable_t>
auto Serializer<encoded_variable_t>::serialize_msgpack_map(msgpack::object_map const& msgpack_map
) -> bool {
    if (0 == msgpack_map.size) {
        serialize_value_empty_object(m_ir_buf);
        return true;
    }

    m_schema_tree.take_snapshot();
    m_schema_tree_node_buf.clear();
    m_key_group_buf.clear();
    m_value_group_buf.clear();

    // Traverse the map using DFS iteratively
    bool failure{false};
    vector<MsgpackMapIterator> dfs_stack;
    dfs_stack.emplace_back(
            SchemaTree::cRootId,
            span<MsgpackMapIterator::Child>{msgpack_map.ptr, msgpack_map.size}
    );
    while (false == dfs_stack.empty()) {
        auto& curr{dfs_stack.back()};
        if (false == curr.has_next_child()) {
            // Visited all children, so pop node
            dfs_stack.pop_back();
            continue;
        }

        // Convert the current value's type to its corresponding schema-tree node type
        auto const& [key, val]{curr.get_next_child()};
        auto const opt_schema_tree_node_type{get_schema_tree_node_type_from_msgpack_val(val)};
        if (false == opt_schema_tree_node_type.has_value()) {
            failure = true;
            break;
        }
        auto const schema_tree_node_type{opt_schema_tree_node_type.value()};

        SchemaTree::NodeLocator const locator{
                curr.get_schema_tree_node_id(),
                key.as<string_view>(),
                schema_tree_node_type
        };

        // Get the schema-tree node that corresponds with the current kv-pair, or add it if it
        // doesn't exist.
        auto opt_schema_tree_node_id{m_schema_tree.try_get_node_id(locator)};
        if (false == opt_schema_tree_node_id.has_value()) {
            opt_schema_tree_node_id.emplace(m_schema_tree.insert_node(locator));
            if (false == serialize_schema_tree_node(locator)) {
                failure = true;
                break;
            }
        }
        auto const schema_tree_node_id{opt_schema_tree_node_id.value()};

        if (msgpack::type::MAP == val.type) {
            // Serialize map
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
            auto const& inner_map{val.via.map};
            auto const inner_map_size(inner_map.size);
            if (0 == inner_map_size) {
                // Value is an empty map, so we can serialize it immediately
                if (false == serialize_key(schema_tree_node_id)) {
                    failure = true;
                    break;
                }
                serialize_value_empty_object(m_value_group_buf);
            } else {
                // Add map for DFS iteration
                dfs_stack.emplace_back(
                        schema_tree_node_id,
                        span<MsgpackMapIterator::Child>{inner_map.ptr, inner_map_size}
                );
            }
        } else {
            // Serialize primitive
            if (false
                == (serialize_key(schema_tree_node_id) && serialize_val(val, schema_tree_node_type)
                ))
            {
                failure = true;
                break;
            }
        }
    }

    if (failure) {
        m_schema_tree.revert();
        return false;
    }

    m_ir_buf.insert(
            m_ir_buf.cend(),
            m_schema_tree_node_buf.cbegin(),
            m_schema_tree_node_buf.cend()
    );
    m_ir_buf.insert(m_ir_buf.cend(), m_key_group_buf.cbegin(), m_key_group_buf.cend());
    m_ir_buf.insert(m_ir_buf.cend(), m_value_group_buf.cbegin(), m_value_group_buf.cend());
    return true;
}

template <typename encoded_variable_t>
auto Serializer<encoded_variable_t>::serialize_schema_tree_node(
        SchemaTree::NodeLocator const& locator
) -> bool {
    switch (locator.get_type()) {
        case SchemaTree::Node::Type::Int:
            m_schema_tree_node_buf.push_back(cProtocol::Payload::SchemaTreeNodeInt);
            break;
        case SchemaTree::Node::Type::Float:
            m_schema_tree_node_buf.push_back(cProtocol::Payload::SchemaTreeNodeFloat);
            break;
        case SchemaTree::Node::Type::Bool:
            m_schema_tree_node_buf.push_back(cProtocol::Payload::SchemaTreeNodeBool);
            break;
        case SchemaTree::Node::Type::Str:
            m_schema_tree_node_buf.push_back(cProtocol::Payload::SchemaTreeNodeStr);
            break;
        case SchemaTree::Node::Type::UnstructuredArray:
            m_schema_tree_node_buf.push_back(cProtocol::Payload::SchemaTreeNodeUnstructuredArray);
            break;
        case SchemaTree::Node::Type::Obj:
            m_schema_tree_node_buf.push_back(cProtocol::Payload::SchemaTreeNodeObj);
            break;
        default:
            // Unknown type
            return false;
    }

    if (false
        == encode_and_serialize_schema_tree_node_id<
                false,
                cProtocol::Payload::EncodedSchemaTreeNodeParentIdByte,
                cProtocol::Payload::EncodedSchemaTreeNodeParentIdShort,
                cProtocol::Payload::EncodedSchemaTreeNodeParentIdInt>(
                locator.get_parent_id(),
                m_schema_tree_node_buf
        ))
    {
        return false;
    }

    return serialize_string(locator.get_key_name(), m_schema_tree_node_buf);
}

template <typename encoded_variable_t>
auto Serializer<encoded_variable_t>::serialize_key(SchemaTree::Node::id_t id) -> bool {
    return encode_and_serialize_schema_tree_node_id<
            false,
            cProtocol::Payload::EncodedSchemaTreeNodeIdByte,
            cProtocol::Payload::EncodedSchemaTreeNodeIdShort,
            cProtocol::Payload::EncodedSchemaTreeNodeIdInt>(id, m_key_group_buf);
}

template <typename encoded_variable_t>
auto Serializer<encoded_variable_t>::serialize_val(
        msgpack::object const& val,
        SchemaTree::Node::Type schema_tree_node_type
) -> bool {
    switch (schema_tree_node_type) {
        case SchemaTree::Node::Type::Int:
            if (msgpack::type::POSITIVE_INTEGER == val.type
                && static_cast<uint64_t>(INT64_MAX) < val.as<uint64_t>())
            {
                return false;
            }
            serialize_value_int(val.as<int64_t>(), m_value_group_buf);
            break;

        case SchemaTree::Node::Type::Float:
            serialize_value_float(val.as<double>(), m_value_group_buf);
            break;

        case SchemaTree::Node::Type::Bool:
            serialize_value_bool(val.as<bool>(), m_value_group_buf);
            break;

        case SchemaTree::Node::Type::Str:
            if (false
                == serialize_value_string<encoded_variable_t>(
                        val.as<string_view>(),
                        m_logtype_buf,
                        m_value_group_buf
                ))
            {
                return false;
            }
            break;

        case SchemaTree::Node::Type::Obj:
            if (msgpack::type::NIL != val.type) {
                return false;
            }
            serialize_value_null(m_value_group_buf);
            break;

        case SchemaTree::Node::Type::UnstructuredArray:
            if (false
                == serialize_value_array<encoded_variable_t>(val, m_logtype_buf, m_value_group_buf))
            {
                return false;
            }
            break;

        default:
            // Unknown schema tree node type
            return false;
    }
    return true;
}

// Explicitly declare template specializations so that we can define the template methods in this
// file
template auto Serializer<eight_byte_encoded_variable_t>::create(
) -> OUTCOME_V2_NAMESPACE::std_result<Serializer<eight_byte_encoded_variable_t>>;
template auto Serializer<four_byte_encoded_variable_t>::create(
) -> OUTCOME_V2_NAMESPACE::std_result<Serializer<four_byte_encoded_variable_t>>;

template auto Serializer<eight_byte_encoded_variable_t>::change_utc_offset(UtcOffset utc_offset
) -> void;
template auto Serializer<four_byte_encoded_variable_t>::change_utc_offset(UtcOffset utc_offset
) -> void;

template auto Serializer<eight_byte_encoded_variable_t>::serialize_msgpack_map(
        msgpack::object_map const& msgpack_map
) -> bool;
template auto Serializer<four_byte_encoded_variable_t>::serialize_msgpack_map(
        msgpack::object_map const& msgpack_map
) -> bool;

template auto Serializer<eight_byte_encoded_variable_t>::serialize_schema_tree_node(
        SchemaTree::NodeLocator const& locator
) -> bool;
template auto Serializer<four_byte_encoded_variable_t>::serialize_schema_tree_node(
        SchemaTree::NodeLocator const& locator
) -> bool;

template auto Serializer<eight_byte_encoded_variable_t>::serialize_key(SchemaTree::Node::id_t id
) -> bool;
template auto Serializer<four_byte_encoded_variable_t>::serialize_key(SchemaTree::Node::id_t id
) -> bool;

template auto Serializer<eight_byte_encoded_variable_t>::serialize_val(
        msgpack::object const& val,
        SchemaTree::Node::Type schema_tree_node_type
) -> bool;
template auto Serializer<four_byte_encoded_variable_t>::serialize_val(
        msgpack::object const& val,
        SchemaTree::Node::Type schema_tree_node_type
) -> bool;
}  // namespace clp::ffi::ir_stream
