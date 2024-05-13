#ifndef CLP_S_SCHEMAWRITER_HPP
#define CLP_S_SCHEMAWRITER_HPP

#include <vector>

#include "ColumnWriter.hpp"
#include "FileWriter.hpp"
#include "ParsedMessage.hpp"
#include "ZstdCompressor.hpp"

namespace clp_s {
class SchemaWriter {
public:
    // Constructor
    SchemaWriter() : m_num_messages(0) {}

    // Destructor
    ~SchemaWriter();

    /**
     * Opens the schema writer.
     * @param path
     * @param compression_level
     */
    void open(std::string path, int compression_level);

    /**
     * Appends a column to the schema writer.
     * @param column_writer
     */
    void append_column(BaseColumnWriter* column_writer);

    /**
     * Appends a message to the schema writer.
     * @param message
     * @return The size of the message in bytes.
     */
    size_t append_message(ParsedMessage& message);

    /**
     * Stores the columns to disk.
     * @param compressor
     * @return the uncompressed in-memory size of the table
     */
    [[nodiscard]] size_t store(ZstdCompressor& compressor);

    /**
     * Closes the schema writer.
     * @return the compressed size of the schema table in bytes
     */
    [[nodiscard]] size_t close();

    uint64_t get_num_messages() const { return m_num_messages; }

private:
    uint64_t m_num_messages;

    std::vector<BaseColumnWriter*> m_columns;
    std::vector<BaseColumnWriter*> m_unordered_columns;
};
}  // namespace clp_s

#endif  // CLP_S_SCHEMAWRITER_HPP
