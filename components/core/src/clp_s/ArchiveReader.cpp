#include "ArchiveReader.hpp"

#include <filesystem>
#include <string_view>

#include "archive_constants.hpp"
#include "ArchiveReaderAdaptor.hpp"
#include "InputConfig.hpp"
#include "ReaderUtils.hpp"

using std::string_view;

namespace clp_s {
void ArchiveReader::open(Path const& archive_path, NetworkAuthOption const& network_auth) {
    if (m_is_open) {
        throw OperationFailed(ErrorCodeNotReady, __FILENAME__, __LINE__);
    }
    m_is_open = true;

    if (false == get_archive_id_from_path(archive_path, m_archive_id)) {
        throw OperationFailed(ErrorCodeBadParam, __FILENAME__, __LINE__);
    }

    m_archive_reader_adaptor = std::make_shared<ArchiveReaderAdaptor>(archive_path, network_auth);

    if (auto const rc = m_archive_reader_adaptor->load_archive_metadata(); ErrorCodeSuccess != rc) {
        throw OperationFailed(rc, __FILENAME__, __LINE__);
    }

    m_schema_tree = ReaderUtils::read_schema_tree(*m_archive_reader_adaptor);
    m_schema_map = ReaderUtils::read_schemas(*m_archive_reader_adaptor);

    m_log_event_idx_column_id = m_schema_tree->get_metadata_field_id(constants::cLogEventIdxName);

    m_var_dict = ReaderUtils::get_variable_dictionary_reader(*m_archive_reader_adaptor);
    m_log_dict = ReaderUtils::get_log_type_dictionary_reader(*m_archive_reader_adaptor);
    m_array_dict = ReaderUtils::get_array_dictionary_reader(*m_archive_reader_adaptor);
}

void ArchiveReader::read_metadata() {
    constexpr size_t cDecompressorFileReadBufferCapacity = 64 * 1024;  // 64 KB
    auto table_metadata_reader = m_archive_reader_adaptor->checkout_reader_for_section(
            constants::cArchiveTableMetadataFile
    );
    m_table_metadata_decompressor.open(*table_metadata_reader, cDecompressorFileReadBufferCapacity);

    m_stream_reader.read_metadata(m_table_metadata_decompressor);

    size_t num_separate_column_schemas;
    if (auto error
        = m_table_metadata_decompressor.try_read_numeric_value(num_separate_column_schemas);
        ErrorCodeSuccess != error)
    {
        throw OperationFailed(error, __FILENAME__, __LINE__);
    }

    if (0 != num_separate_column_schemas) {
        throw OperationFailed(ErrorCode::ErrorCodeUnsupported, __FILENAME__, __LINE__);
    }

    size_t num_schemas;
    if (auto error = m_table_metadata_decompressor.try_read_numeric_value(num_schemas);
        ErrorCodeSuccess != error)
    {
        throw OperationFailed(error, __FILENAME__, __LINE__);
    }

    bool prev_metadata_initialized{false};
    SchemaReader::SchemaMetadata prev_metadata{};
    int32_t prev_schema_id{};
    for (size_t i = 0; i < num_schemas; ++i) {
        uint64_t stream_id;
        uint64_t stream_offset;
        int32_t schema_id;
        uint64_t num_messages;

        if (auto error = m_table_metadata_decompressor.try_read_numeric_value(stream_id);
            ErrorCodeSuccess != error)
        {
            throw OperationFailed(error, __FILENAME__, __LINE__);
        }

        if (auto error = m_table_metadata_decompressor.try_read_numeric_value(stream_offset);
            ErrorCodeSuccess != error)
        {
            throw OperationFailed(error, __FILENAME__, __LINE__);
        }

        if (stream_offset > m_stream_reader.get_uncompressed_stream_size(stream_id)) {
            throw OperationFailed(ErrorCodeCorrupt, __FILENAME__, __LINE__);
        }

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

        if (prev_metadata_initialized) {
            uint64_t uncompressed_size{0};
            if (stream_id != prev_metadata.stream_id) {
                uncompressed_size
                        = m_stream_reader.get_uncompressed_stream_size(prev_metadata.stream_id)
                          - prev_metadata.stream_offset;
            } else {
                uncompressed_size = stream_offset - prev_metadata.stream_offset;
            }
            prev_metadata.uncompressed_size = uncompressed_size;
            m_id_to_schema_metadata[prev_schema_id] = prev_metadata;
        } else {
            prev_metadata_initialized = true;
        }
        prev_metadata = {stream_id, stream_offset, num_messages, 0};
        prev_schema_id = schema_id;
        m_schema_ids.push_back(schema_id);
    }
    prev_metadata.uncompressed_size
            = m_stream_reader.get_uncompressed_stream_size(prev_metadata.stream_id)
              - prev_metadata.stream_offset;
    m_id_to_schema_metadata[prev_schema_id] = prev_metadata;
    m_table_metadata_decompressor.close();

    m_archive_reader_adaptor->checkin_reader_for_section(constants::cArchiveTableMetadataFile);
}

void ArchiveReader::read_dictionaries_and_metadata() {
    read_metadata();
    m_var_dict->read_entries();
    m_log_dict->read_entries();
    m_array_dict->read_entries();
}

void ArchiveReader::open_packed_streams() {
    m_stream_reader.open_packed_streams(m_archive_reader_adaptor);
}

SchemaReader& ArchiveReader::read_schema_table(
        int32_t schema_id,
        bool should_extract_timestamp,
        bool should_marshal_records
) {
    if (m_id_to_schema_metadata.count(schema_id) == 0) {
        throw OperationFailed(ErrorCodeFileNotFound, __FILENAME__, __LINE__);
    }

    initialize_schema_reader(
            m_schema_reader,
            schema_id,
            should_extract_timestamp,
            should_marshal_records
    );

    auto& schema_metadata = m_id_to_schema_metadata[schema_id];
    auto stream_buffer = read_stream(schema_metadata.stream_id, true);
    m_schema_reader
            .load(stream_buffer, schema_metadata.stream_offset, schema_metadata.uncompressed_size);
    return m_schema_reader;
}

std::vector<std::shared_ptr<SchemaReader>> ArchiveReader::read_all_tables() {
    std::vector<std::shared_ptr<SchemaReader>> readers;
    readers.reserve(m_id_to_schema_metadata.size());
    for (auto schema_id : m_schema_ids) {
        auto schema_reader = std::make_shared<SchemaReader>();
        initialize_schema_reader(*schema_reader, schema_id, true, true);
        auto& schema_metadata = m_id_to_schema_metadata[schema_id];
        auto stream_buffer = read_stream(schema_metadata.stream_id, false);
        schema_reader->load(
                stream_buffer,
                schema_metadata.stream_offset,
                schema_metadata.uncompressed_size
        );
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
        case NodeType::DeltaInteger:
            column_reader = new DeltaEncodedInt64ColumnReader(column_id);
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
            column_reader = new DateStringColumnReader(column_id, get_timestamp_dictionary());
            break;
        // No need to push columns without associated object readers into the SchemaReader.
        case NodeType::Metadata:
        case NodeType::NullValue:
        case NodeType::Object:
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
            case NodeType::DeltaInteger:
                column_reader = new DeltaEncodedInt64ColumnReader(column_id);
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
            case NodeType::Metadata:
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
            m_id_to_schema_metadata[schema_id].num_messages,
            should_marshal_records
    );
    auto timestamp_column_ids
            = get_timestamp_dictionary()->get_authoritative_timestamp_column_ids();
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

        if (column_id == m_log_event_idx_column_id) {
            reader.mark_column_as_log_event_idx(column_reader);
        }

        if (should_extract_timestamp && column_reader && timestamp_column_ids.count(column_id) > 0)
        {
            reader.mark_column_as_timestamp(column_reader);
        }
    }
}

void ArchiveReader::store(FileWriter& writer) {
    std::string message;
    for (auto schema_id : m_schema_ids) {
        auto& schema_reader = read_schema_table(schema_id, false, true);
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

    m_stream_reader.close();
    m_archive_reader_adaptor.reset();

    m_id_to_schema_metadata.clear();
    m_schema_ids.clear();
    m_cur_stream_id = 0;
    m_stream_buffer.reset();
    m_stream_buffer_size = 0ULL;
    m_log_event_idx_column_id = -1;
}

std::shared_ptr<char[]> ArchiveReader::read_stream(size_t stream_id, bool reuse_buffer) {
    if (nullptr != m_stream_buffer && m_cur_stream_id == stream_id) {
        return m_stream_buffer;
    }

    if (false == reuse_buffer) {
        m_stream_buffer.reset();
        m_stream_buffer_size = 0;
    }

    m_stream_reader.read_stream(stream_id, m_stream_buffer, m_stream_buffer_size);
    m_cur_stream_id = stream_id;
    return m_stream_buffer;
}
}  // namespace clp_s
