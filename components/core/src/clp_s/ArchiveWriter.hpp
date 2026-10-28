#ifndef CLP_S_ARCHIVEWRITER_HPP
#define CLP_S_ARCHIVEWRITER_HPP

#include <concepts>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <nlohmann/json.hpp>
#include <ystdlib/error_handling/Result.hpp>

#include <clp/Defs.h>
#include <clp/streaming_archive/Constants.hpp>
#include <clp_s/archive_constants.hpp>
#include <clp_s/Defs.hpp>
#include <clp_s/DictionaryEntry.hpp>
#include <clp_s/DictionaryWriter.hpp>
#include <clp_s/ParsedMessage.hpp>
#include <clp_s/RangeIndexWriter.hpp>
#include <clp_s/Schema.hpp>
#include <clp_s/SchemaMap.hpp>
#include <clp_s/SchemaTree.hpp>
#include <clp_s/SchemaWriter.hpp>
#include <clp_s/SingleFileArchiveDefs.hpp>
#include <clp_s/TimestampDictionaryWriter.hpp>
#include <clpp/Defs.hpp>
#include <clpp/LogShapeStats.hpp>
#include <clpp/ParentRuleShapes.hpp>
#include <clpp/TextShape.hpp>

namespace clp_s {
struct ArchiveWriterOption {
    boost::uuids::uuid id;
    std::string archives_dir;
    int compression_level;
    bool print_archive_stats;
    bool single_file_archive;
    size_t min_table_size;
    std::vector<std::string> authoritative_timestamp;
    std::string authoritative_timestamp_namespace;
    ClpFlavour flavour{ClpFlavour::Clps};
    std::string parsing_spec;
};

class ArchiveStats {
public:
    // Constructors
    explicit ArchiveStats(
            std::string id,
            epochtime_t begin_timestamp,
            epochtime_t end_timestamp,
            size_t uncompressed_size,
            size_t compressed_size,
            nlohmann::json range_index,
            bool is_split
    )
            : m_id{id},
              m_begin_timestamp{begin_timestamp},
              m_end_timestamp{end_timestamp},
              m_uncompressed_size{uncompressed_size},
              m_compressed_size{compressed_size},
              m_range_index(std::move(range_index)),  // Avoid {} to prevent wrapping in JSON array.
              m_is_split{is_split} {}

    // Methods
    /**
     * @return The contents of `ArchiveStats` as a JSON object in a string.
     */
    [[nodiscard]] auto as_string() const -> std::string {
        namespace Archive = clp::streaming_archive::cMetadataDB::Archive;
        namespace File = clp::streaming_archive::cMetadataDB::File;
        constexpr std::string_view cRangeIndex{"range_index"};

        nlohmann::json json_msg
                = {{Archive::Id, m_id},
                   {Archive::BeginTimestamp, m_begin_timestamp},
                   {Archive::EndTimestamp, m_end_timestamp},
                   {Archive::UncompressedSize, m_uncompressed_size},
                   {Archive::Size, m_compressed_size},
                   {File::IsSplit, m_is_split},
                   {cRangeIndex, m_range_index}};
        return json_msg.dump(-1, ' ', false, nlohmann::json::error_handler_t::ignore);
    }

    [[nodiscard]] auto get_id() const -> std::string const& { return m_id; }

    [[nodiscard]] auto get_begin_timestamp() const -> epochtime_t { return m_begin_timestamp; }

    [[nodiscard]] auto get_end_timestamp() const -> epochtime_t { return m_end_timestamp; }

    [[nodiscard]] auto get_uncompressed_size() const -> size_t { return m_uncompressed_size; }

    [[nodiscard]] auto get_compressed_size() const -> size_t { return m_compressed_size; }

    [[nodiscard]] auto get_range_index() const -> nlohmann::json const& { return m_range_index; }

    [[nodiscard]] auto get_is_split() const -> bool { return m_is_split; }

private:
    std::string m_id;
    epochtime_t m_begin_timestamp{};
    epochtime_t m_end_timestamp{};
    size_t m_uncompressed_size{};
    size_t m_compressed_size{};
    nlohmann::json m_range_index;
    bool m_is_split{};
};

class ArchiveWriter {
public:
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}
    };

    struct StreamMetadata {
        StreamMetadata(uint64_t file_offset, uint64_t uncompressed_size)
                : file_offset(file_offset),
                  uncompressed_size(uncompressed_size) {}

        uint64_t file_offset{};
        uint64_t uncompressed_size{};
    };

    struct SchemaMetadata {
        SchemaMetadata(
                uint64_t stream_id,
                uint64_t stream_offset,
                int32_t schema_id,
                uint64_t num_messages
        )
                : stream_id(stream_id),
                  stream_offset(stream_offset),
                  schema_id(schema_id),
                  num_messages(num_messages) {}

        uint64_t stream_id{};
        uint64_t stream_offset{};
        int32_t schema_id{};
        uint64_t num_messages{};
    };

    // Constructor
    ArchiveWriter() = default;

    // Destructor
    ~ArchiveWriter() = default;

    /**
     * Opens the archive writer
     * @param option
     * @throws OperationFailed (ErrorCodeBadParam) if using CLP+ and `option.parsing_spec` is empty.
     */
    void open(ArchiveWriterOption const& option);

    /**
     * Closes the archive writer, storing the archive's sections in the order they must be read in
     * a single-file archive: schema tree, schema map, table metadata, parsing spec (clpp only),
     * variable dictionary, log type or log shape dictionary, array dictionary, parent rule shapes
     * and log shape stats (clpp only), and finally the encoded record tables. Each section is
     * stored before the sections that depend on it, so a reader never has to seek backwards.
     * @param is_split Whether the last file ingested into the archive is split.
     * @return Statistics for the newly-written archive.
     */
    [[nodiscard]] auto close(bool is_split = false) -> ArchiveStats;

    /**
     * Appends a message to the archive writer
     * @param schema_id
     * @param schema
     * @param message
     */
    void append_message(int32_t schema_id, Schema const& schema, ParsedMessage& message);

    /**
     * Adds a node to the schema tree and attempts to resolve the node against the authoritative
     * timestamp key.
     * @param parent_node_id
     * @param type
     * @param key
     * @return the node id
     */
    auto add_node(mpt_node_id_t parent_node_id, NodeType type, std::string_view key)
            -> mpt_node_id_t;

    /**
     * Checks if a leaf key with a given parent node id matches the authoritative timestamp column.
     * @param parent_node_id
     * @param key
     * @return true if this leaf node would match the authoritative timestamp and false otherwise
     */
    bool matches_timestamp(int parent_node_id, std::string_view key);

    /**
     * @return The Id that will be assigned to the next log event when appended to the archive.
     */
    int64_t get_next_log_event_id() const { return m_next_log_event_id; }

    /**
     * Return a schema's Id and add the schema to the
     * schema map if it does not already exist.
     * @param schema
     * @return the Id of the schema
     */
    int32_t add_schema(Schema const& schema) { return m_schema_map.add_schema(schema); }

    /**
     * Ingests a timestamp entry from a string.
     * @param key
     * @param node_id
     * @param timestamp
     * @param is_json_literal
     * @return Forwards `TimestampDictionaryWriter::ingest_string_timestamp`'s return values.
     */
    [[nodiscard]] auto ingest_string_timestamp(
            std::string_view key,
            int32_t node_id,
            std::string_view timestamp,
            bool is_json_literal
    ) -> std::pair<epochtime_t, uint64_t> {
        return m_timestamp_dict.ingest_string_timestamp(key, node_id, timestamp, is_json_literal);
    }

    /**
     * Ingests a numeric JSON entry.
     * @param key
     * @param node_id
     * @param timestamp
     * @return Forwards `TimestampDictionaryWriter::ingest_numeric_json_timestamp`'s return values.
     */
    [[nodiscard]] auto
    ingest_numeric_json_timestamp(std::string_view key, int32_t node_id, std::string_view timestamp)
            -> std::pair<epochtime_t, uint64_t> {
        return m_timestamp_dict.ingest_numeric_json_timestamp(key, node_id, timestamp);
    }

    /**
     * Ingests an unknown precision epoch timestamp.
     * @param key
     * @param node_id
     * @param timestamp
     * @return Forwards `TimestampDictionaryWriter::ingest_unknown_precision_epoch_timestamp`'s
     * return values.
     */
    [[nodiscard]] auto ingest_unknown_precision_epoch_timestamp(
            std::string_view key,
            int32_t node_id,
            int64_t timestamp
    ) -> std::pair<epochtime_t, uint64_t> {
        return m_timestamp_dict.ingest_unknown_precision_epoch_timestamp(key, node_id, timestamp);
    }

    /**
     * Increments the size of the original (uncompressed) logs ingested into the archive. This size
     * tracks the raw input size before any encoding or compression.
     * @param size
     */
    void increment_uncompressed_size(size_t size) { m_uncompressed_size += size; }

    /**
     * @return The total size of the encoded (uncompressed) data written to the archive. This
     * reflects the size of the dictionaries, encoded messages, and (for CLP+) the log shape stats,
     * parent rule shapes, and parsing specification after encoding but before compression.
     * TODO: Add the size of schema tree, schema map and timestamp dictionary
     */
    [[nodiscard]] auto get_data_size() const -> size_t;

    /**
     * Adds a metadata key value pair to the current range in the range index, opening a range if no
     * range currently exists.
     * @param key
     * @param value
     * @return ErrorCodeSuccess on success or the relevant error code on failure.
     */
    template <typename T>
    [[nodiscard]] auto add_field_to_current_range(std::string const& key, T const& value)
            -> ErrorCode {
        if (false == m_range_open) {
            if (auto const rc = m_range_index_writer.open_range(m_next_log_event_id);
                ErrorCodeSuccess != rc)
            {
                return rc;
            }
            m_range_open = true;
        }
        return m_range_index_writer.add_value_to_range(key, value);
    }

    /**
     * Closes the currently open range in the range index.
     * @return ErrorCodeSuccess on success or the relevant error code on failure.
     */
    [[nodiscard]] auto close_current_range() -> ErrorCode {
        auto const rc = m_range_index_writer.close_range(m_next_log_event_id);
        if (ErrorCodeSuccess == rc) {
            m_range_open = false;
        }
        return rc;
    }

    /**
     * Updates the log shape dictionary for the given log shape, adding it to the dictionary if
     * necessary.
     * @param log_shape
     * @return A result containing a pair or the error code indicating the failure:
     * The pair:
     * - The log shape ID.
     * - True if the log shape is a new entry in the dictionary, false otherwise.
     * The possible error codes:
     * - ClppErrorCodeEnum::Unsupported if the archive isn't clpp-flavour.
     */
    auto update_log_shape_dict(clpp::TextShape<std::string> const& log_shape)
            -> ystdlib::error_handling::Result<std::pair<clpp::log_shape_id_t, bool>>;

    /**
     * Records the parent rule shapes for the most recently added log shape. Must be called exactly
     * once for every new log shape reported by `update_log_shape_dict`.
     * @param shapes
     * @return Success, or an error code indicating the failure:
     * - ClppErrorCodeEnum::Unsupported if the archive isn't clpp-flavour.
     * @throws Forwards `clpp::ParentRuleShapesArray::add_log_shape`'s exceptions.
     */
    auto update_parent_rule_shapes(clpp::ParentRuleShapes shapes)
            -> ystdlib::error_handling::Result<void>;

private:
    // Types
    struct Clpp {
        std::shared_ptr<VariableDictionaryWriter> log_shape_dict;
        std::string parsing_spec;
        clpp::LogShapeStatsArray log_shape_stats_array;
        clpp::ParentRuleShapesArray parent_rule_shapes_array;
    };

    // Methods
    /**
     * Initializes the schema writer
     * @param writer
     * @param schema
     */
    void initialize_schema_writer(SchemaWriter* writer, Schema const& schema);

    /**
     * Compresses and stores the tables.
     * @return A pair containing:
     *         - The size of the compressed table metadata in bytes.
     *         - The size of the compressed tables in bytes.
     */
    [[nodiscard]] std::pair<size_t, size_t> store_tables();

    /**
     * Compresses, stores, and clears the log shape statistics.
     * @param clpp
     * @return The compressed size in bytes.
     */
    [[nodiscard]] auto close_log_shape_stats(Clpp& clpp) -> size_t;

    /**
     * Compresses, stores, and clears the parent rule shapes.
     * @param clpp
     * @return The compressed size in bytes.
     */
    [[nodiscard]] auto close_parent_rule_shapes(Clpp& clpp) -> size_t;

    /**
     * Compresses and stores the parsing specification to the archive.
     * @param parsing_spec
     * @return The compressed size in bytes.
     */
    [[nodiscard]] auto store_parsing_spec(std::string_view parsing_spec) -> size_t;

    /**
     * Creates a Zstd-compressed file in the archive directory and writes its contents.
     * @tparam WriteContentsFn Invocable with a `ZstdCompressor&`.
     * @param file_name The file's name relative to the archive directory.
     * @param write_contents Writes the file's uncompressed contents to the given compressor.
     * @return The compressed size in bytes.
     */
    template <std::invocable<ZstdCompressor&> WriteContentsFn>
    [[nodiscard]] auto
    store_compressed_file(std::string_view file_name, WriteContentsFn const& write_contents)
            -> size_t;

    /**
     * Writes the archive to a single file
     * @param files
     * @return The archive range index as a JSON object.
     */
    [[nodiscard]] auto write_single_file_archive(std::vector<ArchiveFileInfo> const& files)
            -> nlohmann::json;

    /**
     * Writes the metadata section of an archive.
     * @param archive_writer
     * @param files
     * @return The archive range index as a JSON object.
     */
    [[nodiscard]] auto
    write_archive_metadata(FileWriter& archive_writer, std::vector<ArchiveFileInfo> const& files)
            -> nlohmann::json;

    /**
     * Writes the file section of the single file archive
     * @param archive_writer
     * @param files
     */
    void write_archive_files(FileWriter& archive_writer, std::vector<ArchiveFileInfo> const& files);

    /**
     * Writes the header section of the single file archive
     * @param archive_writer
     * @param metadata_section_size
     */
    void write_archive_header(FileWriter& archive_writer, size_t metadata_section_size);

    static constexpr size_t cReadBlockSize = 4 * 1024;

    // Data members
    size_t m_encoded_message_size{};
    size_t m_uncompressed_size{};
    size_t m_compressed_size{};
    int64_t m_next_log_event_id{};

    std::string m_id;

    std::string m_archives_dir;
    std::string m_archive_path;
    std::string m_encoded_messages_dir;

    std::shared_ptr<VariableDictionaryWriter> m_var_dict;
    std::shared_ptr<LogTypeDictionaryWriter> m_log_dict;
    std::shared_ptr<LogTypeDictionaryWriter> m_array_dict;  // log type dictionary for arrays
    TimestampDictionaryWriter m_timestamp_dict;
    int m_compression_level{};
    bool m_print_archive_stats{};
    bool m_single_file_archive{};
    size_t m_min_table_size{};

    std::vector<std::string> m_authoritative_timestamp;
    std::string m_authoritative_timestamp_namespace;
    size_t m_matched_timestamp_prefix_length{0ULL};
    int32_t m_matched_timestamp_prefix_node_id{constants::cRootNodeId};

    SchemaMap m_schema_map;
    SchemaTree m_schema_tree;

    std::map<int32_t, std::unique_ptr<SchemaWriter>> m_id_to_schema_writer;

    FileWriter m_tables_file_writer;
    FileWriter m_table_metadata_file_writer;
    ZstdCompressor m_tables_compressor;
    ZstdCompressor m_table_metadata_compressor;

    RangeIndexWriter m_range_index_writer;
    bool m_range_open{false};

    std::optional<Clpp> m_clpp;
};
}  // namespace clp_s

#endif  // CLP_S_ARCHIVEWRITER_HPP
