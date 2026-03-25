#ifndef CLP_S_ARCHIVEREADER_HPP
#define CLP_S_ARCHIVEREADER_HPP

#include <cstddef>
#include <map>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
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
#include <clp_s/SchemaReader.hpp>
#include <clp_s/search/Projection.hpp>
#include <clp_s/SingleFileArchiveDefs.hpp>
#include <clp_s/TimestampDictionaryReader.hpp>
#include <clpp/LogTypeStat.hpp>

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
        Options() = default;

        Options(NetworkAuthOption network_auth, bool experimental)
                : m_network_auth{network_auth},
                  m_experimental{experimental} {}

        NetworkAuthOption m_network_auth{};
        bool m_experimental{false};
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
     * @param single_file_archive_reader The already opened archive reader
     * @param archive_id The unique name or identifier for the archive
     */
    auto open(
            std::shared_ptr<clp::ReaderInterface> single_file_archive_reader,
            std::string_view archive_id
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
     * Reads the variable dictionary from the archive.
     * @param lazy
     * @return the variable dictionary reader
     */
    std::shared_ptr<VariableDictionaryReader> read_variable_dictionary(bool lazy = false) {
        m_var_dict->read_entries(lazy);
        return m_var_dict;
    }

    /**
     * Reads the log type dictionary from the archive.
     * @param lazy
     * @return the log type dictionary reader
     */
    std::shared_ptr<LogTypeDictionaryReader> read_log_type_dictionary(bool lazy = false) {
        m_log_dict->read_entries(lazy);
        return m_log_dict;
    }

    /**
     * Reads the array dictionary from the archive.
     * @param lazy
     * @return the array dictionary reader
     */
    std::shared_ptr<LogTypeDictionaryReader> read_array_dictionary(bool lazy = false) {
        m_array_dict->read_entries(lazy);
        return m_array_dict;
    }

    /**
     * Reads the experimental log type statistics from the archive.
     */
    auto read_logtype_stats() -> ystdlib::error_handling::Result<void>;

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

    std::shared_ptr<VariableDictionaryReader> get_variable_dictionary() { return m_var_dict; }

    std::shared_ptr<LogTypeDictionaryReader> get_log_type_dictionary() { return m_log_dict; }

    std::shared_ptr<LogTypeDictionaryReader> get_array_dictionary() { return m_array_dict; }

    std::shared_ptr<VariableDictionaryReader> get_typed_log_type_dictionary() {
        return m_typed_log_dict;
    }

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

    void set_projection(std::shared_ptr<search::Projection> projection) {
        m_projection = projection;
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

    auto get_logtype_stats() const -> std::optional<clpp::LogTypeStatArray> const& {
        return m_logtype_stats;
    }

    /**
     * Decodes variable placeholders from `logtype_dict_entry` replacing them with their type names.
     * @param logtype_dict_entry Only supports clp-s entry due to the requirement of experimental
     * features for the type names.
     * @param logtype_stats Contains the type names for all logtypes.
     * @return A result containing a logtype string with variable type names, or an error code
     * indicating the failure:
     * - `std::errc::bad_message` if the logtype information is malformed.
     */
    static auto decode_logtype_with_variable_types(
            LogTypeDictionaryEntry const& logtype_dict_entry,
            clpp::LogTypeStatArray const& logtype_stats
    ) -> ystdlib::error_handling::Result<std::string>;

    /**
     * @param log_event_idx
     * @return The file-level metadata associated with the record at `log_event_idx`.
     * @throws ArchiveReaderAdaptor::OperationFailed when `log_event_idx` cannot be mapped to
     * any metadata.
     */
    [[nodiscard]] auto get_metadata_for_log_event(int64_t log_event_idx) -> nlohmann::json const& {
        return m_archive_reader_adaptor->get_metadata_for_log_event(log_event_idx);
    }

private:
    /**
     * Reads archive metadata and prepares the archive reader for subsequent archive reads.
     * @param options User specified options for reading an archive.
     */
    auto initialize_archive_reader(Options const& options) -> void;

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
     * Appends columns for the entire schema of an unordered object.
     * @param reader
     * @param mst_subtree_root_node_id
     * @param schema_ids
     * @param should_marshal_records
     */
    void append_unordered_reader_columns(
            SchemaReader& reader,
            int32_t mst_subtree_root_node_id,
            std::span<int32_t> schema_ids,
            bool should_marshal_records
    );

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

    bool m_is_open;
    std::string m_archive_id;
    std::shared_ptr<VariableDictionaryReader> m_var_dict;
    std::shared_ptr<LogTypeDictionaryReader> m_log_dict;
    std::shared_ptr<LogTypeDictionaryReader> m_array_dict;
    std::shared_ptr<ArchiveReaderAdaptor> m_archive_reader_adaptor;

    std::shared_ptr<SchemaTree> m_schema_tree;
    std::shared_ptr<ReaderUtils::SchemaMap> m_schema_map;
    std::vector<int32_t> m_schema_ids;
    std::map<int32_t, SchemaReader::SchemaMetadata> m_id_to_schema_metadata;
    std::shared_ptr<search::Projection> m_projection{
            std::make_shared<search::Projection>(search::ProjectionMode::ReturnAllColumns)
    };

    PackedStreamReader m_stream_reader;
    ZstdDecompressor m_table_metadata_decompressor;
    SchemaReader m_schema_reader;
    std::shared_ptr<char[]> m_stream_buffer{};
    size_t m_stream_buffer_size{0ULL};
    size_t m_cur_stream_id{0ULL};
    int32_t m_log_event_idx_column_id{-1};

    std::optional<clpp::LogTypeStatArray> m_logtype_stats;
    std::shared_ptr<VariableDictionaryReader> m_typed_log_dict;
};
}  // namespace clp_s

#endif  // CLP_S_ARCHIVEREADER_HPP
