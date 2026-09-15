#ifndef CLP_S_ARCHIVEREADER_HPP
#define CLP_S_ARCHIVEREADER_HPP

#include <cstddef>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

#include <nlohmann/json_fwd.hpp>
#include <ystdlib/error_handling/Result.hpp>

#include <clp_s/ArchiveReaderAdaptor.hpp>
#include <clp_s/DictionaryEntry.hpp>
#include <clp_s/DictionaryReader.hpp>
#include <clp_s/InputConfig.hpp>
#include <clp_s/PackedStreamReader.hpp>
#include <clp_s/ReaderUtils.hpp>
#include <clp_s/Schema.hpp>
#include <clp_s/SchemaReader.hpp>
#include <clp_s/SchemaTree.hpp>
#include <clp_s/search/Projection.hpp>
#include <clp_s/SingleFileArchiveDefs.hpp>
#include <clp_s/TimestampDictionaryReader.hpp>
#include <clpp/Defs.hpp>
#include <clpp/LogShapeStat.hpp>
#include <clpp/ParentRuleShapes.hpp>

namespace clp_s {
class ArchiveReader {
public:
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}
    };

    struct Options {
        NetworkAuthOption m_network_auth{};
        bool m_experimental{false};
        bool m_extract_mode{false};
    };

    // Constructor
    ArchiveReader() : m_is_open(false) {}

    /**
     * Opens an archive for reading.
     * @param archive_path
     * @param options
     */
    void open(Path const& archive_path, Options const& options);

    /**
     * Opens a single-file archive for reading from an already open `clp::ReaderInterface`.
     * @param single_file_archive_reader The already opened archive reader.
     * @param archive_id The unique name or identifier for the archive.
     * @param options Options controlling how the archive is read.
     */
    auto open(
            std::shared_ptr<clp::ReaderInterface> single_file_archive_reader,
            std::string_view archive_id,
            Options const& options
    ) -> void;

    /**
     * Reads the dictionaries and metadata.
     * @throws OperationFailed if reading or decompressing metadata fails.
     */
    void read_dictionaries_and_metadata();

    /**
     * Opens packed streams for reading.
     */
    void open_packed_streams();

    /**
     * For single-file archives, reads the sections stored before `section` if they haven't already
     * been read. For multi-file archives, this function is a no-op as it is possible to read
     * sections out of order.
     *
     * @param section
     * @throws OperationFailed(ErrorCodeFailure) if metadata reading fails.
     * @throw Propagates exceptions from the `get_` function of each archive section.
     */
    auto ensure_section_readable(std::string_view section) -> void;

    /**
     * Reads the log type statistics from the archive.
     * @return
     */
    auto read_log_shape_stats() -> ystdlib::error_handling::Result<clpp::LogShapeStatArray>;

    /**
     * Reads the parsing specification from the archive.
     */
    auto read_parsing_spec() -> ystdlib::error_handling::Result<std::string>;

    /**
     * Reads the metadata from the archive.
     * @return A void result on success, or an error code indicating the failure:
     * - Forwards `ArchiveReader::read_single_schema_metadata`'s return values on failure.
     * - Forwards `PackedStreamReader::read_metadata`'s return values on failure.
     * @throws OperationFailed if archive metadata is empty or corrupt.
     * @throws OperationFailed if archive metadata stream offset is not strictly incremental.
     * @throws OperationFailed if reading or decompressing metadata fails.
     */
    [[nodiscard]] auto read_metadata() -> ystdlib::error_handling::Result<void>;

    /**
     * Reads a table from the archive.
     * @param schema_id
     * @param should_extract_timestamp
     * @param should_marshal_records
     * @return the schema reader
     */
    SchemaReader& read_schema_table(
            int32_t schema_id,
            bool should_extract_timestamp,
            bool should_marshal_records
    );

    /**
     * Loads all of the tables in the archive and returns SchemaReaders for them.
     * @return the schema readers for every table in the archive
     */
    std::vector<std::shared_ptr<SchemaReader>> read_all_tables();

    std::string_view get_archive_id() { return m_archive_id; }

    /**
     * @return The variable dictionary, reading it from the archive if it hasn't been read yet.
     */
    auto get_variable_dictionary() -> std::shared_ptr<VariableDictionaryReader>;

    /**
     * @return The log type dictionary, reading it from the archive if it hasn't been read yet.
     * Always null for an experimental archive, which stores log shapes instead.
     */
    auto get_log_type_dictionary() -> std::shared_ptr<LogTypeDictionaryReader>;

    /**
     * @return The array dictionary, reading it from the archive if it hasn't been read yet.
     */
    auto get_array_dictionary() -> std::shared_ptr<LogTypeDictionaryReader>;

    /**
     * @return The log shape dictionary, reading it from the archive if it hasn't been read yet, or
     * null if the archive isn't experimental.
     */
    auto get_log_shape_dictionary() -> std::shared_ptr<LogShapeDictionaryReader>;

    std::shared_ptr<TimestampDictionaryReader> get_timestamp_dictionary() {
        return m_archive_reader_adaptor->get_timestamp_dictionary();
    }

    std::shared_ptr<SchemaTree> get_schema_tree() { return m_schema_tree; }

    std::shared_ptr<ReaderUtils::SchemaMap> get_schema_map() { return m_schema_map; }

    auto get_range_index() const -> std::vector<RangeIndexEntry> const& {
        return m_archive_reader_adaptor->get_range_index();
    }

    [[nodiscard]] auto get_header() const -> ArchiveHeader const& {
        return m_archive_reader_adaptor->get_header();
    }

    auto get_log_shape_stats() -> clpp::LogShapeStatArray const&;

    auto get_parent_rule_shapes() -> clpp::ParentRuleShapesArray const&;

    /**
     * Writes decoded messages to a file.
     * @param writer
     */
    void store(FileWriter& writer);

    /**
     * Closes the archive.
     */
    void close();

    /**
     * @return The schema ids in the archive. It also defines the order that tables should be
     * read in to avoid seeking backwards.
     */
    [[nodiscard]] std::vector<int32_t> const& get_schema_ids() const { return m_schema_ids; }

    /**
     * @param schema_id
     * @return The number of messages stored under the given schema in the archive.
     * @throw std::out_of_range if `schema_id` is not found in the schema metadata.
     */
    [[nodiscard]] auto get_num_messages_for_schema(int32_t schema_id) const -> uint64_t {
        return m_id_to_schema_metadata.at(schema_id).num_messages();
    }

    void set_projection(std::shared_ptr<search::Projection> projection) {
        m_projection = projection;
    }

    [[nodiscard]] auto get_projection() const -> std::shared_ptr<search::Projection> {
        return m_projection;
    }

    /**
     * @return true if this archive has log ordering information, and false otherwise.
     */
    [[nodiscard]] auto has_log_order() const -> bool { return m_log_event_idx_column_id >= 0; }

    /**
     * @return Whether this archive can contain columns with the deprecated DateString timestamp
     * format.
     */
    [[nodiscard]] auto has_deprecated_timestamp_format() const -> bool {
        return get_header().has_deprecated_timestamp_format();
    }

    /**
     * @param log_event_idx
     * @return The file-level metadata associated with the record at `log_event_idx`.
     * @throws ArchiveReaderAdaptor::OperationFailed when `log_event_idx` cannot be mapped to
     * any metadata.
     */
    [[nodiscard]] auto get_metadata_for_log_event(int64_t log_event_idx) -> nlohmann::json const& {
        return m_archive_reader_adaptor->get_metadata_for_log_event(log_event_idx);
    }

    [[nodiscard]] auto experimental() const -> bool { return m_clpp.has_value(); }

private:
    // Types
    struct Clpp {
        std::shared_ptr<LogShapeDictionaryReader> log_shape_dict;
        std::optional<clpp::LogShapeStatArray> log_shape_stats;
        std::optional<clpp::ParentRuleShapesArray> parent_rule_shapes;
    };

    // Methods
    /**
     * Reads archive metadata and prepares the archive reader for subsequent archive reads.
     */
    auto initialize_archive_reader() -> void;

    /**
     * Reads a single schema table entry from the table metadata stream.
     * @return A result containing a pair:
     * - The schema ID.
     * - The schema metadata with `uncompressed_size` not yet computed.
     * on success, or an error code indicating the failure:
     * - std::errc::io_error if reading from the metadata stream fails.
     * - std::errc::illegal_byte_sequence if the stream offset exceeds the stream size.
     * - Forwards `ReaderUtils::try_uint64_to_size_t`'s return values on failure.
     */
    [[nodiscard]] auto read_single_schema_metadata()
            -> ystdlib::error_handling::Result<std::pair<int32_t, SchemaReader::SchemaMetadata>>;

    /**
     * Initializes a schema reader passed by reference to become a reader for a given schema.
     * @param reader
     * @param schema_id
     * @param should_extract_timestamp
     * @param should_marshal_records
     */
    void initialize_schema_reader(
            SchemaReader& reader,
            int32_t schema_id,
            bool should_extract_timestamp,
            bool should_marshal_records
    );

    /**
     * Appends a column to the schema reader.
     * @param reader
     * @param column_id
     * @return a pointer to the newly appended column reader or nullptr if no column reader was
     * created
     */
    BaseColumnReader* append_reader_column(SchemaReader& reader, int32_t column_id);

    /**
     * Resolves the schema-tree node ID of an unordered object. Returns the root node ID stored in
     * the object's metadata entries if present or finds the matching subtree root of the object's
     * type in `search_root_id`'s subtree.
     * @param obj
     * @param search_root_id The node ID whose subtree is searched for the object's matching subtree
     * root.
     * @return The resolved schema-tree node ID.
     */
    [[nodiscard]] auto
    resolve_unordered_object_root(UnorderedObject const& obj, int32_t search_root_id)
            -> SchemaNode::id_t;

    /**
     * Appends columns for the sub-schema of an unordered object.
     * @param reader
     * @param mst_subtree_root_node_id
     * @param sub_schema
     * @param log_shape_id The log shape ID if sub_schema is a `LogMessage` object.
     * @param should_marshal_records
     */
    auto append_unordered_reader_columns(
            SchemaReader& reader,
            SchemaNode::id_t mst_subtree_root_node_id,
            SchemaView sub_schema,
            std::optional<clpp::log_shape_id_t> log_shape_id,
            bool should_marshal_records
    ) -> void;

    /**
     * Reads a table with given ID from the packed stream reader. If read_stream is called
     * multiple times in a row for the same stream_id a cached buffer is returned. This function
     * allows the caller to ask for the same buffer to be reused to read multiple different
     * tables: this can save memory allocations, but can only be used when tables are read one
     * at a time.
     * @param stream_id
     * @param reuse_buffer when true the same buffer is reused across invocations, overwriting
     * data returned previous calls to read_stream
     * @return a buffer containing the decompressed stream identified by stream_id
     */
    std::shared_ptr<char[]> read_stream(size_t stream_id, bool reuse_buffer);

    /**
     * Reads the log type metadata from the archive.
     * @return The read log type metadata array, or an error code indicating the failure:
     * - Forwards `Array::decompress`'s return values on failure.
     * @throws
     */
    auto read_parent_rule_shapes() -> ystdlib::error_handling::Result<clpp::ParentRuleShapesArray>;

    // Data members
    bool m_is_open;
    std::string m_archive_id;
    std::shared_ptr<VariableDictionaryReader> m_var_dict;
    std::shared_ptr<LogTypeDictionaryReader> m_log_dict;
    std::shared_ptr<LogTypeDictionaryReader> m_array_dict;
    std::shared_ptr<ArchiveReaderAdaptor> m_archive_reader_adaptor;
    std::unordered_set<std::string_view> m_read_sections;

    std::shared_ptr<SchemaTree> m_schema_tree;
    std::shared_ptr<ReaderUtils::SchemaMap> m_schema_map;
    std::vector<int32_t> m_schema_ids;
    std::map<int32_t, SchemaReader::SchemaMetadata> m_id_to_schema_metadata;
    std::shared_ptr<search::Projection> m_projection{
            std::make_shared<search::Projection>(search::Projection::Mode::ReturnAllColumns)
    };

    PackedStreamReader m_stream_reader;
    ZstdDecompressor m_table_metadata_decompressor;
    SchemaReader m_schema_reader;
    std::shared_ptr<char[]> m_stream_buffer{};
    size_t m_stream_buffer_size{0ULL};
    size_t m_cur_stream_id{0ULL};
    int32_t m_log_event_idx_column_id{-1};

    std::optional<Clpp> m_clpp;
    Options m_options;
};
}  // namespace clp_s

#endif  // CLP_S_ARCHIVEREADER_HPP
