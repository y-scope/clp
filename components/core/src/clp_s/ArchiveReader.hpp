#ifndef CLP_S_ARCHIVEREADER_HPP
#define CLP_S_ARCHIVEREADER_HPP

#include <map>
#include <set>
#include <utility>

#include <boost/filesystem.hpp>

#include "DictionaryReader.hpp"
#include "ReaderUtils.hpp"
#include "SchemaReader.hpp"
#include "TimestampDictionaryReader.hpp"

namespace clp_s {
class ArchiveReader {
public:
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}
    };

    // Constructor
    ArchiveReader(
            std::shared_ptr<SchemaTree> schema_tree,
            std::shared_ptr<ReaderUtils::SchemaMap> schema_map,
            std::shared_ptr<TimestampDictionaryReader> timestamp_dict
    )
            : m_is_open(false),
              m_schema_tree(std::move(schema_tree)),
              m_schema_map(std::move(schema_map)),
              m_timestamp_dict(std::move(timestamp_dict)) {}

    /**
     * Opens an archive for reading.
     * @param option
     */
    void open(std::string const& archive_path);

    /**
     * Reads the dictionaries and metadata.
     */
    void read_dictionaries_and_metadata();

    /**
     * Reads the variable dictionary from the archive.
     * @param lazy
     * @return the variable dictionary reader
     */
    std::shared_ptr<VariableDictionaryReader> read_variable_dictionary(bool lazy = false) {
        m_var_dict->read_new_entries(lazy);
        return m_var_dict;
    }

    /**
     * Reads the log type dictionary from the archive.
     * @param lazy
     * @return the log type dictionary reader
     */
    std::shared_ptr<LogTypeDictionaryReader> read_log_type_dictionary(bool lazy = false) {
        m_log_dict->read_new_entries(lazy);
        return m_log_dict;
    }

    /**
     * Reads the array dictionary from the archive.
     * @param lazy
     * @return the array dictionary reader
     */
    std::shared_ptr<LogTypeDictionaryReader> read_array_dictionary(bool lazy = false) {
        m_array_dict->read_new_entries(lazy);
        return m_array_dict;
    }

    /**
     * Reads the metadata from the archive.
     */
    void read_metadata();

    /**
     * Reads the local timestamp dictionary from the archive.
     * @return the timestamp dictionary reader
     */
    std::shared_ptr<TimestampDictionaryReader> read_timestamp_dictionary();

    /**
     * Reads a table from the archive.
     * @param schema_id
     * @param should_extract_timestamp
     * @return the schema reader
     */
    std::unique_ptr<SchemaReader> read_table(int32_t schema_id, bool should_extract_timestamp);

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
     * @return The schema ids in the archive.
     */
    [[nodiscard]] std::vector<int32_t> const& get_schema_ids() const { return m_schema_ids; }

private:
    /**
     * Creates a schema reader for a given schema.
     * @param schema_id
     * @param extract_timestamp
     */
    std::unique_ptr<SchemaReader>
    create_schema_reader(int32_t schema_id, bool should_extract_timestamp);

    /**
     * Appends a column to the schema reader.
     * @param reader
     * @param column_id
     */
    BaseColumnReader*
    append_reader_column(std::unique_ptr<SchemaReader>& reader, int32_t column_id);

    bool m_is_open;
    std::string m_archive_path;

    std::shared_ptr<VariableDictionaryReader> m_var_dict;
    std::shared_ptr<LogTypeDictionaryReader> m_log_dict;
    std::shared_ptr<LogTypeDictionaryReader> m_array_dict;
    std::shared_ptr<TimestampDictionaryReader> m_timestamp_dict;

    std::shared_ptr<SchemaTree> m_schema_tree;
    std::shared_ptr<ReaderUtils::SchemaMap> m_schema_map;
    std::vector<int32_t> m_schema_ids;
    std::map<int32_t, SchemaReader::TableMetadata> m_id_to_table_metadata;

    FileReader m_table_file_reader;
    FileReader m_metadata_file_reader;
    ZstdDecompressor m_table_decompressor;
    ZstdDecompressor m_metadata_decompressor;
};
}  // namespace clp_s

#endif  // CLP_S_ARCHIVEREADER_HPP
