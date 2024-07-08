#include "Serializer.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <sstream>  // Should be removed
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include <boost-outcome/include/boost/outcome/std_result.hpp>
#include <json/single_include/nlohmann/json.hpp>
#include <msgpack.hpp>

#include "../../ir/types.hpp"
#include "../../time_types.hpp"
#include "../../type_utils.hpp"
#include "../encoding_methods.hpp"
#include "../SchemaTree.hpp"
#include "../SchemaTreeNode.hpp"
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
 * Class for iterating a msgpack map.
 */
class MsgpackMapIterator {
public:
    // Types
    using Child = msgpack::object_kv;

    // Constructors
    MsgpackMapIterator(SchemaTreeNode::id_t parent_id, Child* child_data, size_t child_data_length)
            : m_parent_id{parent_id},
              m_children{child_data, child_data_length} {}

    // Methods
    /**
     * @return The id of the parent in the schema tree.
     */
    [[nodiscard]] auto get_parent_id() const -> SchemaTreeNode::id_t { return m_parent_id; }

    /**
     * @return Whether it has more child to traverse.
     */
    [[nodiscard]] auto has_next_child() const -> bool {
        return m_children.size() > m_curr_child_idx;
    }

    /**
     * Gets the next child and advances the underlying child idx.
     * @return The next child to traverse.
     */
    [[nodiscard]] auto get_next_child() -> Child const& { return m_children[m_curr_child_idx++]; }

private:
    SchemaTreeNode::id_t m_parent_id;
    span<Child> m_children;
    size_t m_curr_child_idx{0};
};

/**
 * Gets the corresponded schema tree node type from the given msgpack  value.
 * @param val msgpack value.
 * @return The corresponded schema tree node type.
 * @return std::nullopt if the value doesn't match any of the supported schema tree node type.
 */
[[nodiscard]] auto get_schema_tree_node_type_from_msgpack_val(msgpack::object const& val
) -> optional<SchemaTreeNode::Type>;

/**
 * Serializes a value corresponded to an empty object.
 * @param buf Outputs the serialized byte sequence.
 */
auto serialize_empty_object(vector<int8_t>& buf) -> void;

/**
 * Serializes a value of integer type.
 * @param val
 * @param buf Outputs the serialized byte sequence.
 * @return Whether serialization succeeded.
 */
auto serialize_value_int(int64_t val, vector<int8_t>& buf) -> void;

/**
 * Serializes a value of float type.
 * @param val
 * @param buf Outputs the serialized byte sequence.
 */
auto serialize_value_float(double val, vector<int8_t>& buf) -> void;

/**
 * Serializes a value of bool.
 * @param val
 * @param buf Outputs the serialized byte sequence.
 */
auto serialize_value_bool(bool val, vector<int8_t>& buf) -> void;

/**
 * Serializes a value of NULL type.
 * @param buf Outputs the serialized byte sequence.
 */
auto serialize_value_null(vector<int8_t>& buf) -> void;

/**
 * Serializes a value of string type.
 * @tparam encoded_variable_t
 * @param val
 * @param logtype_buf
 * @param buf Outputs the serialized byte sequence.
 * @return Whether serialization succeeded.
 */
template <typename encoded_variable_t>
[[nodiscard]] auto
serialize_value_string(string_view val, string& logtype_buf, vector<int8_t>& buf) -> bool;

/**
 * Serializes a msgpack array as a JSON string, using clp encoding.
 * @tparam encoded_variable_t
 * @param val
 * @param logtype_buf
 * @param buf Outputs the serialized byte sequence.
 * @return Whether serialization succeeded.
 */
template <typename encoded_variable_t>
[[nodiscard]] auto
serialize_value_array(msgpack::object const& val, string& logtype_buf, vector<int8_t>& buf) -> bool;

auto get_schema_tree_node_type_from_msgpack_val(msgpack::object const& val
) -> optional<SchemaTreeNode::Type> {
    optional<SchemaTreeNode::Type> ret_val;
    switch (val.type) {
        case msgpack::type::POSITIVE_INTEGER:
        case msgpack::type::NEGATIVE_INTEGER:
            ret_val.emplace(SchemaTreeNode::Type::Int);
            break;
        case msgpack::type::FLOAT32:
        case msgpack::type::FLOAT64:
            ret_val.emplace(SchemaTreeNode::Type::Float);
            break;
        case msgpack::type::STR:
            ret_val.emplace(SchemaTreeNode::Type::Str);
            break;
        case msgpack::type::BOOLEAN:
            ret_val.emplace(SchemaTreeNode::Type::Bool);
            break;
        case msgpack::type::NIL:
        case msgpack::type::MAP:
            ret_val.emplace(SchemaTreeNode::Type::Obj);
            break;
        case msgpack::type::ARRAY:
            ret_val.emplace(SchemaTreeNode::Type::UnstructuredArray);
            break;
        default:
            return std::nullopt;
    }
    return ret_val;
}

auto serialize_empty_object(vector<int8_t>& buf) -> void {
    buf.push_back(cProtocol::Payload::ValueEmpty);
}

auto serialize_value_int(int64_t val, vector<int8_t>& buf) -> void {
    if (INT8_MIN <= val && val <= INT8_MAX) {
        buf.push_back(cProtocol::Payload::ValueInt8);
        buf.push_back(static_cast<int8_t>(val));
    } else if (INT16_MIN <= val && val <= INT16_MAX) {
        buf.push_back(cProtocol::Payload::ValueInt16);
        serialize_int(static_cast<int16_t>(val), buf);
    } else if (INT32_MIN <= val && val <= INT32_MAX) {
        buf.push_back(cProtocol::Payload::ValueInt32);
        serialize_int(static_cast<int32_t>(val), buf);
    } else {  // (INT64_MIN <= val && val <= INT64_MAX)
        buf.push_back(cProtocol::Payload::ValueInt64);
        serialize_int(val, buf);
    }
}

auto serialize_value_float(double val, vector<int8_t>& buf) -> void {
    buf.push_back(cProtocol::Payload::ValueFloat);
    serialize_int(bit_cast<uint64_t>(val), buf);
}

auto serialize_value_bool(bool val, vector<int8_t>& buf) -> void {
    buf.push_back(val ? cProtocol::Payload::ValueTrue : cProtocol::Payload::ValueFalse);
}

auto serialize_value_null(vector<int8_t>& buf) -> void {
    buf.push_back(cProtocol::Payload::ValueNull);
}

template <typename encoded_variable_t>
auto serialize_value_string(string_view val, string& logtype_buf, vector<int8_t>& buf) -> bool {
    if (string_view::npos == val.find(' ')) {
        return serialize_string(val, buf);
    }
    logtype_buf.clear();
    return serialize_clp_string<encoded_variable_t>(val, logtype_buf, buf);
}

template <typename encoded_variable_t>
auto serialize_value_array(msgpack::object const& val, string& logtype_buf, vector<int8_t>& buf)
        -> bool {
    std::ostringstream oss;
    oss << val;
    logtype_buf.clear();
    return serialize_clp_string<encoded_variable_t>(oss.str(), logtype_buf, buf);
}
}  // namespace

template <typename encoded_variable_t>
auto Serializer<encoded_variable_t>::create(
) -> BOOST_OUTCOME_V2_NAMESPACE::std_result<Serializer<encoded_variable_t>> {
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
        serialize_empty_object(m_ir_buf);
        return true;
    }

    m_schema_tree.take_snapshot();
    m_schema_tree_node_buf.clear();
    m_key_group_buf.clear();
    m_value_group_buf.clear();

    // Traverse the map from the root using DFS iteratively.
    bool failure{false};
    vector<MsgpackMapIterator> working_stack;
    working_stack.emplace_back(
            SchemaTree::cRootId,
            msgpack_map.ptr,
            static_cast<size_t>(msgpack_map.size)
    );
    while (false == working_stack.empty()) {
        auto& curr{working_stack.back()};
        if (false == curr.has_next_child()) {
            // All child has been visited. Pop it out from the working stack
            working_stack.pop_back();
            continue;
        }

        // Convert the type of the current value to its corresponded schema tree node type
        auto const& [key, val]{curr.get_next_child()};
        auto const opt_schema_tree_node_type{get_schema_tree_node_type_from_msgpack_val(val)};
        if (false == opt_schema_tree_node_type.has_value()) {
            failure = true;
            break;
        }
        auto const schema_tree_node_type{opt_schema_tree_node_type.value()};

        SchemaTree::NodeLocator const locator{
                curr.get_parent_id(),
                key.as<string_view>(),
                schema_tree_node_type
        };

        // Get the corresponded node in the schema tree. If the node doesn't exist yet, create one
        auto opt_schema_tree_node_id{m_schema_tree.try_get_node_id(locator)};
        if (false == opt_schema_tree_node_id.has_value()) {
            opt_schema_tree_node_id.emplace(m_schema_tree.insert_node(locator));
            if (false == serialize_schema_tree_node(locator)) {
                failure = true;
                break;
            }
        }
        auto const schema_tree_node_id{opt_schema_tree_node_id.value()};

        if (SchemaTreeNode::Type::Obj == schema_tree_node_type && msgpack::type::MAP == val.type) {
            // Serialize sub-maps. If the sub-map is not empty, push an iterator of the sub-map into
            // the working stack to continue DFS
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
            auto const& inner_map{val.via.map};
            auto const inner_map_size(static_cast<size_t>(inner_map.size));
            if (0 == inner_map_size) {
                if (false == serialize_key(schema_tree_node_id)) {
                    failure = true;
                    break;
                }
                serialize_empty_object(m_value_group_buf);
            } else {
                working_stack.emplace_back(schema_tree_node_id, inner_map.ptr, inner_map_size);
            }
        } else {
            // A primitive value is reached. Directly serialize its key and value into the IR stream
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
        case SchemaTreeNode::Type::Int:
            m_schema_tree_node_buf.push_back(cProtocol::Payload::SchemaTreeNodeInt);
            break;
        case SchemaTreeNode::Type::Float:
            m_schema_tree_node_buf.push_back(cProtocol::Payload::SchemaTreeNodeFloat);
            break;
        case SchemaTreeNode::Type::Bool:
            m_schema_tree_node_buf.push_back(cProtocol::Payload::SchemaTreeNodeBool);
            break;
        case SchemaTreeNode::Type::Str:
            m_schema_tree_node_buf.push_back(cProtocol::Payload::SchemaTreeNodeStr);
            break;
        case SchemaTreeNode::Type::UnstructuredArray:
            m_schema_tree_node_buf.push_back(cProtocol::Payload::SchemaTreeNodeUnstructuredArray);
            break;
        case SchemaTreeNode::Type::Obj:
            m_schema_tree_node_buf.push_back(cProtocol::Payload::SchemaTreeNodeObj);
            break;
        default:
            // Unknown type
            return false;
    }

    auto const parent_id{locator.get_parent_id()};
    if (parent_id <= UINT8_MAX) {
        m_schema_tree_node_buf.push_back(cProtocol::Payload::SchemaTreeNodeParentIdUByte);
        m_schema_tree_node_buf.push_back(static_cast<int8_t>(static_cast<uint8_t>(parent_id)));
    } else if (parent_id <= UINT16_MAX) {
        m_schema_tree_node_buf.push_back(cProtocol::Payload::SchemaTreeNodeParentIdUShort);
        serialize_int(static_cast<uint16_t>(parent_id), m_schema_tree_node_buf);
    } else {
        // Out of range
        return false;
    }

    return serialize_string(locator.get_key_name(), m_schema_tree_node_buf);
}

template <typename encoded_variable_t>
auto Serializer<encoded_variable_t>::serialize_key(SchemaTreeNode::id_t id) -> bool {
    if (id <= UINT8_MAX) {
        m_key_group_buf.push_back(cProtocol::Payload::KeyIdUByte);
        m_key_group_buf.push_back(bit_cast<int8_t>(static_cast<uint8_t>(id)));
    } else if (id <= UINT16_MAX) {
        m_key_group_buf.push_back(cProtocol::Payload::KeyIdUShort);
        serialize_int(static_cast<uint16_t>(id), m_key_group_buf);
    } else {
        return false;
    }
    return true;
}

template <typename encoded_variable_t>
auto Serializer<encoded_variable_t>::serialize_val(
        msgpack::object const& val,
        SchemaTreeNode::Type type
) -> bool {
    switch (type) {
        case SchemaTreeNode::Type::Int:
            if (msgpack::type::POSITIVE_INTEGER == val.type
                && static_cast<uint64_t>(INT64_MAX) < val.as<uint64_t>())
            {
                return false;
            }
            serialize_value_int(val.as<int64_t>(), m_value_group_buf);
            break;

        case SchemaTreeNode::Type::Float:
            serialize_value_float(val.as<double>(), m_value_group_buf);
            break;

        case SchemaTreeNode::Type::Bool:
            serialize_value_bool(val.as<bool>(), m_value_group_buf);
            break;

        case SchemaTreeNode::Type::Str:
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

        case SchemaTreeNode::Type::Obj:
            if (msgpack::type::NIL != val.type) {
                return false;
            }
            serialize_value_null(m_value_group_buf);
            break;

        case SchemaTreeNode::Type::UnstructuredArray:
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
) -> BOOST_OUTCOME_V2_NAMESPACE::std_result<Serializer<eight_byte_encoded_variable_t>>;
template auto Serializer<four_byte_encoded_variable_t>::create(
) -> BOOST_OUTCOME_V2_NAMESPACE::std_result<Serializer<four_byte_encoded_variable_t>>;
template auto Serializer<eight_byte_encoded_variable_t>::change_utc_offset(UtcOffset utc_offset
) -> void;
template auto Serializer<four_byte_encoded_variable_t>::change_utc_offset(UtcOffset utc_offset
) -> void;
template auto Serializer<four_byte_encoded_variable_t>::serialize_msgpack_map(
        msgpack::object_map const& msgpack_map
) -> bool;
template auto Serializer<eight_byte_encoded_variable_t>::serialize_msgpack_map(
        msgpack::object_map const& msgpack_map
) -> bool;
template auto Serializer<four_byte_encoded_variable_t>::serialize_schema_tree_node(
        SchemaTree::NodeLocator const& locator
) -> bool;
template auto Serializer<eight_byte_encoded_variable_t>::serialize_schema_tree_node(
        SchemaTree::NodeLocator const& locator
) -> bool;
template auto Serializer<four_byte_encoded_variable_t>::serialize_key(SchemaTreeNode::id_t id
) -> bool;
template auto Serializer<eight_byte_encoded_variable_t>::serialize_key(SchemaTreeNode::id_t id
) -> bool;
template auto Serializer<four_byte_encoded_variable_t>::serialize_val(
        msgpack::object const& val,
        SchemaTreeNode::Type type
) -> bool;
template auto Serializer<eight_byte_encoded_variable_t>::serialize_val(
        msgpack::object const& val,
        SchemaTreeNode::Type type
) -> bool;
}  // namespace clp::ffi::ir_stream
