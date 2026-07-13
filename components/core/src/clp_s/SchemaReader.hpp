#ifndef CLP_S_SCHEMAREADER_HPP
#define CLP_S_SCHEMAREADER_HPP

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include <absl/container/flat_hash_set.h>
#include <ystdlib/error_handling/Result.hpp>

#include <clp_s/ErrorCode.hpp>
#include <clp_s/TraceableException.hpp>
#include <clpp/Defs.hpp>
#include <clpp/ErrorCode.hpp>
#include <clpp/ParentRuleShapes.hpp>

#include "ColumnReader.hpp"
#include "DictionaryReader.hpp"
#include "FileReader.hpp"
#include "JsonSerializer.hpp"
#include "Schema.hpp"
#include "SchemaTree.hpp"
#include "search/Projection.hpp"
#include "ZstdDecompressor.hpp"

namespace clp_s {
class SchemaReader;

class FilterClass {
public:
    /**
     * Initializes the filter
     * @param reader
     * @param column_readers
     */
    virtual void init(SchemaReader* reader, std::vector<BaseColumnReader*> const& column_readers)
            = 0;

    /**
     * Initializes the filter with a column map.
     * Note: the column map only contains the ordered columns in a schema.
     * @param reader
     * @param column_map
     */
    virtual void
    init(SchemaReader* reader, std::unordered_map<int32_t, BaseColumnReader*> const& column_map) {}

    /**
     * Filters the message
     * @param cur_message
     * @return true if the message is accepted
     */
    virtual bool filter(uint64_t cur_message) = 0;
};

class SchemaReader {
public:
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}
    };

    /**
     * Metadata describing one schema table entry.
     *
     * Fields (except `num_messages`) are stored as `size_t` instead of their serialized type
     * `uint64_t` because they are passed to buffer and index APIs that operate on pointer sized
     * offsets. Deserialized metadata must therefore be converted before constructing this object.
     */
    class SchemaMetadata {
    public:
        // Constructors
        SchemaMetadata() = default;

        /**
         * Constructs metadata without `uncompressed_size`, which is derived later after reading
         * neighboring schema entries.
         *
         * @param stream_id
         * @param stream_offset
         * @param num_messages
         */
        SchemaMetadata(size_t stream_id, size_t stream_offset, uint64_t num_messages)
                : m_stream_id{stream_id},
                  m_stream_offset{stream_offset},
                  m_num_messages{num_messages} {}

        // Methods
        [[nodiscard]] auto stream_id() const -> size_t { return m_stream_id; }

        [[nodiscard]] auto stream_offset() const -> size_t { return m_stream_offset; }

        [[nodiscard]] auto num_messages() const -> uint64_t { return m_num_messages; }

        [[nodiscard]] auto uncompressed_size() const -> size_t { return m_uncompressed_size; }

        auto set_uncompressed_size(size_t uncompressed_size) -> void {
            m_uncompressed_size = uncompressed_size;
        }

    private:
        // Members
        size_t m_stream_id{0};
        size_t m_stream_offset{0};
        uint64_t m_num_messages{0};
        size_t m_uncompressed_size{0};
    };

    // Constructor
    SchemaReader() = default;

    // Destructor
    ~SchemaReader() { delete_columns(); }

    void delete_columns() {
        for (auto& i : m_columns) {
            delete i;
        }
    }

    /**
     * Resets the contents of this SchemaReader and prepares it to become a SchemaReader with a new
     * schema id, schema tree, and other parameters. After this call the SchemaReader is prepared
     * to accept append_column calls for the new schema.
     *
     * @param schema_tree
     * @param projection
     * @param schema_id
     * @param ordered_schema
     * @param num_messages
     * @param should_marshal_records
     * @param parent_rule_shapes
     */
    void reset(
            std::shared_ptr<SchemaTree> schema_tree,
            std::shared_ptr<search::Projection> projection,
            int32_t schema_id,
            std::span<int32_t> ordered_schema,
            uint64_t num_messages,
            bool should_marshal_records,
            LogShapeDictionaryReader const* log_shape_dict,
            clpp::ParentRuleShapesArray const* parent_rule_shapes
    ) {
        m_schema_id = schema_id;
        m_num_messages = num_messages;
        m_cur_message = 0;
        m_serializer_initialized = false;
        m_ordered_schema = ordered_schema;
        delete_columns();
        m_column_map.clear();
        m_columns.clear();
        m_reordered_columns.clear();
        m_timestamp_column = nullptr;
        m_get_timestamp = []() -> epochtime_t { return 0; };
        m_log_event_idx_column = nullptr;
        m_local_id_to_global_id.clear();
        m_global_id_to_local_id.clear();
        m_global_id_to_unordered_object.clear();
        m_local_schema_tree.clear();
        m_json_serializer.clear();
        m_reconstruction_targets.clear();
        m_global_schema_tree = std::move(schema_tree);
        m_projection = std::move(projection);
        m_should_marshal_records = should_marshal_records;
        m_log_shape_dict = log_shape_dict;
        m_parent_rule_shapes = parent_rule_shapes;
    }

    /**
     * Appends a column to the schema reader
     * @param column_reader
     */
    void append_column(BaseColumnReader* column_reader);

    /**
     * Appends an unordered column to the schema reader
     * @param column_reader
     */
    void append_unordered_column(BaseColumnReader* column_reader);

    size_t get_column_size() { return m_columns.size(); }

    /**
     * Marks an unordered object for the purpose of marshalling records.
     * @param column_reader_start,
     * @param mst_subtree_root,
     * @param schema
     */
    void mark_unordered_object(
            size_t column_reader_start,
            int32_t mst_subtree_root,
            std::span<int32_t> schema
    );

    /**
     * Loads the encoded messages from a shared buffer starting at a given offset
     * @param stream_buffer
     * @param offset
     * @param uncompressed_size
     */
    void load(std::shared_ptr<char[]> stream_buffer, size_t offset, size_t uncompressed_size);

    /**
     * @return the number of messages in the schema
     */
    uint64_t get_num_messages() const { return m_num_messages; }

    /**
     * Generates a JSON string from the encoded columns
     * @param message_index The index of the message to generate the JSON string for.
     * @return The generated JSON string
     */
    [[nodiscard]] auto generate_json_string(uint64_t message_index) -> std::string;

    /**
     * Gets the next message
     * @param message
     * @return true if there is a next message
     */
    bool get_next_message(std::string& message);

    /**
     * Gets the next message matching a filter
     * @param message
     * @param filter
     * @return true if there is a next message
     */
    bool get_next_message(std::string& message, FilterClass& filter);

    /**
     * Gets the next message as well as its timestamp and log event index.
     * @param message
     * @param timestamp
     * @param log_event_idx
     * @return true if there is a next message
     */
    bool get_next_message_with_metadata(
            std::string& message,
            epochtime_t& timestamp,
            int64_t& log_event_idx
    );

    /**
     * Gets the next message matching a filter as well as its timestamp and log event index.
     * @param message
     * @param timestamp
     * @param log_event_idx
     * @param filter
     * @return true if there is a next message
     */
    bool get_next_message_with_metadata(
            std::string& message,
            epochtime_t& timestamp,
            int64_t& log_event_idx,
            FilterClass& filter
    );

    /**
     * Initializes the filter
     * @param filter
     */
    void initialize_filter(FilterClass& filter);

    /**
     * Initializes the filter with a column map.
     * Note: the column map only contains the ordered columns in a schema.
     * @param filter
     */
    void initialize_filter_with_column_map(FilterClass& filter);

    /**
     * Initializes all internal data structures required to serialize records.
     * @return A void result on success, or an error code indicating the failure:
     * - ClppErrorCodeEnum::Corrupt if a `ParentRule` scope in a schema cannot be resolved to a
     *   schema-tree node, which indicates a corrupt or inconsistent archive.
     */
    [[nodiscard]] auto initialize_serializer() -> ystdlib::error_handling::Result<void>;

    /**
     * Marks a column as timestamp
     * @param column_reader
     */
    void mark_column_as_timestamp(BaseColumnReader* column_reader);

    /**
     * Marks a column as the log_event_idx column.
     */
    void mark_column_as_log_event_idx(BaseColumnReader* column_reader) {
        m_log_event_idx_column = column_reader;
    }

    int32_t get_schema_id() const { return m_schema_id; }

    /**
     * @param schema
     * @return the first column ID found in the given schema, or -1 if the schema contains no
     * columns
     */
    static int32_t get_first_column_in_span(std::span<int32_t> schema);

    /**
     * @return the timestamp found in the row pointed to by m_cur_message
     */
    epochtime_t get_next_timestamp() const { return m_get_timestamp(); }

    /**
     * @return the log_event_idx in the row pointed to by m_cur_message or 0 if there is no
     * log_event_idx in this table.
     */
    int64_t get_next_log_event_idx() const;

    /**
     * @return true if all records in this table have been iterated over, false otherwise
     */
    bool done() const { return m_cur_message >= m_num_messages; }

private:
    /**
     * Merges the current local schema tree with the section of the global schema tree corresponding
     * to the path from the root of the global schema tree to the node matching the global MPT node
     * id passed to this function.
     * @param global_id
     */
    void generate_local_tree(int32_t global_id);

    /**
     * Generates a json template
     * @param id
     */
    void generate_json_template(int32_t id);

    /**
     * Generates a json template for a structured array
     * @param id
     * @param column_start the index of the first reader in m_columns belonging to this array
     * @param schema
     * @return the index of the next reader in m_columns after those consumed by this array
     */
    size_t
    generate_structured_array_template(int32_t id, size_t column_start, std::span<int32_t> schema);

    /**
     * Generates a json template for a structured object
     * @param id
     * @param column_start the index of the first reader in m_columns belonging to this object
     * @param schema
     * @return the index of the next reader in m_columns after those consumed by this object
     */
    size_t
    generate_structured_object_template(int32_t id, size_t column_start, std::span<int32_t> schema);

    /**
     * Generates a JSON template for a LogMessage.
     * @param log_msg_node_id The LogMessage node ID.
     * @return A result containing the index of the next reader in m_columns after those consumed by
     * this object, or an error code indicating the failure:
     * - ClppErrorCodeEnum::Failure if the capture has no register IDs or the positions are invalid.
     * - ClppErrorCodeEnum::Unsupported if an unsupported or unexpected column type is found.
     */
    auto generate_log_message_template(SchemaNode::id_t log_msg_node_id)
            -> ystdlib::error_handling::Result<size_t>;

    /**
     * Finds the common root of the subtree containing cur_root and next_root, and adds brackets
     * and keys to m_json_serializer as necessary so that the json object is correct between the
     * previous field which is a child of cur_root, and the next field which is a child of
     * next_root.
     *
     * For example for the object {"a": {"b": "c"}, "d": {"e": {"f": "g"}}} after appending "b"
     * cur_root would be "a", and next_root would be "e". (since it is the parent of the next field
     * "f"). The current state of the object would look like "a":{"b":"c" -- to prepare for "f" we
     * would add },"d":{"e":{ or in other words close one bracket, add "d" and open bracket, add "e"
     * and open bracket. After adding field "f" the current root is "e", and the next root is the
     * original object which is the parent of "a" so we add }}.
     *
     * This works by tracing the path between both cur_root and next_root to their nearest common
     * ancestor. For every step cur_root takes towards this common ancestor we must close a bracket,
     * and for every step on the path from next_root a key must be added and a bracket must be
     * opened. The parameter `path_to_intersection` is used as a buffer to store the path from
     * next_root to this intersection so that the keys can be added to m_json_serializer in the
     * correct order.
     * @param cur_root
     * @param next_root
     * @param path_to_intersection
     */
    void find_intersection_and_fix_brackets(
            int32_t cur_root,
            int32_t next_root,
            std::vector<int32_t>& path_to_intersection
    );

    /**
     * Checks whether a node (or any ancestor up to and including the LogMessage root) is
     * projected. When Projection::Mode::ReturnAllColumns is active, this always returns true.
     * @param node_id The global schema node ID to check.
     * @return true if the node should be included in the output.
     */
    [[nodiscard]] auto is_node_projected(SchemaNode::id_t node_id) -> bool;

    /**
     * Reconstructs the text for a LogMessage or ParentRule from its shape by replacing
     * %column-name% placeholders with the column/leaf values.
     * @param log_msg_node_id The LogMessage node ID.
     * @param parent_rule_column_name The column name of a ParentRule to reconstruct, or empty for
     * the full message.
     * @param message_index The index of the message to reconstruct.
     * @return The reconstructed raw log text.
     */
    [[nodiscard]] auto reconstruct_log_shape(
            SchemaNode::id_t log_msg_node_id,
            std::string_view parent_rule_column_name,
            uint64_t message_index
    ) -> std::string;

    // LogMessage template generation helpers
    struct LogMessageModes {
        bool has_default{false};
        bool has_decomposed{false};
        bool has_shape{false};
        bool is_return_all{false};
        bool effective_has_default{false};
    };

    struct ChildProjectionModes {
        bool any_child_projected{false};
        bool any_child_decomposed{false};
        bool any_child_shape{false};
    };

    struct LeafEntry {
        SchemaNode::id_t global_column_id;
        size_t column_idx;
        std::string column_name;
        NodeType type;
    };

    /**
     * Computes the projection modes active for a LogMessage node.
     * @param log_msg_node_id The LogMessage node ID.
     * @return The computed modes.
     */
    [[nodiscard]] auto compute_log_message_modes(SchemaNode::id_t log_msg_node_id)
            -> LogMessageModes;

    /**
     * Scans the schema children of a LogMessage to determine which projection modes are active.
     * @param schema The schema span containing the LogMessage's children.
     * @param log_msg_node_id The LogMessage node ID.
     * @return The aggregated child projection modes, or an error code indicating the failure:
     * - ClppErrorCodeEnum::Corrupt if a `ParentRule` scope cannot be resolved to a schema-tree
     *   node, which indicates a corrupt or inconsistent archive.
     */
    [[nodiscard]] auto scan_child_projection_modes(
            std::span<SchemaNode::id_t> schema,
            SchemaNode::id_t log_msg_node_id
    ) -> ystdlib::error_handling::Result<ChildProjectionModes>;

    /**
     * Emits the default reconstruction and/or shape field for a ParentRule node.
     * @param log_msg_node_id The LogMessage node ID.
     * @param global_column_id The ParentRule column ID.
     * @param log_shape_id The log shape ID.
     * @param found_log_shape_id Whether the log shape ID was found.
     * @param emitted_parent_rules Set tracking already-emitted ParentRules (deduplication).
     */
    void emit_parent_rule_shape(
            SchemaNode::id_t log_msg_node_id,
            SchemaNode::id_t global_column_id,
            clpp::log_shape_id_t log_shape_id,
            bool found_log_shape_id,
            absl::flat_hash_set<SchemaNode::id_t>& emitted_parent_rules
    );

    /**
     * Visits every `ParentRule` unordered-object scope contained in `schema`, recursing into nested
     * scopes, and invokes `visit` with the resolved schema-tree node ID of the scope and the
     * sub-span it owns.
     *
     * `ParentRule` scopes are stored as unordered-object delimiters, which carry only a NodeType
     * and length rather than a node ID, so the owning tree node is recovered by walking up from the
     * scope's first leaf to the nearest `ParentRule` ancestor of `tree_root`.
     *
     * @param schema The schema span to walk.
     * @param tree_root The schema-tree node owning `schema` (the LogMessage for the top-level call,
     *     the owning `ParentRule` for nested calls).
     * @param visit Invoked as `visit(parent_rule_id, sub_span)` for each `ParentRule` scope, in
     *     depth-first schema order.
     * @tparam Visit Callable accepting `(SchemaNode::id_t, std::span<SchemaNode::id_t>)`.
     * @return A void result on success, or an error code indicating the failure:
     * - ClppErrorCodeEnum::Corrupt if a `ParentRule` unordered-object scope cannot be resolved to
     *   a schema-tree node (the scope's first leaf has no matching `ParentRule` ancestor of
     *   `tree_root`), which indicates a corrupt or inconsistent archive.
     */
    template <typename Visit>
    auto for_each_parent_rule_scope(
            std::span<SchemaNode::id_t> schema,
            SchemaNode::id_t tree_root,
            Visit&& visit
    ) -> ystdlib::error_handling::Result<void> {
        for (size_t i{0}; i < schema.size(); ++i) {
            auto const entry{schema[i]};
            if (false == Schema::schema_entry_is_unordered_object(entry)) {
                continue;
            }
            auto const length{Schema::get_unordered_object_length(entry)};
            if (NodeType::ParentRule != Schema::get_unordered_object_type(entry)) {
                i += length;
                continue;
            }
            auto const sub_span{schema.subspan(i + 1, length)};
            auto const parent_rule_id{m_global_schema_tree->find_matching_subtree_root_in_subtree(
                    tree_root,
                    get_first_column_in_span(sub_span),
                    NodeType::ParentRule
            )};
            if (-1 == parent_rule_id) {
                return clpp::ClppErrorCode{clpp::ClppErrorCodeEnum::Corrupt};
            }
            visit(parent_rule_id, sub_span);
            if (auto const recurse{for_each_parent_rule_scope(sub_span, parent_rule_id, visit)};
                recurse.has_error())
            {
                return recurse.error();
            }
            i += length;
        }
        return ystdlib::error_handling::success();
    }

    /**
     * Emits the shape constant string field for a LogTypeID node.
     * @param log_shape_id The log shape ID.
     * @return A void result on success, or an error code indicating the failure:
     * - ClppErrorCodeEnum::Failure if the shape dictionary is not available.
     */
    [[nodiscard]] auto emit_log_shape(clpp::log_shape_id_t log_shape_id)
            -> ystdlib::error_handling::Result<void>;

    /**
     * Collects leaf entries that should be emitted under a LogMessage's decomposed projection.
     * @param schema The schema span containing the LogMessage's children.
     * @param log_msg_node_id The LogMessage node ID.
     * @param column_idx The current column index, updated as columns are consumed.
     * @return The collected leaf entries.
     */
    [[nodiscard]] auto collect_leaf_entries(
            std::span<SchemaNode::id_t> schema,
            SchemaNode::id_t log_msg_node_id,
            size_t& column_idx
    ) -> std::vector<LeafEntry>;

    /**
     * Sorts, groups by column name, and emits leaf entries as JSON array fields.
     * @param entries The leaf entries to emit (sorted in-place).
     * @return A void result on success, or an error code indicating the failure:
     * - ClppErrorCodeEnum::Unsupported if an unsupported column type is encountered.
     */
    [[nodiscard]] auto emit_grouped_leaf_entries(std::vector<LeafEntry>& entries)
            -> ystdlib::error_handling::Result<void>;

    int32_t m_schema_id;
    uint64_t m_num_messages;
    uint64_t m_cur_message;
    std::span<int32_t> m_ordered_schema;

    std::unordered_map<int32_t, BaseColumnReader*> m_column_map;
    std::vector<BaseColumnReader*> m_columns;
    std::vector<BaseColumnReader*> m_reordered_columns;
    std::shared_ptr<char[]> m_stream_buffer;

    BaseColumnReader* m_timestamp_column;
    std::function<epochtime_t()> m_get_timestamp;
    BaseColumnReader* m_log_event_idx_column{nullptr};

    std::shared_ptr<SchemaTree> m_global_schema_tree;
    SchemaTree m_local_schema_tree;
    std::unordered_map<int32_t, int32_t> m_global_id_to_local_id;
    std::unordered_map<int32_t, int32_t> m_local_id_to_global_id;

    JsonSerializer m_json_serializer;
    bool m_should_marshal_records{true};
    bool m_serializer_initialized{false};
    std::shared_ptr<search::Projection> m_projection;

    std::map<int32_t, std::pair<size_t, std::span<int32_t>>> m_global_id_to_unordered_object;
    std::vector<std::pair<SchemaNode::id_t, std::string>> m_reconstruction_targets;
    // TODO clpp: the archive reader owns the schema reader so this is safe, but the ownership
    // between the readers is problematic.
    LogShapeDictionaryReader const* m_log_shape_dict;
    clpp::ParentRuleShapesArray const* m_parent_rule_shapes;
};
}  // namespace clp_s

#endif  // CLP_S_SCHEMAREADER_HPP
