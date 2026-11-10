#include "ArchiveReader.hpp"

#include <cstddef>
#include <cstdint>
#include <exception>
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
#include <clp_s/ColumnReader.hpp>
#include <clp_s/DictionaryEntry.hpp>
#include <clp_s/DictionaryReader.hpp>
#include <clp_s/ErrorCode.hpp>
#include <clp_s/InputConfig.hpp>
#include <clp_s/ReaderUtils.hpp>
#include <clp_s/TraceableException.hpp>

namespace clp_s {
namespace {
/**
 * Loads a dictionary's entries, translating reader exceptions into
 * `ArchiveReader::OperationFailed`.
 * @param dictionary The dictionary to load.
 * @param lazy Whether to defer entry decoding.
 * @throws ArchiveReader::OperationFailed if the dictionary can't be read.
 */
template <typename DictionaryReaderType>
auto load_dictionary(std::shared_ptr<DictionaryReaderType> const& dictionary, bool lazy) -> void;

template <typename DictionaryReaderType>
auto load_dictionary(std::shared_ptr<DictionaryReaderType> const& dictionary, bool lazy) -> void {
    try {
        dictionary->read_entries(lazy);
    } catch (TraceableException const& e) {
        throw ArchiveReader::OperationFailed(e.get_error_code(), __FILENAME__, __LINE__);
    }
}
}  // namespace

ArchiveReader::ArchiveReader(Path const& archive_path, Options const& options) {
    if (false == get_archive_id_from_path(archive_path, m_archive_id)) {
        throw OperationFailed(ErrorCodeBadParam, __FILENAME__, __LINE__);
    }

    m_archive_reader_adaptor
            = std::make_shared<ArchiveReaderAdaptor>(archive_path, options.network_auth);
    initialize_archive_reader();
}

ArchiveReader::ArchiveReader(
        std::shared_ptr<clp::ReaderInterface> single_file_archive_reader,
        std::string_view archive_id
) {
    if (nullptr == single_file_archive_reader || archive_id.empty()) {
        throw OperationFailed(ErrorCodeBadParam, __FILENAME__, __LINE__);
    }
    m_archive_id = archive_id;

    m_archive_reader_adaptor
            = std::make_shared<ArchiveReaderAdaptor>(std::move(single_file_archive_reader));
    initialize_archive_reader();
}

ArchiveReader::~ArchiveReader() {
    try {
        m_stream_reader.close();
    } catch (std::exception const& e) {
        SPDLOG_ERROR("Failed to close packed streams for archive {} - {}", m_archive_id, e.what());
    }
}

auto ArchiveReader::initialize_archive_reader() -> void {
    if (auto const rc = m_archive_reader_adaptor->load_archive_metadata(); ErrorCodeSuccess != rc) {
        throw OperationFailed(rc, __FILENAME__, __LINE__);
    }

    m_schema_tree = ReaderUtils::read_schema_tree(*m_archive_reader_adaptor);
    m_schema_map = ReaderUtils::read_schemas(*m_archive_reader_adaptor);

    m_log_event_idx_column_id = m_schema_tree->get_metadata_field_id(constants::cLogEventIdxName);

    m_var_dict = std::make_shared<VariableDictionaryReader>(
            *m_archive_reader_adaptor,
            std::string{static_cast<char const*>(constants::cArchiveVarDictFile)}
    );
    m_log_dict = std::make_shared<LogTypeDictionaryReader>(
            *m_archive_reader_adaptor,
            std::string{static_cast<char const*>(constants::cArchiveLogDictFile)}
    );
    m_array_dict = std::make_shared<LogTypeDictionaryReader>(
            *m_archive_reader_adaptor,
            std::string{static_cast<char const*>(constants::cArchiveArrayDictFile)}
    );
}

auto ArchiveReader::get_variable_dictionary() -> std::shared_ptr<VariableDictionaryReader> {
    load_dictionary(m_var_dict, true);
    return m_var_dict;
}

auto ArchiveReader::get_log_type_dictionary() -> std::shared_ptr<LogTypeDictionaryReader> {
    load_dictionary(m_log_dict, true);
    return m_log_dict;
}

auto ArchiveReader::get_array_dictionary() -> std::shared_ptr<LogTypeDictionaryReader> {
    load_dictionary(m_array_dict, true);
    return m_array_dict;
}

auto ArchiveReader::get_read_dictionaries() const -> ReadDictionaries {
    return {.var_dict = m_var_dict->has_read_entries(),
            .log_type_dict = m_log_dict->has_read_entries(),
            .array_dict = m_array_dict->has_read_entries()};
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
    if (m_has_read_metadata) {
        return ystdlib::error_handling::success();
    }

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
    m_has_read_metadata = true;

    return ystdlib::error_handling::success();
}

auto ArchiveReader::read_dictionaries_and_metadata() -> void {
    if (auto const result{read_metadata()}; result.has_error()) {
        throw OperationFailed(ErrorCodeFailure, __FILENAME__, __LINE__);
    }
    load_dictionary(m_var_dict, false);
    load_dictionary(m_log_dict, false);
    load_dictionary(m_array_dict, false);
}

auto ArchiveReader::open_packed_streams() -> void {
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

auto ArchiveReader::make_column_reader(int32_t column_id, bool is_unordered_object)
        -> std::unique_ptr<BaseColumnReader> {
    auto const& node = m_schema_tree->get_node(column_id);
    switch (node.get_type()) {
        case NodeType::Integer:
            return std::make_unique<Int64ColumnReader>(column_id);
        case NodeType::DeltaInteger:
            return std::make_unique<DeltaEncodedInt64ColumnReader>(column_id);
        case NodeType::Float:
            return std::make_unique<FloatColumnReader>(column_id);
        case NodeType::FormattedFloat:
            return std::make_unique<FormattedFloatColumnReader>(column_id);
        case NodeType::DictionaryFloat:
            return std::make_unique<DictionaryFloatColumnReader>(
                    column_id,
                    get_variable_dictionary()
            );
        case NodeType::ClpString:
            return std::make_unique<ClpStringColumnReader>(
                    column_id,
                    get_variable_dictionary(),
                    get_log_type_dictionary()
            );
        case NodeType::VarString:
            return std::make_unique<VariableStringColumnReader>(
                    column_id,
                    get_variable_dictionary()
            );
        case NodeType::Boolean:
            return std::make_unique<BooleanColumnReader>(column_id);
        case NodeType::UnstructuredArray:
            if (is_unordered_object) {
                return nullptr;
            }
            return std::make_unique<ClpStringColumnReader>(
                    column_id,
                    get_variable_dictionary(),
                    get_array_dictionary(),
                    true
            );
        case NodeType::DeprecatedDateString:
            if (is_unordered_object) {
                return nullptr;
            }
            return std::make_unique<DeprecatedDateStringColumnReader>(
                    column_id,
                    get_timestamp_dictionary()
            );
        case NodeType::Timestamp:
            if (is_unordered_object) {
                return nullptr;
            }
            return std::make_unique<TimestampColumnReader>(column_id, get_timestamp_dictionary());
        // No need to create readers for columns without associated object readers.
        case NodeType::Metadata:
        case NodeType::NullValue:
        case NodeType::Object:
        case NodeType::StructuredArray:
        case NodeType::Unknown:
            return nullptr;
    }
    return nullptr;
}

auto ArchiveReader::append_reader_column(SchemaReader& reader, int32_t column_id)
        -> BaseColumnReader* {
    auto column_reader{make_column_reader(column_id, false)};
    if (nullptr == column_reader) {
        return nullptr;
    }
    auto* const column_reader_ptr{column_reader.get()};
    reader.append_column(column_reader.release());
    return column_reader_ptr;
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
        // UnstructuredArray, DeprecatedDateString, and Timestamp currently aren't supported as part
        // of any unordered object, so `make_column_reader` returns nullptr for them.
        auto column_reader{make_column_reader(column_id, true)};
        if (column_reader) {
            reader.append_unordered_column(column_reader.release());
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
