#ifndef CLP_S_SCHEMA_HPP
#define CLP_S_SCHEMA_HPP

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <utility>
#include <vector>

#include <clp_s/SchemaTree.hpp>
#include <clpp/Defs.hpp>

namespace clp_s {
struct UnorderedObject;

/**
 * A read-only view over a sequence of schema entries.
 *
 * Each top-level entry is either an MPT node ID or the delimiter of an unordered object.
 * `visit_entries` iterates these entries and decodes unordered objects into `UnorderedObject`s.
 * `any_node_id` and `for_each_node_id` visit every MPT node ID, recursing into unordered-object
 * sub-schemas.
 */
class SchemaView {
public:
    // Static methods
    /**
     * Encodes a NodeType as a schema entry to delimit an unordered object using bithacks.
     * @param node_type
     * @return The NodeType encoded as a schema entry
     */
    static constexpr auto encode_node_type(NodeType node_type) -> int32_t {
        return static_cast<int32_t>(node_type) << cEncodedTypeOffset;
    }

    // Constructors
    explicit SchemaView(std::span<int32_t const> schema) : m_schema{schema} {}

    // Methods
    /**
     * Invokes `predicate` on every MPT node ID in this view, recursing into unordered object
     * sub-schemas, until the predicate returns true.
     * @param predicate A callable accepting a `SchemaNode::id_t` and returning a bool.
     * @return Whether the predicate returned true for any node ID.
     */
    template <typename Predicate>
    [[nodiscard]] auto any_node_id(Predicate const& predicate) const -> bool;

    /**
     * Invokes `callback` on every MPT node ID in this view, recursing into unordered-object
     * sub-schemas.
     * @param callback A callable accepting a single `SchemaNode::id_t`.
     */
    template <typename Callback>
    auto for_each_node_id(Callback const& callback) const -> void {
        static_cast<void>(any_node_id([&](SchemaNode::id_t node_id) -> bool {
            callback(node_id);
            return false;
        }));
    }

    /**
     * Visits each entry of this view, stopping early if a visitor returns true.
     *
     * Plain MPT node IDs are passed to `visit_node`. Delimiter entries are decoded into
     * `UnorderedObject`s and passed to `visit_object`, which is responsible for recursing into
     * the sub-schema if desired.
     * @param visit_node Invoked with each plain MPT node ID.
     * @param visit_object Invoked with each decoded unordered object.
     * @return Whether any visitor returned true.
     */
    template <typename NodeVisitor, typename ObjectVisitor>
    auto visit_entries(NodeVisitor const& visit_node, ObjectVisitor const& visit_object) const
            -> bool;

    /**
     * Collects every `LogMessage` unordered object in this view, recursing into other unordered
     * objects (e.g. nested objects and structured arrays) but not into `LogMessage` sub-schemas.
     *
     * A schema may contain several sibling `LogMessage` objects when a log event has multiple
     * unstructured text fields.
     * @return A pair of the `LogMessage` node ID and its log shape ID for each LogMessage
     * object, in document order.
     */
    [[nodiscard]] auto find_log_messages() const
            -> std::vector<std::pair<SchemaNode::id_t, clpp::log_shape_id_t>>;

private:
    // Static constants
    static constexpr size_t cEncodedTypeOffset{(sizeof(int32_t) - 1) * 8};
    static constexpr uint32_t cEncodedTypeBitmask{0xFF00'0000};
    static constexpr int32_t cEncodedTypeLengthBitmask{~static_cast<int32_t>(cEncodedTypeBitmask)};

    // Static methods
    /**
     * @param schema_entry
     * @return Whether the schema entry is an unordered-object delimiter.
     */
    static constexpr auto is_unordered_object(int32_t schema_entry) -> bool {
        return 0 != (schema_entry & cEncodedTypeBitmask);
    }

    /**
     * Extracts the unordered object length from an unordered object delimiter.
     * @param schema_entry
     * @return The extracted object length
     */
    static constexpr auto get_unordered_object_length(int32_t schema_entry) -> int32_t {
        return schema_entry & cEncodedTypeLengthBitmask;
    }

    /**
     * Extracts the NodeType from an unordered object delimiter.
     * @param schema_entry
     * @return The extracted NodeType
     */
    static constexpr auto get_unordered_object_type(int32_t schema_entry) -> NodeType {
        return static_cast<NodeType>(static_cast<uint32_t>(schema_entry) >> cEncodedTypeOffset);
    }

    // Methods
    /**
     * Decodes the unordered object delimited at entry `i`, if any.
     * @param i The index of the potential delimiter.
     * @return The decoded unordered object, or std::nullopt if entry `i` is a plain node ID.
     */
    [[nodiscard]] auto decode_unordered_object(size_t i) const -> std::optional<UnorderedObject>;

    // Data members
    std::span<int32_t const> m_schema;
};

/**
 * An unordered object is stored in a schema as a delimiter entry followed by any metadata entries
 * and the object's sub-schema.
 *
 * Metadata for each node type:
 * - LogMessage:
 *  - enclosing LogMessage node's MPT node ID
 *  - log shape ID
 * - ParentRule:
 *  - enclosing ParentRule node's MPT node ID
 */
struct UnorderedObject {
    // Static methods
    /**
     * @return The number of leading metadata entries in an unordered object of the given type.
     */
    [[nodiscard]] static constexpr auto get_num_metadata_entries(NodeType type) -> size_t {
        switch (type) {
            case NodeType::LogMessage:
                return 2;
            case NodeType::ParentRule:
                return 1;
            default:
                return 0;
        }
    }

    // Data members
    NodeType type;
    // Total number of entries (metadata + sub-schema) this object occupies after its delimiter.
    size_t length;
    std::optional<SchemaNode::id_t> node_id;
    std::optional<clpp::log_shape_id_t> log_shape_id;
    // The object's schema entries, excluding metadata entries.
    SchemaView sub_schema;
};

template <typename Predicate>
auto SchemaView::any_node_id(Predicate const& predicate) const -> bool {
    return visit_entries(predicate, [&](UnorderedObject const& obj) -> bool {
        return obj.sub_schema.any_node_id(predicate);
    });
}

template <typename NodeVisitor, typename ObjectVisitor>
auto
SchemaView::visit_entries(NodeVisitor const& visit_node, ObjectVisitor const& visit_object) const
        -> bool {
    for (size_t i{0}; i < m_schema.size(); ++i) {
        if (auto const obj{decode_unordered_object(i)}) {
            if (visit_object(*obj)) {
                return true;
            }
            i += obj->length;
            continue;
        }
        if (visit_node(m_schema[i])) {
            return true;
        }
    }
    return false;
}

inline auto SchemaView::find_log_messages() const
        -> std::vector<std::pair<SchemaNode::id_t, clpp::log_shape_id_t>> {
    std::vector<std::pair<SchemaNode::id_t, clpp::log_shape_id_t>> msgs;
    visit_entries(
            [](SchemaNode::id_t) -> bool { return false; },
            [&](UnorderedObject const& obj) -> bool {
                if (NodeType::LogMessage == obj.type) {
                    if (obj.node_id.has_value() && obj.log_shape_id.has_value()) {
                        msgs.emplace_back(obj.node_id.value(), obj.log_shape_id.value());
                    }
                    return false;
                }
                auto const nested_msgs{obj.sub_schema.find_log_messages()};
                msgs.insert(msgs.end(), nested_msgs.begin(), nested_msgs.end());
                return false;
            }
    );
    return msgs;
}

inline auto SchemaView::decode_unordered_object(size_t i) const -> std::optional<UnorderedObject> {
    auto const entry{m_schema[i]};
    if (false == is_unordered_object(entry)) {
        return std::nullopt;
    }
    auto const type{get_unordered_object_type(entry)};
    auto const length{static_cast<size_t>(get_unordered_object_length(entry))};
    auto const content{m_schema.subspan(i + 1, length)};
    auto const num_metadata_entries{UnorderedObject::get_num_metadata_entries(type)};
    std::optional<SchemaNode::id_t> node_id;
    std::optional<clpp::log_shape_id_t> log_shape_id;
    if (content.size() >= num_metadata_entries) {
        auto const metadata{content.first(num_metadata_entries)};
        if (false == metadata.empty()) {
            node_id = metadata[0];
        }
        if (metadata.size() >= 2) {
            log_shape_id = static_cast<clpp::log_shape_id_t>(metadata[1]);
        }
    }
    return UnorderedObject{
            .type = type,
            .length = length,
            .node_id = node_id,
            .log_shape_id = log_shape_id,
            .sub_schema
            = SchemaView{content.subspan(std::min(num_metadata_entries, content.size()))}
    };
}

/**
 * Class representing a schema made up of MST nodes.
 *
 * Internally, the schema is represented by a vector where the first m_num_ordered entries are
 * ordered by MST node ID, and the following entries are allowed to have arbitrary order.
 *
 * In the current implementation of clp-s, MST node IDs must be unique in the ordered region of a
 * schema, but can be repeated in the unordered region. The caller is responsible for not inserting
 * duplicate MST nodes into the ordered region of a schema.
 */
class Schema {
public:
    // Static constants
    // The maximum valid MST node ID that can be stored in a schema, as the top byte encodes an
    // unordered object's type.
    static constexpr SchemaNode::id_t cMaxNodeId{0x00FF'FFFF};

    // Methods
    /**
     * Inserts a node into the ordered region of the schema.
     */
    auto insert_ordered(SchemaNode::id_t node_id) -> void;

    /**
     * Inserts a node into the unordered region of the schema.
     */
    auto insert_unordered(SchemaNode::id_t node_id) -> void;

    /**
     * Inserts another schema into the unordered region of the schema, maintaining that Schema's
     * order.
     */
    auto insert_unordered(Schema const& schema) -> void;

    /**
     * Clear the Schema object so that it can be reused without reallocating the underlying vector.
     */
    auto clear() -> void {
        m_schema.clear();
        m_num_ordered = 0;
    }

    /**
     * Sets the number of ordered nodes present in the schema. This method is used during
     * decompression to help initialize this object.
     * @param num_ordered
     */
    auto set_num_ordered(size_t num_ordered) -> void { m_num_ordered = num_ordered; }

    /**
     * @return the number of ordered elements in the underlying schema
     */
    [[nodiscard]] auto get_num_ordered() const -> size_t { return m_num_ordered; }

    /**
     * @return the number of elements in the underlying schema
     */
    [[nodiscard]] auto size() const -> size_t { return m_schema.size(); }

    /**
     * @return constant iterator to the start of the underlying schema
     */
    [[nodiscard]] auto begin() const { return m_schema.cbegin(); }

    /**
     * @return constant iterator to the end of the underlying schema
     */
    [[nodiscard]] auto end() const { return m_schema.cend(); }

    /**
     * @return a view into the underlying schema
     */
    [[nodiscard]] auto get_view() const -> SchemaView { return SchemaView{m_schema}; }

    /**
     * @return A view into the ordered region of the underlying schema.
     */
    [[nodiscard]] auto get_ordered_schema_view() const -> std::span<SchemaNode::id_t const> {
        return {m_schema.data(), m_num_ordered};
    }

    /**
     * @return A view into the unordered region of the underlying schema.
     */
    [[nodiscard]] auto get_unordered_schema_view() const -> SchemaView {
        return SchemaView{std::span<int32_t const>{m_schema}.subspan(m_num_ordered)};
    }

    /**
     * Resizes the internal schema vector to match the given length.
     * @param size
     */
    auto resize(size_t size) -> void { m_schema.resize(size); }

    /**
     * @return mutable pointer to the underlying schema storage
     */
    [[nodiscard]] auto data() -> int32_t* { return m_schema.data(); }

    /**
     * Less than comparison operator so that Schema can act as a key for SchemaMap
     * @return true if this schema is less than the schema on the right hand side
     * @return false otherwise
     */
    auto operator<(Schema const& rhs) const -> bool { return m_schema < rhs.m_schema; }

    /**
     * Equal to comparison operator so that Schema can act as a key for SchemaMap
     * @return true if this schema is equal to the schema on the right hand side
     * @return false otherwise
     */
    auto operator==(Schema const& rhs) const -> bool { return m_schema == rhs.m_schema; }

    /**
     * Starts an unordered object of a given NodeType.
     *
     * Unordered objects must be closed by calling the `end_unordered_object` method with the start
     * position returned by this method.
     * @param object_type
     * @return the start position of the unordered object
     */
    [[nodiscard]] auto start_unordered_object(NodeType object_type) -> size_t {
        insert_unordered(SchemaView::encode_node_type(object_type));
        return m_schema.size();
    }

    /**
     * Starts an unordered object of a given NodeType, storing the MPT node ID as the first
     * entry.
     *
     * Unordered objects must be closed by calling the `end_unordered_object` method with the start
     * position returned by this method.
     * @param object_type
     * @return the start position of the unordered object
     */
    [[nodiscard]] auto start_unordered_object(NodeType object_type, SchemaNode::id_t node_id)
            -> size_t {
        insert_unordered(SchemaView::encode_node_type(object_type));
        auto const start_position{m_schema.size()};
        insert_unordered(node_id);
        return start_position;
    }

    /**
     * Starts a `NodeType::LogMessage` unordered object, storing the MPT node ID of the `LogMessage`
     * node and a reserved log shape ID (to be filled in by `end_log_message`) as its metadata
     * entries.
     *
     * The log shape ID must be filled in by calling `end_log_message`.
     * @param node_id The MPT node ID of the `LogMessage` node.
     * @return The start position of the unordered object content.
     */
    [[nodiscard]] auto start_log_message(SchemaNode::id_t node_id) -> size_t {
        auto const start_position{start_unordered_object(NodeType::LogMessage, node_id)};
        insert_unordered(0);
        return start_position;
    }

    /**
     * Ends a `NodeType::LogMessage` unordered object, writing the log shape ID into the reserved
     * entry created by `start_log_message`.
     * @param start_position The value returned by `start_log_message`.
     * @param log_shape_id
     */
    auto end_log_message(size_t start_position, clpp::log_shape_id_t log_shape_id) -> void {
        m_schema.at(start_position + 1) = static_cast<int32_t>(log_shape_id);
        end_unordered_object(start_position);
    }

    /**
     * Ends an unordered object which was started by calling `start_unordered_object`.
     * @param start_position
     */
    auto end_unordered_object(size_t start_position) -> void {
        m_schema.at(start_position - 1) |= static_cast<int32_t>(m_schema.size() - start_position);
    }

private:
    // Data members
    std::vector<int32_t> m_schema;
    size_t m_num_ordered{0};
};
}  // namespace clp_s

#endif  // CLP_S_SCHEMA_HPP
