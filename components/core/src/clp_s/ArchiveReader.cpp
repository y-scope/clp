#include "ArchiveReader.hpp"

#include <filesystem>
#include <string_view>

#include "archive_constants.hpp"
#include "ReaderUtils.hpp"

using std::string_view;

namespace clp_s {
void ArchiveReader::open(string_view archives_dir, string_view archive_id) {
    if (m_is_open) {
        throw OperationFailed(ErrorCodeNotReady, __FILENAME__, __LINE__);
    }
    m_is_open = true;
    m_archive_id = archive_id;
    std::filesystem::path archive_path{archives_dir};
    archive_path /= m_archive_id;
    auto const archive_path_str = archive_path.string();

    m_var_dict = ReaderUtils::get_variable_dictionary_reader(archive_path_str);
    m_log_dict = ReaderUtils::get_log_type_dictionary_reader(archive_path_str);
    m_array_dict = ReaderUtils::get_array_dictionary_reader(archive_path_str);
    m_timestamp_dict = ReaderUtils::get_timestamp_dictionary_reader(archive_path_str);

    m_schema_tree = ReaderUtils::read_schema_tree(archive_path_str);
    m_schema_map = ReaderUtils::read_schemas(archive_path_str);

    m_tables_file_reader.open(archive_path_str + constants::cArchiveTablesFile);
    m_table_metadata_file_reader.open(archive_path_str + constants::cArchiveTableMetadataFile);
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
        size_t uncompressed_size;

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

        if (auto error = m_table_metadata_decompressor.try_read_numeric_value(uncompressed_size);
            ErrorCodeSuccess != error)
        {
            throw OperationFailed(error, __FILENAME__, __LINE__);
        }

        m_id_to_table_metadata[schema_id] = {num_messages, table_offset, uncompressed_size};
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

SchemaReader& ArchiveReader::read_table(
        int32_t schema_id,
        bool should_extract_timestamp,
        bool should_marshal_records
) {
    constexpr size_t cDecompressorFileReadBufferCapacity = 64 * 1024;  // 64 KB

    if (m_id_to_table_metadata.count(schema_id) == 0) {
        throw OperationFailed(ErrorCodeFileNotFound, __FILENAME__, __LINE__);
    }

    initialize_schema_reader(
            m_schema_reader,
            schema_id,
            should_extract_timestamp,
            should_marshal_records
    );

    m_tables_file_reader.try_seek_from_begin(m_id_to_table_metadata[schema_id].offset);
    m_tables_decompressor.open(m_tables_file_reader, cDecompressorFileReadBufferCapacity);
    m_schema_reader.load(
            m_tables_decompressor,
            m_id_to_table_metadata[schema_id].uncompressed_size
    );
    m_tables_decompressor.close_for_reuse();
    return m_schema_reader;
}

std::vector<std::shared_ptr<SchemaReader>> ArchiveReader::read_all_tables() {
    constexpr size_t cDecompressorFileReadBufferCapacity = 64 * 1024;  // 64 KB

    std::vector<std::shared_ptr<SchemaReader>> readers;
    readers.reserve(m_id_to_table_metadata.size());
    for (auto const& [id, table_metadata] : m_id_to_table_metadata) {
        auto schema_reader = std::make_shared<SchemaReader>();
        initialize_schema_reader(*schema_reader, id, true, true);

        m_tables_file_reader.try_seek_from_begin(table_metadata.offset);
        m_tables_decompressor.open(m_tables_file_reader, cDecompressorFileReadBufferCapacity);
        schema_reader->load(m_tables_decompressor, table_metadata.uncompressed_size);
        m_tables_decompressor.close_for_reuse();

        readers.push_back(std::move(schema_reader));
    }
    return readers;
}

BaseColumnReader* ArchiveReader::append_reader_column(SchemaReader& reader, int32_t column_id) {
    BaseColumnReader* column_reader = nullptr;
    auto const& node = m_schema_tree->get_node(column_id);
    switch (node.get_type()) {
        case NodeType::Integer:
            column_reader = new Int64ColumnReader(column_id);
            break;
        case NodeType::Float:
            column_reader = new FloatColumnReader(column_id);
            break;
        case NodeType::ClpString:
            column_reader = new ClpStringColumnReader(column_id, m_var_dict, m_log_dict);
            break;
        case NodeType::VarString:
            column_reader = new VariableStringColumnReader(column_id, m_var_dict);
            break;
        case NodeType::Boolean:
            column_reader = new BooleanColumnReader(column_id);
            break;
        case NodeType::UnstructuredArray:
            column_reader = new ClpStringColumnReader(column_id, m_var_dict, m_array_dict, true);
            break;
        case NodeType::DateString:
            column_reader = new DateStringColumnReader(column_id, m_timestamp_dict);
            break;
        // No need to push columns without associated object readers into the SchemaReader.
        case NodeType::Object:
        case NodeType::NullValue:
        case NodeType::StructuredArray:
        case NodeType::Unknown:
            break;
    }

    if (column_reader) {
        reader.append_column(column_reader);
    }
    return column_reader;
}

void ArchiveReader::append_unordered_reader_columns(
        SchemaReader& reader,
        int32_t mst_subtree_root_node_id,
        std::span<int32_t> schema_ids,
        bool should_marshal_records
) {
    size_t object_begin_pos = reader.get_column_size();
    for (int32_t column_id : schema_ids) {
        if (Schema::schema_entry_is_unordered_object(column_id)) {
            continue;
        }
        BaseColumnReader* column_reader = nullptr;
        auto const& node = m_schema_tree->get_node(column_id);
        switch (node.get_type()) {
            case NodeType::Integer:
                column_reader = new Int64ColumnReader(column_id);
                break;
            case NodeType::Float:
                column_reader = new FloatColumnReader(column_id);
                break;
            case NodeType::ClpString:
                column_reader = new ClpStringColumnReader(column_id, m_var_dict, m_log_dict);
                break;
            case NodeType::VarString:
                column_reader = new VariableStringColumnReader(column_id, m_var_dict);
                break;
            case NodeType::Boolean:
                column_reader = new BooleanColumnReader(column_id);
                break;
            // UnstructuredArray and DateString currently aren't supported as part of any unordered
            // object, so we disregard them here
            case NodeType::UnstructuredArray:
            case NodeType::DateString:
            // No need to push columns without associated object readers into the SchemaReader.
            case NodeType::StructuredArray:
            case NodeType::Object:
            case NodeType::NullValue:
            case NodeType::Unknown:
                break;
        }

        if (column_reader) {
            reader.append_unordered_column(column_reader);
        }
    }

    if (should_marshal_records) {
        reader.mark_unordered_object(object_begin_pos, mst_subtree_root_node_id, schema_ids);
    }
}

void ArchiveReader::initialize_schema_reader(
        SchemaReader& reader,
        int32_t schema_id,
        bool should_extract_timestamp,
        bool should_marshal_records
) {
    auto& schema = (*m_schema_map)[schema_id];
    reader.reset(
            m_schema_tree,
            m_projection,
            schema_id,
            schema.get_ordered_schema_view(),
            m_id_to_table_metadata[schema_id].num_messages,
            should_marshal_records
    );
    auto timestamp_column_ids = m_timestamp_dict->get_authoritative_timestamp_column_ids();
    for (size_t i = 0; i < schema.size(); ++i) {
        int32_t column_id = schema[i];
        if (Schema::schema_entry_is_unordered_object(column_id)) {
            size_t length = Schema::get_unordered_object_length(column_id);

            auto sub_schema = schema.get_view(i + 1, length);
            auto mst_subtree_root_node_id = m_schema_tree->find_matching_subtree_root_in_subtree(
                    -1,
                    SchemaReader::get_first_column_in_span(sub_schema),
                    Schema::get_unordered_object_type(column_id)
            );
            append_unordered_reader_columns(
                    reader,
                    mst_subtree_root_node_id,
                    sub_schema,
                    should_marshal_records
            );
            i += length;
            continue;
        }
        if (i >= schema.get_num_ordered()) {
            // Length one unordered object that doesn't have a tag. This is only allowed when the
            // column id is the root of the unordered object, so we can pass it directly to
            // append_unordered_reader_columns.
            append_unordered_reader_columns(
                    reader,
                    column_id,
                    std::span<int32_t>(),
                    should_marshal_records
            );
            continue;
        }
        BaseColumnReader* column_reader = append_reader_column(reader, column_id);

        if (should_extract_timestamp && column_reader && timestamp_column_ids.count(column_id) > 0)
        {
            reader.mark_column_as_timestamp(column_reader);
        }
    }
}

void ArchiveReader::store(FileWriter& writer) {
    std::string message;

    for (auto& [id, table_metadata] : m_id_to_table_metadata) {
        auto& schema_reader = read_table(id, false, true);
        while (schema_reader.get_next_message(message)) {
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
