#ifndef CLP_S_ARCHIVEWRITER_HPP
#define CLP_S_ARCHIVEWRITER_HPP

#include <utility>

#include <boost/filesystem.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "DictionaryWriter.hpp"
#include "Schema.hpp"
#include "SchemaMap.hpp"
#include "SchemaTree.hpp"
#include "SchemaWriter.hpp"
#include "TimestampDictionaryWriter.hpp"

namespace clp_s {
struct ArchiveWriterOption {
    boost::uuids::uuid id;
    std::string archives_dir;
    int compression_level;
    bool print_archive_stats;
};

class ArchiveWriter {
public:
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}
    };

    // Constructor
    ArchiveWriter() : m_encoded_message_size(0UL) {}

    /**
     * Opens the archive writer
     * @param option
     */
    void open(ArchiveWriterOption const& option);

    /**
     * Closes the archive writer
     * @return the compressed size of the archive in bytes
     */
    [[nodiscard]] size_t close();

    /**
     * Appends a message to the archive writer
     * @param schema_id
     * @param schema
     * @param message
     */
    void append_message(int32_t schema_id, Schema const& schema, ParsedMessage& message);

    /**
     * @return Size of the uncompressed data written to the archive
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
     * Updates the archive's metadata
     */
    void update_metadata();

    size_t m_encoded_message_size;

    boost::uuids::uuid m_id{};

    std::string m_archive_path;
    std::string m_encoded_messages_dir;

    std::shared_ptr<VariableDictionaryWriter> m_var_dict;
    std::shared_ptr<LogTypeDictionaryWriter> m_log_dict;
    std::shared_ptr<LogTypeDictionaryWriter> m_array_dict;  // log type dictionary for arrays
    std::shared_ptr<TimestampDictionaryWriter> m_timestamp_dict;
    int m_compression_level{};
    bool m_print_archive_stats{};

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
