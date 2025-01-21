#ifndef CLP_S_ARCHIVEWRITER_HPP
#define CLP_S_ARCHIVEWRITER_HPP

#include <string_view>
#include <utility>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "../clp/GlobalMySQLMetadataDB.hpp"
#include "DictionaryWriter.hpp"
#include "Schema.hpp"
#include "SchemaMap.hpp"
#include "SchemaTree.hpp"
#include "SchemaWriter.hpp"
#include "SingleFileArchiveDefs.hpp"
#include "TimestampDictionaryWriter.hpp"

namespace clp_s {
struct ArchiveWriterOption {
    boost::uuids::uuid id;
    std::string archives_dir;
    int compression_level;
    bool print_archive_stats;
    bool single_file_archive;
    size_t min_table_size;
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
    explicit ArchiveWriter(std::shared_ptr<clp::GlobalMySQLMetadataDB> metadata_db)
            : m_metadata_db(std::move(metadata_db)) {}

    // Destructor
    ~ArchiveWriter() = default;

    /**
     * Opens the archive writer
     * @param option
     */
    void open(ArchiveWriterOption const& option);

    /**
     * Closes the archive writer
     */
    void close();

    /**
     * Appends a message to the archive writer
     * @param schema_id
     * @param schema
     * @param message
     */
    void append_message(int32_t schema_id, Schema const& schema, ParsedMessage& message);

    /**
     * Adds a node to the schema tree
     * @param parent_node_id
     * @param type
     * @param key
     * @return the node id
     */
    int32_t add_node(int parent_node_id, NodeType type, std::string_view const key) {
        return m_schema_tree.add_node(parent_node_id, type, key);
    }

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
     * Ingests a timestamp entry from a string
     * @param key
     * @param node_id
     * @param timestamp
     * @param pattern_id
     * @return the epoch time corresponding to the string timestamp
     */
    epochtime_t ingest_timestamp_entry(
            std::string_view key,
            int32_t node_id,
            std::string_view timestamp,
            uint64_t& pattern_id
    ) {
        return m_timestamp_dict.ingest_entry(key, node_id, timestamp, pattern_id);
    }

    /**
     * Ingests a timestamp entry from a number
     * @param column_key
     * @param node_id
     * @param timestamp
     */
    void ingest_timestamp_entry(std::string_view key, int32_t node_id, double timestamp) {
        m_timestamp_dict.ingest_entry(key, node_id, timestamp);
    }

    void ingest_timestamp_entry(std::string_view key, int32_t node_id, int64_t timestamp) {
        m_timestamp_dict.ingest_entry(key, node_id, timestamp);
    }

    /**
     * Increments the size of the original (uncompressed) logs ingested into the archive. This size
     * tracks the raw input size before any encoding or compression.
     * @param size
     */
    void increment_uncompressed_size(size_t size) { m_uncompressed_size += size; }

    /**
     * @return The total size of the encoded (uncompressed) data written to the archive. This
     * reflects the size of the data after encoding but before compression.
     * TODO: Add the size of schema tree, schema map and timestamp dictionary
     */
    size_t get_data_size();

private:
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
     * Writes the archive to a single file
     * @param files
     */
    void write_single_file_archive(std::vector<ArchiveFileInfo> const& files);

    /**
     * Writes the metadata section of an archive.
     * @param archive_writer
     * @param files
     */
    void
    write_archive_metadata(FileWriter& archive_writer, std::vector<ArchiveFileInfo> const& files);

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

    /**
     * Updates the metadata db with the archive's metadata (id, size, timestamp ranges, etc.)
     */
    void update_metadata_db();

    /**
     * Prints the archive's statistics (id, uncompressed size, compressed size, etc.)
     */
    void print_archive_stats();

    static constexpr size_t cReadBlockSize = 4 * 1024;

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
    std::shared_ptr<clp::GlobalMySQLMetadataDB> m_metadata_db;
    int m_compression_level{};
    bool m_print_archive_stats{};
    bool m_single_file_archive{};
    size_t m_min_table_size{};

    SchemaMap m_schema_map;
    SchemaTree m_schema_tree;

    std::map<int32_t, SchemaWriter*> m_id_to_schema_writer;

    FileWriter m_tables_file_writer;
    FileWriter m_table_metadata_file_writer;
    ZstdCompressor m_tables_compressor;
    ZstdCompressor m_table_metadata_compressor;
};
}  // namespace clp_s

#endif  // CLP_S_ARCHIVEWRITER_HPP
