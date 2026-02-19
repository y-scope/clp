#ifndef CLP_S_SCHEMAWRITER_HPP
#define CLP_S_SCHEMAWRITER_HPP

#include <memory>
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
    ~SchemaWriter() = default;

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
    void append_column(std::unique_ptr<BaseColumnWriter> column_writer);

    /**
     * Appends a message to the schema writer.
     * @param message
     * @return The size of the message in bytes.
     */
    size_t append_message(ParsedMessage& message);

    /**
     * Stores the columns to disk.
     * @param compressor
     */
    void store(ZstdCompressor& compressor);

    uint64_t get_num_messages() const { return m_num_messages; }

    /**
     * @return the uncompressed in-memory size of the data that will be written to the compressor
     */
    size_t get_total_uncompressed_size() const { return m_total_uncompressed_size; }

private:
    uint64_t m_num_messages;
    size_t m_total_uncompressed_size{};

    std::vector<std::unique_ptr<BaseColumnWriter>> m_columns;
};
}  // namespace clp_s

#endif  // CLP_S_SCHEMAWRITER_HPP
