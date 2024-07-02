#ifndef CLP_S_TABLEREADER_HPP
#define CLP_S_TABLEREADER_HPP

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "FileReader.hpp"
#include "ZstdDecompressor.hpp"

namespace clp_s {
/**
 * TableReader ensures that the tables section of an archive is read safely. Any attempt to read the
 * tables section without loading the tables metadata, and any attempt to read tables section out of
 * order will throw. As well, any incorrect usage of this class (e.g. closing without opening) will
 * throw.
 */
class TableReader {
public:
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}
    };

    struct TableMetadata {
        size_t file_offset;
        size_t uncompressed_size;
    };

    /**
     * Reads table metadata from the provided compression stream. Must be invoked before reading
     * tables.
     */
    void read_metadata(ZstdDecompressor& decompressor);

    /**
     * Opens a file reader for the tables section. Must be invoked before reading tables.
     */
    void open_tables(std::string const& tables_file_path);

    /**
     * Closes the file reader for the tables section.
     */
    void close();

    /**
     * Decompresses a table with a given table_id and returns it. This function must be called
     * strictly in ascending table_id order. If this function is called twice for the same table or
     * if a table with lower id is requested after a table with higher id then an error is thrown.
     *
     * Note: the buffer and buffer size are returned by reference. This is to support the use case
     * where the caller wants to re-use the same buffer for multiple tables to avoid allocations
     * when they already have a sufficiently large buffer. If no buffer is provided or the provided
     * buffer is too small calling read_table will create a buffer exactly as large as the table
     * being decompressed.
     *
     * @param table_id
     * @param buf
     * @param buf_size
     * @return a shared_ptr to a buffer containing the requested table
     */
    void read_table(size_t table_id, std::shared_ptr<char[]>& buf, size_t& buf_size);

    size_t get_uncompressed_table_size(size_t table_id) const {
        return m_table_metadata.at(table_id).uncompressed_size;
    }

private:
    enum TableReaderState {
        Uninitialized,
        MetadataRead,
        TablesOpened,
        TablesOpenedAndMetadataRead,
        ReadingTables
    };

    std::vector<TableMetadata> m_table_metadata;
    FileReader m_tables_reader;
    ZstdDecompressor m_tables_decompressor;
    TableReaderState m_state{TableReaderState::Uninitialized};
    size_t m_previous_table_id{0ULL};
};

}  // namespace clp_s

#endif  // CLP_S_TABLEREADER_HPP
