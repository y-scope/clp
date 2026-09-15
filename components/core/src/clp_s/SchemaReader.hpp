#ifndef CLP_S_SCHEMAREADER_HPP
#define CLP_S_SCHEMAREADER_HPP

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include <absl/container/flat_hash_map.h>
#include <ystdlib/error_handling/Result.hpp>

#include <clp_s/ColumnReader.hpp>
#include <clp_s/DictionaryReader.hpp>
#include <clp_s/ErrorCode.hpp>
#include <clp_s/JsonSerializer.hpp>
#include <clp_s/Schema.hpp>
#include <clp_s/SchemaTree.hpp>
#include <clp_s/search/Projection.hpp>
#include <clp_s/TraceableException.hpp>
#include <clpp/Defs.hpp>
#include <clpp/ErrorCode.hpp>
#include <clpp/ParentRuleShapes.hpp>

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
    // Types
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

    /**
     * A shape compiled into segments for fast per-message reconstruction.
     *
     * Each segment is either:
     * - A `std::string` containing text that is safe to append to the output buffer (JSON-escaped
     *   and literal % unescaped).
     * - A `BaseColumnReader*` used to extract the value for that placeholder in the shape.
     */
    struct CompiledShape {
        using Segment = std::variant<std::string, BaseColumnReader*>;
        std::vector<Segment> segments;
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
     * @param log_shape_dict
     * @param parent_rule_shapes
     * @param extract_mode
     */
    void reset(
            std::shared_ptr<SchemaTree> schema_tree,
            std::shared_ptr<search::Projection> projection,
            int32_t schema_id,
            std::span<int32_t const> ordered_schema,
            uint64_t num_messages,
            bool should_marshal_records,
            LogShapeDictionaryReader const* log_shape_dict,
            clpp::ParentRuleShapesArray const* parent_rule_shapes,
            bool extract_mode = false
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
        m_extract_mode = extract_mode;
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
    auto append_unordered_column(std::unique_ptr<BaseColumnReader> column_reader) -> void;

    size_t get_column_size() { return m_columns.size(); }

    /**
     * Marks an unordered object for the purpose of marshalling records.
     *
     * Objects rooted at a `ParentRule` node are not stored in the map and must be reached by
     * walking the schema.
     * Multiple `ParentRule` objects (of the same parent rule) will share the same MST node ID if
     * they are directly under the same parent. This means it is not possible to store each match in
     * the map as they have the same key.
     *
     * @param column_reader_start
     * @param mst_subtree_root
     * @param sub_schema The object's sub-schema.
     * @param log_shape_id The object's log shape ID (present for `LogMessage` objects).
     */
    void mark_unordered_object(
            size_t column_reader_start,
            int32_t mst_subtree_root,
            SchemaView sub_schema,
            std::optional<clpp::log_shape_id_t> log_shape_id
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
     */
    void initialize_serializer();

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
     * @param sub_schema
     * @return the first column ID found in the given sub-schema, or -1 if the sub-schema contains
     * no columns
     */
    [[nodiscard]] static auto get_first_column_in_span(SchemaView sub_schema) -> SchemaNode::id_t;

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
    // Types
    /**
     * A leaf column targeted for decomposed output.
     */
    struct DecompositionTarget {
        SchemaNode::id_t node_id;
        size_t reader_idx;
        std::string_view name;
        NodeType type;
    };

    /**
     * Records the sub-schema and starting column index for a single occurrence of a ParentRule
     * within its parent scope.
     */
    struct ParentRuleOccurrence {
        SchemaView sub_schema;
        size_t start_column_reader_idx{0};
    };

    /**
     * A marked unordered object: its starting column index, sub-schema, and log shape ID (present
     * for `LogMessage` objects).
     */
    struct MarkedUnorderedObject {
        size_t column_reader_start{0};
        SchemaView sub_schema;
        std::optional<clpp::log_shape_id_t> log_shape_id;
    };

    /**
     * Content collected from a single walk of a sub-schema during decomposed output generation:
     * @var next_column_reader_idx Index into `m_columns` after consuming all of the sub-schema's
     * columns.
     * @var entries The direct leaf entries collected from this sub-schema.
     * @var parent_rule_insertion_order ParentRule node IDs in first-seen order.
     * @var parent_rule_occurrences Maps each ParentRule node ID to all of its occurrences.
     */
    struct SchemaSpanContents {
        size_t next_column_reader_idx{0};
        std::vector<DecompositionTarget> entries;
        std::vector<SchemaNode::id_t> parent_rule_insertion_order;
        absl::flat_hash_map<SchemaNode::id_t, std::vector<ParentRuleOccurrence>>
                parent_rule_occurrences;
    };

    // Methods
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
     * @param array_root_id
     * @param column_start the index of the first reader in m_columns belonging to this array
     * @param schema
     * @return the index of the next reader in m_columns after those consumed by this array
     */
    auto generate_structured_array_template(
            int32_t array_root_id,
            size_t column_start,
            SchemaView sub_schema
    ) -> size_t;

    /**
     * Generates a json template for a structured object
     * @param id
     * @param column_start the index of the first reader in m_columns belonging to this object
     * @param schema
     * @return the index of the next reader in m_columns after those consumed by this object
     */
    auto generate_structured_object_template(int32_t id, size_t column_start, SchemaView sub_schema)
            -> size_t;

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
     * projected. When all columns are returned (ReturnAllColumns mode), this always returns true.
     * @param node_id The global schema node ID to check.
     * @return true if the node should be included in the output.
     */
    [[nodiscard]] auto is_node_projected(SchemaNode::id_t node_id) -> bool;

    /**
     * Compiles a shape into a `CompiledShape` for per-message reconstruction.
     *
     * Parses the shape, unescapes literal %'s, escapes JSON characters, and resolves each
     * `%column_name%` placeholder to its corresponding column reader. This allows reconstructing
     * each message to be done in a single pass with no repeated work.
     *
     * @param log_shape_id The log shape ID of the owning LogMessage scope.
     * @param parent_rule_column_name The column name of the ParentRule to narrow the shape to,
     * or empty to reconstruct the full LogMessage shape.
     * @param start_column_reader_idx Index in `m_columns`.
     * @param sub_schema The sub-schema to iterate for column values.
     * @return The fully-compiled shape.
     */
    [[nodiscard]] auto compile_shape(
            clpp::log_shape_id_t log_shape_id,
            std::string_view parent_rule_column_name,
            size_t start_column_reader_idx,
            SchemaView sub_schema
    ) -> CompiledShape;

    /**
     * Visits every `ParentRule` unordered object contained in `schema`, recursing into nested
     * objects, and invokes `visit` with the ParentRule's MST node ID and its sub-schema (excluding
     * the leading `root_node_id` metadata entry).
     *
     * @param schema
     * @param visit Invoked for each `ParentRule` scope, in depth-first schema order.
     * @tparam Visit Callable accepting `(SchemaNode::id_t parent_rule_id, SchemaView sub_schema)`.
     */
    template <typename Visit>
    auto for_each_parent_rule_scope(SchemaView schema, Visit const& visit) -> void {
        schema.visit_entries(
                [](SchemaNode::id_t) -> bool { return false; },
                [&](UnorderedObject const& obj) -> bool {
                    if (NodeType::ParentRule != obj.type) {
                        return false;
                    }
                    visit(obj.root_node_id.value(), obj.sub_schema);
                    for_each_parent_rule_scope(obj.sub_schema, visit);
                    return false;
                }
        );
    }

    /**
     * Emits the shape constant string field.
     * @param log_shape_id The log shape ID.
     * @return A void result on success, or an error code indicating the failure:
     * - ClppErrorCodeEnum::Failure if the shape dictionary is not available.
     */
    [[nodiscard]] auto emit_log_shape(clpp::log_shape_id_t log_shape_id)
            -> ystdlib::error_handling::Result<void>;

    /**
     * Emits the shape substring for a specific ParentRule within a LogMessage's shape.
     * @param log_shape_id The log shape ID.
     * @param parent_rule_column_name The column name of the ParentRule.
     * @return A void result on success, or an error code indicating the failure:
     * - ClppErrorCodeEnum::Failure if the shape dictionary or parent rule shapes are not available.
     */
    [[nodiscard]] auto emit_parent_rule_shape_substring(
            clpp::log_shape_id_t log_shape_id,
            std::string_view parent_rule_column_name
    ) -> ystdlib::error_handling::Result<void>;

    /**
     * Sorts, groups by column name, and emits leaf entries as JSON array fields.
     * @param entries The leaf entries to emit (sorted in-place).
     * @return A void result on success, or an error code indicating the failure:
     * - ClppErrorCodeEnum::Unsupported if an unsupported column type is encountered.
     */
    [[nodiscard]] auto emit_grouped_leaf_entries(std::vector<DecompositionTarget>& entries)
            -> ystdlib::error_handling::Result<void>;

    /**
     * Walks a sub-schema, collecting direct leaf entries and ParentRule occurrences (grouped by
     * parent-rule node ID, with insertion order preserved), while advancing the column index past
     * all column-consuming entries including those inside nested scopes.
     * @param schema The sub-schema to walk.
     * @param scope_node_id The node ID of the owning scope (LogMessage or ParentRule).
     * @param start_column_reader_idx The index of this scope's first column reader in `m_columns`.
     * @param ancestor_decomposed Whether an ancestor scope has Decomposed projection active,
     * causing all direct leaves to be collected regardless of individual projection checks.
     * @return The collected scope contents.
     */
    [[nodiscard]] auto collect_scope_entries(
            SchemaView schema,
            SchemaNode::id_t scope_node_id,
            size_t start_column_reader_idx,
            bool ancestor_decomposed
    ) -> SchemaSpanContents;

    /**
     * Emits ParentRule arrays from the grouped occurrences in a `SchemaSpanContents`. For each
     * ParentRule group, emits a JSON array with one object per occurrence. Each object contains any
     * projected fields.
     * @param scope The collected scope contents.
     * @param log_shape_id
     * @param ancestor_decomposed Whether an ancestor scope has Decomposed projection active.
     * @return A void result on success, or an error code indicating the failure:
     * - Forwards `emit_parent_rule_shape_substring`'s return values.
     * - Forwards `emit_decomposed_scope`'s return values.
     */
    [[nodiscard]] auto emit_parent_rule_arrays(
            SchemaSpanContents const& scope,
            clpp::log_shape_id_t log_shape_id,
            bool ancestor_decomposed
    ) -> ystdlib::error_handling::Result<void>;

    /**
     * Emits the decomposed content of a LogMessage or ParentRule sub-schema as direct leaf arrays
     * and nested ParentRule objects. The ParentRule objects contain their direct leaf arrays and
     * other nested ParentRule objects.
     * @param schema The sub-schema to walk.
     * @param scope_node_id The node ID of the owning scope (LogMessage or ParentRule).
     * @param column_idx The index of this scope's first column reader in `m_columns`.
     * @param log_shape_id
     * @param ancestor_decomposed Whether an ancestor scope has Decomposed projection active,
     * causing all leaves and child ParentRules to be emitted regardless of individual projection
     * checks.
     * @return The column index past the last consumed column in this scope, or an error code
     * indicating the failure:
     * - Forwards `emit_grouped_leaf_entries`'s return values.
     * - Forwards `emit_parent_rule_arrays`'s return values.
     */
    [[nodiscard]] auto emit_decomposed_scope(
            SchemaView schema,
            SchemaNode::id_t scope_node_id,
            size_t column_idx,
            clpp::log_shape_id_t log_shape_id,
            bool ancestor_decomposed
    ) -> ystdlib::error_handling::Result<size_t>;

    /**
     * Reconstructs the original raw log text for a single message by walking the compiled
     * shape and writing into the output buffer.
     *
     * std::string segments are pre-escaped and appended to the buffer. BaseColumnReader segments
     * extract and JSON-escape the column value into the buffer. The entire output is wrapped in a
     * single pair of JSON quotes.
     *
     * @param shape The compiled shape.
     * @param message_index The index of the message to reconstruct.
     * @param buffer The output buffer to write into.
     */
    auto reconstruct_compiled_shape(
            CompiledShape const& shape,
            uint64_t message_index,
            std::string& buffer
    ) -> void;

    // Data members
    int32_t m_schema_id;
    uint64_t m_num_messages;
    uint64_t m_cur_message;
    std::span<int32_t const> m_ordered_schema;

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

    // Keyed by the object root's schema-tree node ID.
    std::map<int32_t, MarkedUnorderedObject> m_global_id_to_unordered_object;
    std::vector<CompiledShape> m_reconstruction_targets;
    LogShapeDictionaryReader const* m_log_shape_dict;
    clpp::ParentRuleShapesArray const* m_parent_rule_shapes;
    bool m_extract_mode{false};
};
}  // namespace clp_s

#endif  // CLP_S_SCHEMAREADER_HPP
