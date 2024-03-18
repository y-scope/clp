#include "ArchiveReader.hpp"

#include "archive_constants.hpp"
#include "ReaderUtils.hpp"

namespace clp_s {
void ArchiveReader::open(std::string const& archive_path) {
    if (m_is_open) {
        throw OperationFailed(ErrorCodeNotReady, __FILENAME__, __LINE__);
    }
    m_is_open = true;
    m_archive_path = archive_path;

    m_var_dict = ReaderUtils::get_variable_dictionary_reader(m_archive_path);
    m_log_dict = ReaderUtils::get_log_type_dictionary_reader(m_archive_path);
    m_array_dict = ReaderUtils::get_array_dictionary_reader(m_archive_path);
    m_timestamp_dict = ReaderUtils::get_timestamp_dictionary_reader(m_archive_path);

    m_schema_tree = ReaderUtils::read_schema_tree(m_archive_path);
    m_schema_map = ReaderUtils::read_schemas(m_archive_path);

    m_tables_file_reader.open(m_archive_path + constants::cArchiveTablesFile);
    m_table_metadata_file_reader.open(m_archive_path + constants::cArchiveTableMetadataFile);
}

void ArchiveReader::read_metadata() {
    constexpr size_t cDecompressorFileReadBufferCapacity = 64 * 1024;  // 64 KB
    m_table_metadata_decompressor.open(
            m_table_metadata_file_reader,
            cDecompressorFileReadBufferCapacity
    );

    size_t num_schemas;
    if (auto error = m_table_metadata_decompressor.try_read_numeric_value(num_schemas);
        ErrorCodeSuccess != error)
    {
        throw OperationFailed(error, __FILENAME__, __LINE__);
    }

    for (size_t i = 0; i < num_schemas; i++) {
        int32_t schema_id;
        uint64_t num_messages;
        size_t table_offset;

        if (auto error = m_table_metadata_decompressor.try_read_numeric_value(schema_id);
            ErrorCodeSuccess != error)
        {
            throw OperationFailed(error, __FILENAME__, __LINE__);
        }

        if (auto error = m_table_metadata_decompressor.try_read_numeric_value(num_messages);
            ErrorCodeSuccess != error)
        {
            throw OperationFailed(error, __FILENAME__, __LINE__);
        }

        if (auto error = m_table_metadata_decompressor.try_read_numeric_value(table_offset);
            ErrorCodeSuccess != error)
        {
            throw OperationFailed(error, __FILENAME__, __LINE__);
        }

        m_id_to_table_metadata[schema_id] = {num_messages, table_offset};
        m_schema_ids.push_back(schema_id);
    }
    m_table_metadata_decompressor.close();
}

void ArchiveReader::read_dictionaries_and_metadata() {
    m_var_dict->read_new_entries();
    m_log_dict->read_new_entries();
    m_array_dict->read_new_entries();
    m_timestamp_dict->read_new_entries();
    read_metadata();
}

std::unique_ptr<SchemaReader> ArchiveReader::read_table(
        int32_t schema_id,
        bool should_extract_timestamp,
        bool should_marshal_records
) {
    constexpr size_t cDecompressorFileReadBufferCapacity = 64 * 1024;  // 64 KB

    if (m_id_to_table_metadata.count(schema_id) == 0) {
        throw OperationFailed(ErrorCodeFileNotFound, __FILENAME__, __LINE__);
    }

    auto schema_reader
            = create_schema_reader(schema_id, should_extract_timestamp, should_marshal_records);

    m_tables_file_reader.try_seek_from_begin(m_id_to_table_metadata[schema_id].offset);
    m_tables_decompressor.open(m_tables_file_reader, cDecompressorFileReadBufferCapacity);
    schema_reader->load(m_tables_decompressor);
    m_tables_decompressor.close();
    return schema_reader;
}

BaseColumnReader*
ArchiveReader::append_reader_column(std::unique_ptr<SchemaReader>& reader, int32_t column_id) {
    BaseColumnReader* column_reader = nullptr;
    auto node = m_schema_tree->get_node(column_id);
    std::string key_name = node->get_key_name();
    switch (node->get_type()) {
        case NodeType::INTEGER:
            column_reader = new Int64ColumnReader(key_name, column_id);
            break;
        case NodeType::FLOAT:
            column_reader = new FloatColumnReader(key_name, column_id);
            break;
        case NodeType::CLPSTRING:
            column_reader = new ClpStringColumnReader(key_name, column_id, m_var_dict, m_log_dict);
            break;
        case NodeType::VARSTRING:
            column_reader = new VariableStringColumnReader(key_name, column_id, m_var_dict);
            break;
        case NodeType::BOOLEAN:
            column_reader = new BooleanColumnReader(key_name, column_id);
            break;
        case NodeType::ARRAY:
            column_reader = new ClpStringColumnReader(
                    key_name,
                    column_id,
                    m_var_dict,
                    m_array_dict,
                    true
            );
            break;
        case NodeType::DATESTRING:
            column_reader = new DateStringColumnReader(key_name, column_id, m_timestamp_dict);
            break;
        case NodeType::OBJECT:
        case NodeType::NULLVALUE:
            reader->append_column(column_id);
            break;
        case NodeType::UNKNOWN:
            break;
    }

    if (column_reader) {
        reader->append_column(column_reader);
    }
    return column_reader;
}

std::unique_ptr<SchemaReader> ArchiveReader::create_schema_reader(
        int32_t schema_id,
        bool should_extract_timestamp,
        bool should_marshal_records
) {
    auto reader = std::make_unique<SchemaReader>(
            m_schema_tree,
            schema_id,
            m_id_to_table_metadata[schema_id].num_messages,
            should_marshal_records
    );
    auto timestamp_column_ids = m_timestamp_dict->get_authoritative_timestamp_column_ids();

    for (int32_t column_id : (*m_schema_map)[reader->get_schema_id()]) {
        BaseColumnReader* column_reader = append_reader_column(reader, column_id);

        if (should_extract_timestamp && column_reader && timestamp_column_ids.count(column_id) > 0)
        {
            reader->mark_column_as_timestamp(column_reader);
        }
    }
    return reader;
}

void ArchiveReader::store(FileWriter& writer) {
    std::string message;

    for (auto& [id, table_metadata] : m_id_to_table_metadata) {
        auto schema_reader = read_table(id, false, true);
        while (schema_reader->get_next_message(message)) {
            writer.write(message.c_str(), message.length());
        }
    }
}

void ArchiveReader::close() {
    if (false == m_is_open) {
        throw OperationFailed(ErrorCodeNotInit, __FILENAME__, __LINE__);
    }
    m_is_open = false;

    m_var_dict->close();
    m_log_dict->close();
    m_array_dict->close();
    m_timestamp_dict->close();

    m_tables_file_reader.close();
    m_table_metadata_file_reader.close();

    m_id_to_table_metadata.clear();
    m_schema_ids.clear();
}

}  // namespace clp_s
