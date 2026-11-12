#ifndef CLP_S_SCHEMA_HPP
#define CLP_S_SCHEMA_HPP

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <utility>
#include <variant>
#include <vector>

#include <clp_s/Defs.hpp>
#include <clp_s/SchemaTree.hpp>
#include <clpp/Defs.hpp>

namespace clp_s {
struct UnorderedObject;

/**
 * A read-only view over a sequence of schema entries.
 *
 * Each top-level entry is either an MPT node ID or the delimiter of an unordered object. Iterating
 * the view yields each top-level entry as an `Entry`, with unordered objects decoded into
 * `UnorderedObject`s. `any_node_id` and `for_each_node_id` visit every MPT node ID at any nesting
 * depth in a single flat scan.
 */
class SchemaView {
public:
    // Types
    /**
     * A top-level entry of a view: a plain MPT node ID or a decoded unordered object.
     */
    using Entry = std::variant<mpt_node_id_t, UnorderedObject>;

    /**
     * Iterator over a view's top-level entries. Advancing past an unordered object skips its entire
     * content (metadata and sub-schema).
     */
    class EntryIterator {
    public:
        // Constructors
        EntryIterator(SchemaView const& view, size_t idx) : m_view{&view}, m_idx{idx} {}

        // Methods
        [[nodiscard]] auto operator*() const -> Entry;

        auto operator++() -> EntryIterator&;

        [[nodiscard]] auto operator==(EntryIterator const& rhs) const -> bool = default;

    private:
        // Data members
        SchemaView const* m_view;
        size_t m_idx;
    };

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
    [[nodiscard]] auto begin() const -> EntryIterator { return {*this, 0}; }

    [[nodiscard]] auto end() const -> EntryIterator { return {*this, m_schema.size()}; }

    /**
     * Invokes `predicate` on every MPT node ID in this view, including those within nested
     * unordered-object sub-schemas, until the predicate returns true.
     * @tparam include_object_roots Whether to also invoke `predicate` on the root node IDs stored
     * as unordered-object metadata.
     * @param predicate A callable accepting a `mpt_node_id_t` and returning a bool.
     * @return Whether the predicate returned true for any node ID.
     */
    template <bool include_object_roots = false, typename Predicate>
    [[nodiscard]] auto any_node_id(Predicate const& predicate) const -> bool;

    /**
     * Invokes `callback` on every plain MPT node ID in this view, including those within nested
     * unordered-object sub-schemas.
     * @param callback A callable accepting a single `mpt_node_id_t`.
     */
    template <typename Callback>
    auto for_each_node_id(Callback const& callback) const -> void {
        static_cast<void>(any_node_id([&](mpt_node_id_t node_id) -> bool {
            callback(node_id);
            return false;
        }));
    }

    /**
     * Finds the first MPT node ID in this view, descending into nested `Object` and
     * `StructurizedArray` sub-schemas. For `StructurizedText` and `ParentRule` objects the root
     * node ID stored in their metadata is returned.
     * @return The first MPT node ID.
     * @return std::nullopt if the view contains no node IDs.
     */
    [[nodiscard]] auto first_node_id() const -> std::optional<mpt_node_id_t>;

    /**
     * Collects every `StructurizedText` unordered object in this view, recursing into other
     * unordered objects (e.g. nested objects and structurized arrays) but not into
     * `StructurizedText` sub-schemas.
     *
     * A schema may contain several sibling `StructurizedText` objects when a log event has
     * multiple unstructured text fields.
     * @return A pair of the `StructurizedText` node ID and its log shape ID for each
     * StructurizedText object, in document order.
     */
    [[nodiscard]] auto find_structurized_texts() const
            -> std::vector<std::pair<mpt_node_id_t, clpp::log_shape_id_t>>;

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
 * - StructurizedText:
 *  - enclosing StructurizedText node's MPT node ID
 *  - log shape ID
 * - ParentRule:
 *  - enclosing ParentRule node's MPT node ID
 */
struct UnorderedObject {
    // Offsets of each metadata entry from the first entry after an unordered object's delimiter.
    static constexpr size_t cRootNodeIdMetadataOffset{0};
    static constexpr size_t cLogShapeIdMetadataOffset{1};

    /**
     * @return The number of leading metadata entries in an unordered object of the given type.
     */
    [[nodiscard]] static constexpr auto get_num_metadata_entries(NodeType type) -> size_t {
        switch (type) {
            case NodeType::StructurizedText:
                return 2;
            case NodeType::ParentRule:
                return 1;
            default:
                return 0;
        }
    }

    /**
     * @param type The unordered object's node type.
     * @return Whether an unordered object of the given type stores its MPT root node ID as
     * metadata.
     */
    [[nodiscard]] static constexpr auto has_root_node_id_metadata(NodeType type) -> bool {
        return 0 < get_num_metadata_entries(type);
    }

    NodeType type;
    // Total number of entries (metadata + sub-schema) this object occupies after its delimiter.
    size_t length;
    std::optional<mpt_node_id_t> node_id;
    std::optional<clpp::log_shape_id_t> log_shape_id;
    // The object's schema entries, excluding metadata entries.
    SchemaView sub_schema;
};

inline auto SchemaView::EntryIterator::operator*() const -> Entry {
    if (auto obj{m_view->decode_unordered_object(m_idx)}) {
        return *obj;
    }
    return m_view->m_schema[m_idx];
}

inline auto SchemaView::EntryIterator::operator++() -> EntryIterator& {
    auto const entry{m_view->m_schema[m_idx]};
    size_t const stride{
            is_unordered_object(entry) ? 1 + static_cast<size_t>(get_unordered_object_length(entry))
                                       : 1
    };
    m_idx = std::min(m_idx + stride, m_view->m_schema.size());
    return *this;
}

inline auto SchemaView::first_node_id() const -> std::optional<mpt_node_id_t> {
    std::optional<mpt_node_id_t> first_node_id;
    static_cast<void>(any_node_id<true>([&](mpt_node_id_t node_id) -> bool {
        first_node_id = node_id;
        return true;
    }));
    return first_node_id;
}

template <bool include_object_roots, typename Predicate>
auto SchemaView::any_node_id(Predicate const& predicate) const -> bool {
    for (size_t i{0}; i < m_schema.size(); ++i) {
        auto const entry{m_schema[i]};
        if (false == is_unordered_object(entry)) {
            if (predicate(entry)) {
                return true;
            }
            continue;
        }
        auto const type{get_unordered_object_type(entry)};
        if constexpr (include_object_roots) {
            auto const root_node_id_idx{i + 1 + UnorderedObject::cRootNodeIdMetadataOffset};
            if (UnorderedObject::has_root_node_id_metadata(type)
                && root_node_id_idx < m_schema.size() && predicate(m_schema[root_node_id_idx]))
            {
                return true;
            }
        }
        i += UnorderedObject::get_num_metadata_entries(type);
    }
    return false;
}

inline auto SchemaView::find_structurized_texts() const
        -> std::vector<std::pair<mpt_node_id_t, clpp::log_shape_id_t>> {
    std::vector<std::pair<mpt_node_id_t, clpp::log_shape_id_t>> texts;
    for (size_t i{0}; i < m_schema.size(); ++i) {
        auto const obj{decode_unordered_object(i)};
        if (false == obj.has_value()) {
            continue;
        }
        if (NodeType::StructurizedText == obj->type) {
            if (obj->node_id.has_value() && obj->log_shape_id.has_value()) {
                texts.emplace_back(obj->node_id.value(), obj->log_shape_id.value());
            }
            i += obj->length;
            continue;
        }
        i += UnorderedObject::get_num_metadata_entries(obj->type);
    }
    return texts;
}

inline auto SchemaView::decode_unordered_object(size_t i) const -> std::optional<UnorderedObject> {
    auto const entry{m_schema[i]};
    if (false == is_unordered_object(entry)) {
        return std::nullopt;
    }
    auto const type{get_unordered_object_type(entry)};
    auto const length{std::min(
            static_cast<size_t>(get_unordered_object_length(entry)),
            m_schema.size() - i - 1
    )};
    auto const content{m_schema.subspan(i + 1, length)};
    auto const num_metadata_entries{UnorderedObject::get_num_metadata_entries(type)};
    std::optional<mpt_node_id_t> node_id;
    std::optional<clpp::log_shape_id_t> log_shape_id;
    if (content.size() >= num_metadata_entries) {
        auto const metadata{content.first(num_metadata_entries)};
        if (UnorderedObject::cRootNodeIdMetadataOffset < metadata.size()) {
            node_id = metadata[UnorderedObject::cRootNodeIdMetadataOffset];
        }
        if (UnorderedObject::cLogShapeIdMetadataOffset < metadata.size()) {
            log_shape_id = static_cast<clpp::log_shape_id_t>(
                    metadata[UnorderedObject::cLogShapeIdMetadataOffset]
            );
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
    static constexpr mpt_node_id_t cMaxNodeId{0x00FF'FFFF};

    // Methods
    /**
     * Inserts a node into the ordered region of the schema.
     */
    auto insert_ordered(mpt_node_id_t node_id) -> void;

    /**
     * Inserts a node into the unordered region of the schema.
     */
    auto insert_unordered(mpt_node_id_t node_id) -> void;

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
    [[nodiscard]] auto get_ordered_schema_view() const -> std::span<mpt_node_id_t const> {
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
    [[nodiscard]] auto start_unordered_object(NodeType object_type, mpt_node_id_t node_id)
            -> size_t {
        insert_unordered(SchemaView::encode_node_type(object_type));
        auto const start_position{m_schema.size()};
        insert_unordered(node_id);
        return start_position;
    }

    /**
     * Starts a `NodeType::StructurizedText` unordered object, storing the MPT node ID of the
     * `StructurizedText` node and a reserved log shape ID (to be filled in by
     * `end_structurized_text`) as its metadata entries.
     *
     * The log shape ID must be filled in by calling `end_structurized_text`.
     * @param node_id The MPT node ID of the `StructurizedText` node.
     * @return The start position of the unordered object content.
     */
    [[nodiscard]] auto start_structurized_text(mpt_node_id_t node_id) -> size_t {
        auto const start_position{start_unordered_object(NodeType::StructurizedText, node_id)};
        insert_unordered(0);
        return start_position;
    }

    /**
     * Ends a `NodeType::StructurizedText` unordered object, writing the log shape ID into the
     * reserved entry created by `start_structurized_text`.
     * @param start_position The value returned by `start_structurized_text`.
     * @param log_shape_id
     */
    auto end_structurized_text(size_t start_position, clpp::log_shape_id_t log_shape_id) -> void {
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
