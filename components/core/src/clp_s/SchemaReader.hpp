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
#include <clpp/TextShape.hpp>

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
     * Column-reader range `[begin, end)` of one StructurizedText occurrence.
     */
    struct StructurizedTextColumnRange {
        size_t begin;
        size_t end;
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
     * @param extract_mode
     */
    auto reset(
            std::shared_ptr<SchemaTree> schema_tree,
            std::shared_ptr<search::Projection> projection,
            int32_t schema_id,
            std::span<int32_t const> ordered_schema,
            uint64_t num_messages,
            bool should_marshal_records,
            bool extract_mode = false
    ) -> void {
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
        m_structurized_text_column_ranges.clear();
        m_local_schema_tree.clear();
        m_json_serializer.clear();
        m_global_schema_tree = std::move(schema_tree);
        m_projection = std::move(projection);
        m_should_marshal_records = should_marshal_records;
        m_log_shape_dict = nullptr;
        m_parent_rule_shapes_array = nullptr;
        m_extract_mode = extract_mode;
    }

    /**
     * Sets the dictionary used to reconstruct the text of the schema's StructurizedText objects.
     * @param log_shape_dict Must outlive the reader's use of this schema.
     */
    auto set_log_shape_dictionary(LogShapeDictionaryReader const* log_shape_dict) -> void {
        m_log_shape_dict = log_shape_dict;
    }

    /**
     * Sets the shapes used to reconstruct the text of the schema's ParentRule objects.
     * @param parent_rule_shapes_array Must outlive the reader's use of this schema.
     */
    auto set_parent_rule_shapes_array(clpp::ParentRuleShapesArray const* parent_rule_shapes_array)
            -> void {
        m_parent_rule_shapes_array = parent_rule_shapes_array;
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
     * Multiple `ParentRule` objects (of the same parent rule) will share the same MPT node ID if
     * they are directly under the same parent. This means it is not possible to store each match in
     * the map as they have the same key.
     *
     * @param start_column_reader_idx Index in `m_columns` of the object's first column reader.
     * @param subtree_root
     * @param sub_schema The object's sub-schema.
     * @param log_shape_id The object's log shape ID (present for `StructurizedText` objects).
     */
    auto mark_unordered_object(
            size_t start_column_reader_idx,
            mpt_node_id_t subtree_root,
            SchemaView sub_schema,
            std::optional<clpp::log_shape_id_t> log_shape_id
    ) -> void;

    /**
     * Records an occurrence's columns in same-node document order, even when not marshalling.
     * StructurizedText objects are never nested, so completion order is also document order.
     * @param structurized_text_id The `StructurizedText` root node ID.
     * @param begin Index in `m_columns` of the object's first column reader.
     * @param end Index just past the object's last column reader.
     */
    auto append_structurized_text_column_range(
            mpt_node_id_t structurized_text_id,
            size_t begin,
            size_t end
    ) -> void {
        m_structurized_text_column_ranges[structurized_text_id].push_back(
                {.begin = begin, .end = end}
        );
    }

    /**
     * @param structurized_text_id The `StructurizedText` root node ID.
     * @param occurrence_idx The zero-based same-node occurrence index in document order.
     * @return The occurrence's column-reader range.
     * @return std::nullopt if the occurrence does not exist.
     */
    [[nodiscard]] auto get_structurized_text_column_range(
            mpt_node_id_t structurized_text_id,
            size_t occurrence_idx
    ) const -> std::optional<StructurizedTextColumnRange> {
        auto const it{m_structurized_text_column_ranges.find(structurized_text_id)};
        if (m_structurized_text_column_ranges.end() == it || occurrence_idx >= it->second.size()) {
            return std::nullopt;
        }
        return it->second.at(occurrence_idx);
    }

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
     * @throws std::runtime_error if generating the serializer ops fails.
     * @throws std::out_of_range if the schema consumes more column readers than `m_columns` holds
     * (corrupt archive).
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
     * A leaf column selected for output as part of a decomposed StructurizedText or ParentRule
     * scope, with the reader it is read from:
     * @var reader_idx Index of the leaf's column reader in `m_columns`.
     * @var name The leaf's key name, used as the key of its output array.
     * @var type The leaf's node type, which selects the serializer op used to output its value.
     */
    struct DecompositionTarget {
        size_t reader_idx{};
        std::string_view name;
        NodeType type{NodeType::Unknown};
    };

    /**
     * A single occurrence of a ParentRule within its parent scope:
     * @var sub_schema The occurrence's sub-schema.
     * @var start_column_reader_idx Index in `m_columns` of the occurrence's first column reader.
     * @var end_column_reader_idx Index in `m_columns` one past the occurrence's last column reader.
     */
    struct ParentRuleOccurrence {
        SchemaView sub_schema;
        size_t start_column_reader_idx{0};
        size_t end_column_reader_idx{0};
    };

    /**
     * A marked unordered object:
     * @var start_column_reader_idx Index in `m_columns` of the object's first column reader.
     * @var sub_schema The object's sub-schema.
     * @var log_shape_id The object's log shape ID (present for `StructurizedText` objects).
     */
    struct MarkedUnorderedObject {
        size_t start_column_reader_idx{0};
        SchemaView sub_schema;
        std::optional<clpp::log_shape_id_t> log_shape_id;
    };

    /**
     * Content collected from a single walk of a sub-schema during decomposed output generation:
     * @var entries The direct leaf entries collected from this sub-schema.
     * @var parent_rule_insertion_order ParentRule node IDs in first-seen order.
     * @var parent_rule_occurrences Maps each ParentRule node ID to its occurrences, in document
     * order.
     */
    struct SchemaSpanContents {
        std::vector<DecompositionTarget> entries;
        std::vector<mpt_node_id_t> parent_rule_insertion_order;
        absl::flat_hash_map<mpt_node_id_t, std::vector<ParentRuleOccurrence>>
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
     * Generates the serializer ops for the subtree of the local schema tree rooted at `id`.
     * @param id The local schema tree node ID.
     * @return A void result on success, or an error code indicating the failure:
     * - Forwards `generate_structured_array_template`'s return values.
     * - Forwards `generate_structurized_text_template`'s return values.
     */
    [[nodiscard]] auto generate_json_template(int32_t id) -> ystdlib::error_handling::Result<void>;

    /**
     * Generates the serializer ops for a structurized array.
     * @param array_root_id The array's node ID in the global schema tree.
     * @param start_column_reader_idx Index in `m_columns` of the array's first column reader.
     * @param sub_schema The array's sub-schema.
     * @return A result containing the index in `m_columns` one past the array's last column reader,
     * or an error code indicating the failure:
     * - Forwards `generate_array_nested_object_template`'s return values.
     */
    [[nodiscard]] auto generate_structured_array_template(
            int32_t array_root_id,
            size_t start_column_reader_idx,
            SchemaView sub_schema
    ) -> ystdlib::error_handling::Result<size_t>;

    /**
     * Generates the serializer ops for an unordered object nested in a structurized array: a nested
     * array, a nested object, or a `StructurizedText` emitted as an unnamed text value.
     * @param obj The nested unordered object.
     * @param array_root_id The enclosing array's node ID in the global schema tree.
     * @param start_column_reader_idx Index in `m_columns` of `obj`'s first column reader.
     * @return A result containing the index in `m_columns` one past `obj`'s last column reader, or
     * an error code indicating the failure:
     * - ClppErrorCodeEnum::Corrupt if `obj` has an unexpected type, no node IDs, or (for a
     *   `StructurizedText`) no log shape ID.
     * - Forwards `get_log_shape`'s return values.
     * - Forwards `scope_column_readers`' return values.
     * - Forwards `JsonSerializer::add_reconstructed_text_value_op`'s return values.
     * - Forwards `generate_structured_array_template`'s return values.
     * - Forwards `generate_structured_object_template`'s return values.
     */
    [[nodiscard]] auto generate_array_nested_object_template(
            UnorderedObject const& obj,
            int32_t array_root_id,
            size_t start_column_reader_idx
    ) -> ystdlib::error_handling::Result<size_t>;

    /**
     * Generates the serializer ops for a structured object nested in a structurized array.
     * @param object_root The object's node ID in the global schema tree.
     * @param start_column_reader_idx Index in `m_columns` of the object's first column reader.
     * @param sub_schema The object's sub-schema.
     * @return A result containing the index in `m_columns` one past the object's last column
     * reader, or an error code indicating the failure:
     * - Forwards `generate_object_nested_object_template`'s return values.
     */
    [[nodiscard]] auto generate_structured_object_template(
            int32_t object_root,
            size_t start_column_reader_idx,
            SchemaView sub_schema
    ) -> ystdlib::error_handling::Result<size_t>;

    /**
     * Generates the serializer ops for an unordered object nested in an object that is itself
     * nested in a structurized array. Nested plain objects are flattened into the enclosing
     * object's sub-schema, so `obj` is either a nested array or a `StructurizedText` emitted as a
     * text field.
     * @param obj The nested unordered object.
     * @param object_root The enclosing object's node ID in the global schema tree.
     * @param cur_root The node ID whose children the previous field was emitted under; updated to
     * the node ID whose children the next field should be emitted under.
     * @param path_to_intersection Scratch buffer for `find_intersection_and_fix_brackets`.
     * @param start_column_reader_idx Index in `m_columns` of `obj`'s first column reader.
     * @return A result containing the index in `m_columns` one past `obj`'s last column reader, or
     * an error code indicating the failure:
     * - ClppErrorCodeEnum::Corrupt if `obj` has an unexpected type, no node IDs, or (for a
     *   `StructurizedText`) no log shape ID.
     * - Forwards `get_log_shape`'s return values.
     * - Forwards `scope_column_readers`' return values.
     * - Forwards `JsonSerializer::add_reconstructed_text_field_op`'s return values.
     * - Forwards `generate_structured_array_template`'s return values.
     */
    [[nodiscard]] auto generate_object_nested_object_template(
            UnorderedObject const& obj,
            int32_t object_root,
            int32_t& cur_root,
            std::vector<int32_t>& path_to_intersection,
            size_t start_column_reader_idx
    ) -> ystdlib::error_handling::Result<size_t>;

    /**
     * Counts the column readers consumed by a scope, including entries within nested unordered
     * objects.
     * @param start_column_reader_idx Index in `m_columns` of a scope's first column reader.
     * @param sub_schema The scope's sub-schema.
     * @return Index in `m_columns` one past the scope's last column reader.
     */
    [[nodiscard]] auto
    compute_end_column_reader_idx(size_t start_column_reader_idx, SchemaView sub_schema) const
            -> size_t;

    /**
     * Slices `m_columns` to the readers consumed by one scope. The returned span references
     * `m_columns` and is invalidated by any operation that modifies `m_columns`.
     * @param start_column_reader_idx Index in `m_columns` of a scope's first column reader.
     * @param end_column_reader_idx Index in `m_columns` one past the scope's last column reader.
     * @return A result containing the column readers consumed by the scope, or an error code
     * indicating the failure:
     * - ClppErrorCodeEnum::Corrupt if the range is inverted or exceeds `m_columns`.
     */
    [[nodiscard]] auto
    scope_column_readers(size_t start_column_reader_idx, size_t end_column_reader_idx) const
            -> ystdlib::error_handling::Result<std::span<BaseColumnReader* const>>;

    /**
     * Looks up a StructurizedText's shape in the log shape dictionary.
     * @param log_shape_id A StructurizedText's log shape ID.
     * @return A result containing the log shape, or an error code indicating the failure:
     * - ClppErrorCodeEnum::Failure if the log shape dictionary is not available.
     * - ClppErrorCodeEnum::Corrupt if `log_shape_id` is out of range.
     * The shape views `m_log_shape_dict`'s storage and is valid only while the dictionary is.
     */
    [[nodiscard]] auto get_log_shape(clpp::log_shape_id_t log_shape_id) const
            -> ystdlib::error_handling::Result<clpp::TextShape<std::string_view>>;

    /**
     * Looks up a log shape narrowed to one ParentRule occurrence.
     * @param log_shape_id The log shape ID of the StructurizedText containing the ParentRule.
     * @param parent_rule_column_name The column name of the ParentRule.
     * @param leaf_position The position of the occurrence's first leaf placeholder among the
     * StructurizedText's placeholders.
     * @param num_placeholders The number of leaf placeholders in the occurrence.
     * @return A result containing the log shape narrowed to the ParentRule occurrence, or an error
     * code indicating the failure:
     * - ClppErrorCodeEnum::Failure if the log shape dictionary or parent rule shapes are not
     *   available.
     * - ClppErrorCodeEnum::Corrupt if `log_shape_id` is out of range or the occurrence is not
     *   found in the log shape.
     * The shape views `m_log_shape_dict`'s storage and is valid only while the dictionary is.
     */
    [[nodiscard]] auto get_parent_rule_shape(
            clpp::log_shape_id_t log_shape_id,
            std::string_view parent_rule_column_name,
            size_t leaf_position,
            size_t num_placeholders
    ) const -> ystdlib::error_handling::Result<clpp::TextShape<std::string_view>>;

    /**
     * Generates the serializer ops for a StructurizedText. Its possible fields are the
     * reconstructed text (when the StructurizedText itself is projected), its shape, and its
     * leaf-rule and ParentRule arrays. The StructurizedText is emitted as an object containing
     * those fields, or as a bare text string when the text is its only field. Extraction always
     * emits the bare text.
     * @param structurized_text_id The StructurizedText node ID.
     * @return A void result on success, or an error code indicating the failure:
     * - ClppErrorCodeEnum::Failure if the StructurizedText was not marked.
     * - ClppErrorCodeEnum::Corrupt if the StructurizedText has no log shape ID.
     * - Forwards `get_log_shape`'s return values.
     * - Forwards `scope_column_readers`' return values.
     * - Forwards `JsonSerializer::add_reconstructed_text_field_op`'s return values.
     * - Forwards `add_decomposed_scope_ops`'s return values.
     */
    [[nodiscard]] auto generate_structurized_text_template(mpt_node_id_t structurized_text_id)
            -> ystdlib::error_handling::Result<void>;

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
     * Adds the serializer ops that output leaf entries as JSON array fields, one array per column
     * name, and appends each entry's column reader to `m_reordered_columns` in output order.
     * @param entries The leaf entries to output (sorted by name in-place).
     * @return A void result on success, or an error code indicating the failure:
     * - ClppErrorCodeEnum::Unsupported if an unsupported column type is encountered.
     */
    [[nodiscard]] auto add_grouped_leaf_array_ops(std::vector<DecompositionTarget>& entries)
            -> ystdlib::error_handling::Result<void>;

    /**
     * Walks a sub-schema, collecting direct leaf entries and ParentRule occurrences (grouped by
     * parent-rule node ID, with insertion order preserved), assigning each its column reader
     * indices in `m_columns`.
     * @param schema The sub-schema to walk.
     * @param scope_node_id The node ID of the owning decomposable scope.
     * @param start_column_reader_idx Index in `m_columns` of this scope's first column reader.
     * @param ancestor_decomposed Whether an ancestor scope has Decomposed projection active,
     * causing all direct leaves to be collected regardless of individual projection checks.
     * @return A result containing the collected scope contents, or an error code indicating the
     * failure:
     * - ClppErrorCodeEnum::Corrupt if a ParentRule object has no node ID.
     */
    [[nodiscard]] auto collect_scope_entries(
            SchemaView schema,
            mpt_node_id_t scope_node_id,
            size_t start_column_reader_idx,
            bool ancestor_decomposed
    ) -> ystdlib::error_handling::Result<SchemaSpanContents>;

    /**
     * Adds the serializer ops that output the ParentRule arrays for the grouped occurrences in a
     * `SchemaSpanContents`. Each projected ParentRule group is output as a JSON array with one
     * object per occurrence, containing that occurrence's projected fields: reconstructed text
     * (when the ParentRule itself is projected), shape, and leaf-rule/ParentRule arrays.
     * @param scope The collected scope contents.
     * @param log_shape_id
     * @param structurized_text_column_start Index in `m_columns` of the enclosing
     * StructurizedText's first column reader.
     * @param ancestor_decomposed Whether an ancestor scope has Decomposed projection active.
     * @return A void result on success, or an error code indicating the failure:
     * - Forwards `add_parent_rule_occurrence_ops`'s return values.
     */
    [[nodiscard]] auto add_parent_rule_array_ops(
            SchemaSpanContents const& scope,
            clpp::log_shape_id_t log_shape_id,
            size_t structurized_text_column_start,
            bool ancestor_decomposed
    ) -> ystdlib::error_handling::Result<void>;

    /**
     * Adds the serializer ops that output one ParentRule occurrence's projected fields
     * (reconstructed text and/or narrowed shape) and its decomposed content. The occurrence's
     * shape is located by its leaf position and placeholder count, computed from its column
     * reader range relative to `structurized_text_column_start`.
     * @param parent_rule_id The ParentRule's node ID.
     * @param occurrence The occurrence to emit.
     * @param parent_rule_column_name The ParentRule's column name.
     * @param log_shape_id The log shape ID of the enclosing StructurizedText.
     * @param structurized_text_column_start Index in `m_columns` of the enclosing
     * StructurizedText's first column reader.
     * @param emit_text Whether the occurrence's reconstructed text is projected.
     * @param emit_shape Whether the occurrence's narrowed shape is projected.
     * @param decompose_children Whether the occurrence's content is decomposed.
     * @return A void result on success, or an error code indicating the failure:
     * - Forwards `get_parent_rule_shape`'s return values.
     * - Forwards `scope_column_readers`' return values.
     * - Forwards `JsonSerializer::add_reconstructed_text_field_op`'s return values.
     * - Forwards `add_decomposed_scope_ops`'s return values.
     */
    [[nodiscard]] auto add_parent_rule_occurrence_ops(
            mpt_node_id_t parent_rule_id,
            ParentRuleOccurrence const& occurrence,
            std::string_view parent_rule_column_name,
            clpp::log_shape_id_t log_shape_id,
            size_t structurized_text_column_start,
            bool emit_text,
            bool emit_shape,
            bool decompose_children
    ) -> ystdlib::error_handling::Result<void>;

    /**
     * Adds the serializer ops that output the decomposed content of a decomposable sub-schema as
     * direct leaf rule arrays and nested ParentRule arrays. Each ParentRule occurrence object
     * contains its own direct leaf arrays and nested ParentRule arrays.
     * @param schema The sub-schema to walk.
     * @param scope_node_id The node ID of the owning decomposable scope.
     * @param start_column_reader_idx Index in `m_columns` of this scope's first column reader.
     * @param log_shape_id
     * @param structurized_text_column_start Index in `m_columns` of the enclosing
     * StructurizedText's first column reader.
     * @param ancestor_decomposed Whether an ancestor scope has Decomposed projection active,
     * causing all leaves and child ParentRules to be output regardless of individual projection
     * checks.
     * @return A void result on success, or an error code indicating the failure:
     * - Forwards `collect_scope_entries`'s return values.
     * - Forwards `add_grouped_leaf_array_ops`'s return values.
     * - Forwards `add_parent_rule_array_ops`'s return values.
     */
    [[nodiscard]] auto add_decomposed_scope_ops(
            SchemaView schema,
            mpt_node_id_t scope_node_id,
            size_t start_column_reader_idx,
            clpp::log_shape_id_t log_shape_id,
            size_t structurized_text_column_start,
            bool ancestor_decomposed
    ) -> ystdlib::error_handling::Result<void>;

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
    // Keyed by the `StructurizedText` root's schema-tree node ID.
    std::unordered_map<mpt_node_id_t, std::vector<StructurizedTextColumnRange>>
            m_structurized_text_column_ranges;
    LogShapeDictionaryReader const* m_log_shape_dict{nullptr};
    clpp::ParentRuleShapesArray const* m_parent_rule_shapes_array{nullptr};
    bool m_extract_mode{false};
};
}  // namespace clp_s

#endif  // CLP_S_SCHEMAREADER_HPP
