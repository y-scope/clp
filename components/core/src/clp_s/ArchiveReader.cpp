#include "ArchiveReader.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <system_error>
#include <utility>
#include <vector>

#include <fmt/core.h>
#include <spdlog/spdlog.h>
#include <ystdlib/error_handling/Result.hpp>

#include <clp/ir/types.hpp>
#include <clp/type_utils.hpp>
#include <clp_s/archive_constants.hpp>
#include <clp_s/ArchiveReaderAdaptor.hpp>
#include <clp_s/DictionaryEntry.hpp>
#include <clp_s/ErrorCode.hpp>
#include <clp_s/InputConfig.hpp>
#include <clp_s/ReaderUtils.hpp>
#include <clpp/ErrorCode.hpp>
#include <clpp/LogTypeStat.hpp>

#include "clp_s/SchemaTree.hpp"
#include "clpp/LogTypeMetadata.hpp"

namespace clp_s {
void ArchiveReader::open(Path const& archive_path, Options const& options) {
    if (m_is_open) {
        throw OperationFailed(ErrorCodeNotReady, __FILENAME__, __LINE__);
    }
    m_is_open = true;
    m_options = options;

    if (false == get_archive_id_from_path(archive_path, m_archive_id)) {
        throw OperationFailed(ErrorCodeBadParam, __FILENAME__, __LINE__);
    }

    m_archive_reader_adaptor
            = std::make_shared<ArchiveReaderAdaptor>(archive_path, options.m_network_auth);
    initialize_archive_reader();
}

auto ArchiveReader::open(
        std::shared_ptr<clp::ReaderInterface> single_file_archive_reader,
        std::string_view archive_id
) -> void {
    if (m_is_open) {
        throw OperationFailed(ErrorCodeNotReady, __FILENAME__, __LINE__);
    }
    m_is_open = true;

    if (nullptr == single_file_archive_reader || archive_id.empty()) {
        throw OperationFailed(ErrorCodeBadParam, __FILENAME__, __LINE__);
    }
    m_archive_id = archive_id;

    m_archive_reader_adaptor
            = std::make_shared<ArchiveReaderAdaptor>(std::move(single_file_archive_reader));
    initialize_archive_reader();
}

auto ArchiveReader::initialize_archive_reader() -> void {
    if (auto const rc = m_archive_reader_adaptor->load_archive_metadata(); ErrorCodeSuccess != rc) {
        throw OperationFailed(rc, __FILENAME__, __LINE__);
    }

    m_schema_tree = ReaderUtils::read_schema_tree(*m_archive_reader_adaptor);
    m_schema_map = ReaderUtils::read_schemas(*m_archive_reader_adaptor);

    m_log_event_idx_column_id = m_schema_tree->get_metadata_field_id(constants::cLogEventIdxName);

    m_var_dict = ReaderUtils::get_variable_dictionary_reader(*m_archive_reader_adaptor);
    if (m_options.m_experimental) {
        // TODO clpp: inlined get_variable_dictionary_reader
        m_typed_log_dict = std::make_shared<VariableDictionaryReader>(*m_archive_reader_adaptor);
        m_typed_log_dict->open(constants::cArchiveLogDictFile);
    } else {
        m_log_dict = ReaderUtils::get_log_type_dictionary_reader(*m_archive_reader_adaptor);
    }
    m_array_dict = ReaderUtils::get_array_dictionary_reader(*m_archive_reader_adaptor);

    // TODO clpp: go back to reading metadata and stats on demand
    if (m_options.m_experimental) {
        auto metadata{read_logtype_metadata()};
        if (metadata.has_error()) {
            throw OperationFailed(ErrorCodeBadParam, __FILENAME__, __LINE__);
        }
        m_logtype_metadata = metadata.value();
        m_logtype_stats = clpp::LogTypeStatArray();
        read_logtype_stats();
        m_typed_log_dict->read_entries();
    }
}

auto ArchiveReader::read_single_schema_metadata()
        -> ystdlib::error_handling::Result<std::pair<int32_t, SchemaReader::SchemaMetadata>> {
    uint64_t stream_id_u64{0};
    uint64_t stream_offset_u64{0};
    int32_t schema_id{0};
    uint64_t num_messages{0};

    if (auto const error{m_table_metadata_decompressor.try_read_numeric_value(stream_id_u64)};
        ErrorCodeSuccess != error)
    {
        return std::errc::io_error;
    }
    auto const stream_id{
            YSTDLIB_ERROR_HANDLING_TRYX(ReaderUtils::try_uint64_to_size_t(stream_id_u64))
    };

    if (auto const error{m_table_metadata_decompressor.try_read_numeric_value(stream_offset_u64)};
        ErrorCodeSuccess != error)
    {
        return std::errc::io_error;
    }
    auto const stream_offset{
            YSTDLIB_ERROR_HANDLING_TRYX(ReaderUtils::try_uint64_to_size_t(stream_offset_u64))
    };

    if (stream_offset > m_stream_reader.get_uncompressed_stream_size(stream_id)) {
        return std::errc::illegal_byte_sequence;
    }

    if (auto const error{m_table_metadata_decompressor.try_read_numeric_value(schema_id)};
        ErrorCodeSuccess != error)
    {
        return std::errc::io_error;
    }

    if (auto const error{m_table_metadata_decompressor.try_read_numeric_value(num_messages)};
        ErrorCodeSuccess != error)
    {
        return std::errc::io_error;
    }

    return std::make_pair(
            schema_id,
            SchemaReader::SchemaMetadata{stream_id, stream_offset, num_messages}
    );
}

auto ArchiveReader::read_metadata() -> ystdlib::error_handling::Result<void> {
    constexpr size_t cDecompressorFileReadBufferCapacity{64 * 1024};  // 64 KiB
    auto table_metadata_reader = m_archive_reader_adaptor->checkout_reader_for_section(
            constants::cArchiveTableMetadataFile
    );
    m_table_metadata_decompressor.open(*table_metadata_reader, cDecompressorFileReadBufferCapacity);

    YSTDLIB_ERROR_HANDLING_TRYV(m_stream_reader.read_metadata(m_table_metadata_decompressor));

    uint64_t num_separate_column_schemas{0};
    if (auto const error{
                m_table_metadata_decompressor.try_read_numeric_value(num_separate_column_schemas)
        };
        ErrorCodeSuccess != error)
    {
        throw OperationFailed(error, __FILENAME__, __LINE__);
    }

    if (0 != num_separate_column_schemas) {
        throw OperationFailed(ErrorCodeUnsupported, __FILENAME__, __LINE__);
    }

    uint64_t num_schemas{0};
    if (auto const error{m_table_metadata_decompressor.try_read_numeric_value(num_schemas)};
        ErrorCodeSuccess != error)
    {
        throw OperationFailed(error, __FILENAME__, __LINE__);
    }
    if (0 == num_schemas) {
        throw OperationFailed(ErrorCodeUnsupported, __FILENAME__, __LINE__);
    }

    auto [prev_schema_id,
          prev_metadata]{YSTDLIB_ERROR_HANDLING_TRYX(read_single_schema_metadata())};
    m_schema_ids.push_back(prev_schema_id);
    for (uint64_t i{1}; i < num_schemas; ++i) {
        auto const [schema_id, metadata]{
                YSTDLIB_ERROR_HANDLING_TRYX(read_single_schema_metadata())
        };
        m_schema_ids.push_back(schema_id);

        if (metadata.stream_id() != prev_metadata.stream_id()) {
            prev_metadata.set_uncompressed_size(
                    m_stream_reader.get_uncompressed_stream_size(prev_metadata.stream_id())
                    - prev_metadata.stream_offset()
            );
        } else if (metadata.stream_offset() < prev_metadata.stream_offset()) {
            throw OperationFailed(ErrorCodeCorrupt, __FILENAME__, __LINE__);
        } else {
            prev_metadata.set_uncompressed_size(
                    metadata.stream_offset() - prev_metadata.stream_offset()
            );
        }
        m_id_to_schema_metadata[prev_schema_id] = prev_metadata;

        prev_schema_id = schema_id;
        prev_metadata = metadata;
    }
    prev_metadata.set_uncompressed_size(
            m_stream_reader.get_uncompressed_stream_size(prev_metadata.stream_id())
            - prev_metadata.stream_offset()
    );
    m_id_to_schema_metadata[prev_schema_id] = prev_metadata;
    m_table_metadata_decompressor.close();

    m_archive_reader_adaptor->checkin_reader_for_section(constants::cArchiveTableMetadataFile);

    return ystdlib::error_handling::success();
}

void ArchiveReader::read_dictionaries_and_metadata() {
    if (auto const result{read_metadata()}; result.has_error()) {
        throw OperationFailed(ErrorCodeFailure, __FILENAME__, __LINE__);
    }
    m_var_dict->read_entries();
    if (nullptr != m_typed_log_dict) {
        m_typed_log_dict->read_entries();
    } else {
        m_log_dict->read_entries();
    }
    m_array_dict->read_entries();
    std::ignore = read_logtype_stats();
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

    auto const& schema_metadata = m_id_to_schema_metadata[schema_id];
    auto stream_buffer = read_stream(schema_metadata.stream_id(), true);
    m_schema_reader.load(
            stream_buffer,
            schema_metadata.stream_offset(),
            schema_metadata.uncompressed_size()
    );
    return m_schema_reader;
}

std::vector<std::shared_ptr<SchemaReader>> ArchiveReader::read_all_tables() {
    std::vector<std::shared_ptr<SchemaReader>> readers;
    readers.reserve(m_id_to_schema_metadata.size());
    for (auto schema_id : m_schema_ids) {
        auto schema_reader = std::make_shared<SchemaReader>();
        initialize_schema_reader(*schema_reader, schema_id, true, true);
        auto const& schema_metadata = m_id_to_schema_metadata[schema_id];
        auto stream_buffer = read_stream(schema_metadata.stream_id(), false);
        schema_reader->load(
                stream_buffer,
                schema_metadata.stream_offset(),
                schema_metadata.uncompressed_size()
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
        case NodeType::FormattedFloat:
            column_reader = new FormattedFloatColumnReader(column_id);
            break;
        case NodeType::DictionaryFloat:
            column_reader = new DictionaryFloatColumnReader(column_id, m_var_dict);
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
        case NodeType::DeprecatedDateString:
            column_reader
                    = new DeprecatedDateStringColumnReader(column_id, get_timestamp_dictionary());
            break;
        case NodeType::Timestamp:
            column_reader = new TimestampColumnReader(column_id, get_timestamp_dictionary());
            break;
        // No need to push columns without associated object readers into the SchemaReader.
        case NodeType::Metadata:
        case NodeType::NullValue:
        case NodeType::Object:
        case NodeType::StructuredArray:
        case NodeType::LogMessage:
        case NodeType::LogType:
        case NodeType::LogTypeID:
        case NodeType::ParentVarType:
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
    for (size_t i = 0; i < schema_ids.size(); ++i) {
        auto const column_id{schema_ids[i]};
        if (Schema::schema_entry_is_unordered_object(column_id)) {
            auto length{Schema::get_unordered_object_length(column_id)};
            auto sub_schema{schema_ids.subspan(i + 1, length)};
            auto subtree_root_node_id{m_schema_tree->find_matching_subtree_root_in_subtree(
                    mst_subtree_root_node_id,
                    SchemaReader::get_first_column_in_span(sub_schema),
                    Schema::get_unordered_object_type(column_id)
            )};
            append_unordered_reader_columns(
                    reader,
                    subtree_root_node_id,
                    sub_schema,
                    should_marshal_records
            );
            i += length;
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
            case NodeType::FormattedFloat:
                column_reader = new FormattedFloatColumnReader(column_id);
                break;
            case NodeType::DictionaryFloat:
                column_reader = new DictionaryFloatColumnReader(column_id, m_var_dict);
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
            // UnstructuredArray, DeprecatedDateString, and Timestamp currently aren't supported as
            // part of any unordered object, so we disregard them here
            case NodeType::UnstructuredArray:
            case NodeType::DeprecatedDateString:
            case NodeType::Timestamp:
                column_reader = new VariableStringColumnReader(column_id, m_typed_log_dict);
                break;
            // No need to push columns without associated object readers into the SchemaReader.
            case NodeType::StructuredArray:
            case NodeType::Object:
            case NodeType::Metadata:
            case NodeType::NullValue:
            case NodeType::LogMessage:
            case NodeType::LogType:
            case NodeType::LogTypeID:
            case NodeType::ParentVarType:
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
            m_id_to_schema_metadata[schema_id].num_messages(),
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

    SPDLOG_INFO("[stats] var dict size: {}", m_var_dict->get_entries().size());
    m_var_dict->close();
    if (nullptr != m_typed_log_dict) {
        SPDLOG_INFO("[stats] log dict size: {}", m_typed_log_dict->get_entries().size());
        m_typed_log_dict->close();
    } else {
        SPDLOG_INFO("[stats] log dict size: {}", m_log_dict->get_entries().size());
        m_log_dict->close();
    }
    if (m_logtype_metadata.has_value()) {
        m_logtype_metadata->clear();
    }
    if (m_logtype_stats.has_value()) {
        m_logtype_stats->clear();
    }
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

auto ArchiveReader::read_logtype_metadata()
        -> ystdlib::error_handling::Result<clpp::LogTypeMetadataArray> {
    constexpr size_t cDecompressorFileReadBufferCapacity{64UL * 1024};
    auto reader{m_archive_reader_adaptor->checkout_reader_for_section(
            constants::cArchiveLogTypeMetadataFile
    )};
    ZstdDecompressor decompressor{};
    decompressor.open(*reader, cDecompressorFileReadBufferCapacity);

    clpp::LogTypeMetadataArray metadata;
    YSTDLIB_ERROR_HANDLING_TRYX(metadata.decompress(decompressor));

    decompressor.close();
    m_archive_reader_adaptor->checkin_reader_for_section(constants::cArchiveLogTypeMetadataFile);
    return metadata;
}

auto ArchiveReader::read_logtype_stats() -> ystdlib::error_handling::Result<void> {
    if (false == m_logtype_stats.has_value()) {
        return clpp::ClppErrorCode{clpp::ClppErrorCodeEnum::BadParam};
    }
    constexpr size_t cDecompressorFileReadBufferCapacity{64UL * 1024};
    auto reader{m_archive_reader_adaptor->checkout_reader_for_section(
            constants::cArchiveLogTypeStatsFile
    )};
    ZstdDecompressor decompressor{};
    decompressor.open(*reader, cDecompressorFileReadBufferCapacity);

    YSTDLIB_ERROR_HANDLING_TRYX(m_logtype_stats->decompress(decompressor));

    decompressor.close();
    m_archive_reader_adaptor->checkin_reader_for_section(constants::cArchiveLogTypeStatsFile);
    return ystdlib::error_handling::success();
}

auto ArchiveReader::read_log_surgeon_schema() -> ystdlib::error_handling::Result<std::string> {
    if (false == m_options.m_experimental) {
        return clpp::ClppErrorCode{clpp::ClppErrorCodeEnum::BadParam};
    }
    return ReaderUtils::read_log_surgeon_schema(*m_archive_reader_adaptor);
}
}  // namespace clp_s
