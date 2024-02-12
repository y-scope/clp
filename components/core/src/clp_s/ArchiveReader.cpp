#include "ArchiveReader.hpp"

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

    m_table_file_reader.open(m_archive_path + "/table");
//    m_table_decompressor.open(m_table_file_reader, 64 * 1024);

    m_metadata_file_reader.open(m_archive_path + "/metadata");
}

void ArchiveReader::load_metadata() {
    m_metadata_decompressor.open(m_metadata_file_reader, 64 * 1024);

    size_t num_schemas;
    if (auto error = m_metadata_decompressor.try_read_numeric_value(num_schemas);
        ErrorCodeSuccess != error)
    {
        throw OperationFailed(error, __FILENAME__, __LINE__);
    }

    for (size_t i = 0; i < num_schemas; i++) {
        int32_t schema_id;
        uint64_t num_messages;
        size_t table_offset;

        if (auto error = m_metadata_decompressor.try_read_numeric_value(schema_id);
            ErrorCodeSuccess != error)
        {
            throw OperationFailed(error, __FILENAME__, __LINE__);
        }

        if (auto error = m_metadata_decompressor.try_read_numeric_value(num_messages);
            ErrorCodeSuccess != error)
        {
            throw OperationFailed(error, __FILENAME__, __LINE__);
        }

        if (auto error = m_metadata_decompressor.try_read_numeric_value(table_offset);
            ErrorCodeSuccess != error)
        {
            throw OperationFailed(error, __FILENAME__, __LINE__);
        }

        m_id_to_table_metadata[schema_id] = {num_messages, table_offset};
        m_schema_ids.push_back(schema_id);
    }
    m_metadata_decompressor.close();
}

void ArchiveReader::load_dictionaries_and_metadata() {
    load_variable_dictionary();
    load_log_type_dictionary();
    load_array_dictionary();
    load_metadata();
}

std::unique_ptr<SchemaReader> ArchiveReader::load_table(int32_t schema_id) {
    constexpr size_t cDecompressorFileReadBufferCapacity = 64 * 1024;  // 64 KB

    if (m_id_to_table_metadata.count(schema_id) == 0) {
        throw OperationFailed(ErrorCodeFileNotFound, __FILENAME__, __LINE__);
    }

    auto schema_reader = std::make_unique<SchemaReader>(
            m_schema_tree,
            schema_id,
            m_id_to_table_metadata[schema_id].num_messages
    );
    append_reader_columns(schema_reader, true);
    m_table_file_reader.try_seek_from_begin(m_id_to_table_metadata[schema_id].offset);
    m_table_decompressor.open(m_table_file_reader, 64 * 1024);
    schema_reader->load(m_table_decompressor);
    m_table_decompressor.close();
    return schema_reader;
}

std::shared_ptr<TimestampDictionaryReader> ArchiveReader::read_timestamp_dictionary() {
    auto reader = std::make_shared<TimestampDictionaryReader>();
    reader->open(m_archive_path + "/timestamp.dict");
    reader->read_local_entries();
    reader->close();

    return reader;
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
            reader->append_column(new BooleanColumnReader(key_name, column_id));
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
        case NodeType::FLOATDATESTRING:
            column_reader = new FloatDateStringColumnReader(key_name, column_id);
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

void ArchiveReader::append_reader_columns(
        std::unique_ptr<SchemaReader>& reader,
        bool extract_timestamp
) {
    auto timestamp_column_ids = m_timestamp_dict->get_authoritative_timestamp_column_ids();

    for (int32_t column_id : (*m_schema_map)[reader->get_schema_id()]) {
        BaseColumnReader* column_reader = append_reader_column(reader, column_id);

        if (extract_timestamp && column_reader && timestamp_column_ids.count(column_id) > 0) {
            reader->mark_column_as_timestamp(column_reader);
        }
    }
}

void ArchiveReader::store(FileWriter& writer) {
    std::string message;

    for (auto& [id, table_metadata] : m_id_to_table_metadata) {
        auto schema_reader = load_table(id);
        while (schema_reader->get_next_message(message)) {
            writer.write(message.c_str(), message.length());
        }
    }
}

void ArchiveReader::close() {
    if (false == m_is_open) {
        throw OperationFailed(ErrorCodeNotInit, __FILENAME__, __LINE__);
    }

    m_var_dict->close();
    m_log_dict->close();
    m_array_dict->close();

    m_table_file_reader.close();
    m_metadata_file_reader.close();

    m_id_to_table_metadata.clear();
}

}  // namespace clp_s
