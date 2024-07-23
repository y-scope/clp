#include "TableReader.hpp"

namespace clp_s {

void TableReader::read_metadata(ZstdDecompressor& decompressor) {
    switch (m_state) {
        case TableReaderState::Uninitialized:
            m_state = TableReaderState::MetadataRead;
            break;
        case TableReaderState::TablesOpened:
            m_state = TableReaderState::TablesOpenedAndMetadataRead;
            break;
        default:
            throw OperationFailed(ErrorCodeNotReady, __FILE__, __LINE__);
    }

    size_t num_tables;
    if (auto error = decompressor.try_read_numeric_value(num_tables); ErrorCodeSuccess != error) {
        throw OperationFailed(error, __FILE__, __LINE__);
    }
    m_table_metadata.reserve(num_tables);

    for (size_t i = 0; i < num_tables; ++i) {
        size_t file_offset;
        size_t uncompressed_size;

        if (auto error = decompressor.try_read_numeric_value(file_offset);
            ErrorCodeSuccess != error)
        {
            throw OperationFailed(error, __FILE__, __LINE__);
        }

        if (auto error = decompressor.try_read_numeric_value(uncompressed_size);
            ErrorCodeSuccess != error)
        {
            throw OperationFailed(error, __FILE__, __LINE__);
        }

        m_table_metadata.emplace_back(file_offset, uncompressed_size);
    }
}

void TableReader::open_tables(std::string const& tables_file_path) {
    switch (m_state) {
        case TableReaderState::Uninitialized:
            m_state = TableReaderState::TablesOpened;
            break;
        case TableReaderState::MetadataRead:
            m_state = TableReaderState::TablesOpenedAndMetadataRead;
            break;
        default:
            throw OperationFailed(ErrorCodeNotReady, __FILE__, __LINE__);
    }
    m_tables_reader.open(tables_file_path);
}

void TableReader::close() {
    switch (m_state) {
        case TableReaderState::TablesOpened:
        case TableReaderState::TablesOpenedAndMetadataRead:
        case TableReaderState::ReadingTables:
            break;
        default:
            throw OperationFailed(ErrorCodeNotReady, __FILE__, __LINE__);
    }
    m_tables_reader.close();
    m_previous_table_id = 0;
    m_table_metadata.clear();
    m_state = TableReaderState::Uninitialized;
}

void TableReader::read_table(size_t table_id, std::shared_ptr<char[]>& buf, size_t& buf_size) {
    constexpr size_t cDecompressorFileReadBufferCapacity = 64 * 1024;  // 64 KB
    if (table_id > m_table_metadata.size()) {
        throw OperationFailed(ErrorCodeCorrupt, __FILE__, __LINE__);
    }

    switch (m_state) {
        case TableReaderState::TablesOpenedAndMetadataRead:
            m_state = TableReaderState::ReadingTables;
            break;
        case TableReaderState::ReadingTables:
            if (m_previous_table_id >= table_id) {
                throw OperationFailed(ErrorCodeBadParam, __FILE__, __LINE__);
            }
            break;
        default:
            throw OperationFailed(ErrorCodeNotReady, __FILE__, __LINE__);
    }
    m_previous_table_id = table_id;

    auto& [file_offset, uncompressed_size] = m_table_metadata[table_id];
    m_tables_reader.try_seek_from_begin(file_offset);
    m_tables_decompressor.open(m_tables_reader, cDecompressorFileReadBufferCapacity);
    if (buf_size < uncompressed_size) {
        // make_shared is supposed to work here for c++20, but it seems like the compiler version
        // we use doesn't support it, so we convert a unique_ptr to a shared_ptr instead.
        buf = std::make_unique<char[]>(uncompressed_size);
        buf_size = uncompressed_size;
    }
    if (auto error = m_tables_decompressor.try_read_exact_length(buf.get(), uncompressed_size);
        ErrorCodeSuccess != error)
    {
        throw OperationFailed(error, __FILE__, __LINE__);
    }
    m_tables_decompressor.close_for_reuse();
}
}  // namespace clp_s
