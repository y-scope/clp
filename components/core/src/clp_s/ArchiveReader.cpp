#include "ArchiveReader.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>
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
#include <clp_s/DictionaryReader.hpp>
#include <clp_s/ErrorCode.hpp>
#include <clp_s/InputConfig.hpp>
#include <clp_s/ReaderUtils.hpp>
#include <clp_s/SchemaTree.hpp>
#include <clpp/Defs.hpp>
#include <clpp/ErrorCode.hpp>
#include <clpp/LogShapeStat.hpp>
#include <clpp/ParentRuleShapes.hpp>

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
        std::string_view archive_id,
        Options const& options
) -> void {
    if (m_is_open) {
        throw OperationFailed(ErrorCodeNotReady, __FILENAME__, __LINE__);
    }
    m_is_open = true;
    m_options = options;

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

    bool const experimental_archive{m_archive_reader_adaptor->experimental()};
    if (experimental_archive && false == m_options.m_experimental) {
        SPDLOG_ERROR("Archive was created with --experimental but --experimental flag is not set");
        throw OperationFailed(ErrorCodeBadParam, __FILENAME__, __LINE__);
    }

    m_schema_tree = ReaderUtils::read_schema_tree(*m_archive_reader_adaptor);
    m_read_sections.emplace(constants::cArchiveSchemaTreeFile);
    m_schema_map = ReaderUtils::read_schemas(*m_archive_reader_adaptor);
    m_read_sections.emplace(constants::cArchiveSchemaMapFile);

    m_log_event_idx_column_id = m_schema_tree->get_metadata_field_id(constants::cLogEventIdxName);

    m_var_dict = ReaderUtils::get_variable_dictionary_reader(*m_archive_reader_adaptor);
    if (experimental_archive) {
        m_clpp.emplace();
        m_clpp->log_shape_dict
                = std::make_shared<LogShapeDictionaryReader>(*m_archive_reader_adaptor);
        m_clpp->log_shape_dict->open(constants::cArchiveLogDictFile);
    } else {
        m_log_dict = ReaderUtils::get_log_type_dictionary_reader(*m_archive_reader_adaptor);
    }
    m_array_dict = ReaderUtils::get_array_dictionary_reader(*m_archive_reader_adaptor);
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
    if (m_read_sections.contains(constants::cArchiveTableMetadataFile)) {
        return ystdlib::error_handling::success();
    }
    constexpr size_t cDecompressorFileReadBufferCapacity{64 * 1024};  // 64 KiB
    ensure_section_readable(constants::cArchiveTableMetadataFile);
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
    m_read_sections.emplace(constants::cArchiveTableMetadataFile);

    return ystdlib::error_handling::success();
}

void ArchiveReader::read_dictionaries_and_metadata() {
    if (auto const result{read_metadata()}; result.has_error()) {
        throw OperationFailed(ErrorCodeFailure, __FILENAME__, __LINE__);
    }
    get_variable_dictionary();
    if (m_clpp.has_value()) {
        get_log_shape_dictionary();
    } else {
        get_log_type_dictionary();
    }
    get_array_dictionary();
}

auto ArchiveReader::ensure_section_readable(std::string_view section) -> void {
    if (false == m_archive_reader_adaptor->is_single_file_archive()) {
        return;
    }
    for (auto const prior : m_archive_reader_adaptor->get_sections_before(section)) {
        if (m_read_sections.contains(prior)) {
            continue;
        }
        if (constants::cArchiveSchemaTreeFile == prior || constants::cArchiveSchemaMapFile == prior)
        {
            m_read_sections.emplace(prior);
        } else if (constants::cArchiveTableMetadataFile == prior) {
            if (auto const result{read_metadata()}; result.has_error()) {
                throw OperationFailed(ErrorCodeFailure, __FILENAME__, __LINE__);
            }
        } else if (constants::cArchiveVarDictFile == prior) {
            get_variable_dictionary();
        } else if (constants::cArchiveLogDictFile == prior) {
            if (m_clpp.has_value()) {
                get_log_shape_dictionary();
            } else {
                get_log_type_dictionary();
            }
        } else if (constants::cArchiveArrayDictFile == prior) {
            get_array_dictionary();
        } else if (constants::cArchiveParentRuleShapesFile == prior) {
            get_parent_rule_shapes();
        } else if (constants::cArchiveLogShapeStatsFile == prior) {
            get_log_shape_stats();
        } else if (constants::cArchiveParsingSpecFile == prior) {
            std::ignore = read_parsing_spec();
        } else {
            SPDLOG_WARN("No reader for single-file archive section {}.", prior);
        }
    }
}

auto ArchiveReader::get_variable_dictionary() -> std::shared_ptr<VariableDictionaryReader> {
    if (false == m_read_sections.contains(constants::cArchiveVarDictFile)) {
        ensure_section_readable(constants::cArchiveVarDictFile);
        m_read_sections.emplace(constants::cArchiveVarDictFile);
        m_var_dict->read_entries(true);
    }
    return m_var_dict;
}

auto ArchiveReader::get_log_type_dictionary() -> std::shared_ptr<LogTypeDictionaryReader> {
    if (m_clpp.has_value()) {
        return nullptr;
    }
    if (false == m_read_sections.contains(constants::cArchiveLogDictFile)) {
        ensure_section_readable(constants::cArchiveLogDictFile);
        m_read_sections.emplace(constants::cArchiveLogDictFile);
        m_log_dict->read_entries(true);
    }
    return m_log_dict;
}

auto ArchiveReader::get_log_shape_dictionary() -> std::shared_ptr<LogShapeDictionaryReader> {
    if (false == m_clpp.has_value()) {
        return nullptr;
    }
    if (false == m_read_sections.contains(constants::cArchiveLogDictFile)) {
        ensure_section_readable(constants::cArchiveLogDictFile);
        m_read_sections.emplace(constants::cArchiveLogDictFile);
        m_clpp->log_shape_dict->read_entries(true);
    }
    return m_clpp->log_shape_dict;
}

auto ArchiveReader::get_array_dictionary() -> std::shared_ptr<LogTypeDictionaryReader> {
    if (false == m_read_sections.contains(constants::cArchiveArrayDictFile)) {
        ensure_section_readable(constants::cArchiveArrayDictFile);
        m_read_sections.emplace(constants::cArchiveArrayDictFile);
        m_array_dict->read_entries(true);
    }
    return m_array_dict;
}

auto ArchiveReader::get_log_shape_stats() -> clpp::LogShapeStatArray const& {
    if (false == m_clpp.has_value()) {
        throw OperationFailed(ErrorCodeBadParam, __FILENAME__, __LINE__);
    }
    if (false == m_clpp->log_shape_stats.has_value()) {
        ensure_section_readable(constants::cArchiveLogShapeStatsFile);
        m_read_sections.emplace(constants::cArchiveLogShapeStatsFile);
        auto result{read_log_shape_stats()};
        if (result.has_error()) {
            throw OperationFailed(ErrorCodeFailure, __FILENAME__, __LINE__);
        }
        m_clpp->log_shape_stats = result.value();
    }
    return m_clpp->log_shape_stats.value();
}

auto ArchiveReader::get_parent_rule_shapes() -> clpp::ParentRuleShapesArray const& {
    if (false == m_clpp.has_value()) {
        throw OperationFailed(ErrorCodeBadParam, __FILENAME__, __LINE__);
    }
    if (false == m_clpp->parent_rule_shapes.has_value()) {
        ensure_section_readable(constants::cArchiveParentRuleShapesFile);
        m_read_sections.emplace(constants::cArchiveParentRuleShapesFile);
        auto result{read_parent_rule_shapes()};
        if (result.has_error()) {
            throw OperationFailed(ErrorCodeFailure, __FILENAME__, __LINE__);
        }
        m_clpp->parent_rule_shapes = result.value();
    }
    return m_clpp->parent_rule_shapes.value();
}

void ArchiveReader::open_packed_streams() {
    ensure_section_readable(constants::cArchiveTablesFile);
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
        case NodeType::ParentRule:
        case NodeType::Unknown:
            break;
    }

    if (column_reader) {
        reader.append_column(column_reader);
    }
    return column_reader;
}

auto
ArchiveReader::resolve_unordered_object_root(UnorderedObject const& obj, int32_t search_root_id)
        -> int32_t {
    if (obj.root_node_id.has_value()) {
        return obj.root_node_id.value();
    }
    return m_schema_tree->find_matching_subtree_root_in_subtree(
            search_root_id,
            SchemaReader::get_first_column_in_span(obj.sub_schema),
            obj.type
    );
}

auto ArchiveReader::append_unordered_reader_columns(
        SchemaReader& reader,
        SchemaNode::id_t mst_subtree_root_node_id,
        SchemaView sub_schema,
        std::optional<clpp::log_shape_id_t> log_shape_id,
        bool should_marshal_records
) -> void {
    size_t const object_begin_pos{reader.get_column_size()};
    sub_schema.visit_entries(
            [&](SchemaNode::id_t node_id) -> bool {
                switch (m_schema_tree->get_node(node_id).get_type()) {
                    case NodeType::Integer:
                        reader.append_unordered_column(
                                std::make_unique<Int64ColumnReader>(node_id)
                        );
                        break;
                    case NodeType::DeltaInteger:
                        reader.append_unordered_column(
                                std::make_unique<DeltaEncodedInt64ColumnReader>(node_id)
                        );
                        break;
                    case NodeType::Float:
                        reader.append_unordered_column(
                                std::make_unique<FloatColumnReader>(node_id)
                        );
                        break;
                    case NodeType::FormattedFloat:
                        reader.append_unordered_column(
                                std::make_unique<FormattedFloatColumnReader>(node_id)
                        );
                        break;
                    case NodeType::DictionaryFloat:
                        reader.append_unordered_column(
                                std::make_unique<DictionaryFloatColumnReader>(node_id, m_var_dict)
                        );
                        break;
                    case NodeType::ClpString:
                        reader.append_unordered_column(
                                std::make_unique<ClpStringColumnReader>(
                                        node_id,
                                        m_var_dict,
                                        m_log_dict
                                )
                        );
                        break;
                    case NodeType::VarString:
                        reader.append_unordered_column(
                                std::make_unique<VariableStringColumnReader>(node_id, m_var_dict)
                        );
                        break;
                    case NodeType::Boolean:
                        reader.append_unordered_column(
                                std::make_unique<BooleanColumnReader>(node_id)
                        );
                        break;
                    // UnstructuredArray, DeprecatedDateString, and Timestamp currently aren't
                    // supported as part of any unordered object, so we disregard them here
                    case NodeType::UnstructuredArray:
                    case NodeType::DeprecatedDateString:
                    case NodeType::Timestamp:
                    // No need to push columns without associated object readers into the
                    // SchemaReader.
                    case NodeType::StructuredArray:
                    case NodeType::Object:
                    case NodeType::Metadata:
                    case NodeType::NullValue:
                    case NodeType::LogMessage:
                    case NodeType::ParentRule:
                    case NodeType::Unknown:
                        break;
                }
                return false;
            },
            [&](UnorderedObject const& obj) -> bool {
                append_unordered_reader_columns(
                        reader,
                        resolve_unordered_object_root(obj, mst_subtree_root_node_id),
                        obj.sub_schema,
                        obj.log_shape_id,
                        should_marshal_records
                );
                return false;
            }
    );

    if (should_marshal_records) {
        reader.mark_unordered_object(
                object_begin_pos,
                mst_subtree_root_node_id,
                sub_schema,
                log_shape_id
        );
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
            should_marshal_records,
            m_clpp.has_value() ? m_clpp->log_shape_dict.get() : nullptr,
            m_clpp.has_value() && m_clpp->parent_rule_shapes.has_value()
                    ? &m_clpp->parent_rule_shapes.value()
                    : nullptr,
            m_options.m_extract_mode
    );
    auto timestamp_column_ids
            = get_timestamp_dictionary()->get_authoritative_timestamp_column_ids();

    for (auto const node_id : schema.get_ordered_schema_view()) {
        auto* column_reader{append_reader_column(reader, node_id)};
        if (node_id == m_log_event_idx_column_id) {
            reader.mark_column_as_log_event_idx(column_reader);
        }

        if (should_extract_timestamp && nullptr != column_reader
            && timestamp_column_ids.contains(node_id))
        {
            reader.mark_column_as_timestamp(column_reader);
        }
    }

    schema.get_unordered_schema_view().visit_entries(
            [&](SchemaNode::id_t node_id) -> bool {
                // A lone MST node ID entry in the unordered region is only allowed when the ID is
                // the root of the unordered object, so we can pass it directly to
                // append_unordered_reader_columns with an empty sub-schema.
                append_unordered_reader_columns(
                        reader,
                        node_id,
                        SchemaView{{}},
                        std::nullopt,
                        should_marshal_records
                );
                return false;
            },
            [&](UnorderedObject const& obj) -> bool {
                append_unordered_reader_columns(
                        reader,
                        resolve_unordered_object_root(obj, -1),
                        obj.sub_schema,
                        obj.log_shape_id,
                        should_marshal_records
                );
                return false;
            }
    );
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
    if (m_clpp.has_value()) {
        m_clpp->log_shape_dict->close();
        if (m_clpp->log_shape_stats) {
            m_clpp->log_shape_stats->clear();
        }
        if (m_clpp->parent_rule_shapes) {
            m_clpp->parent_rule_shapes->clear();
        }
    } else {
        m_log_dict->close();
    }
    m_array_dict->close();

    m_clpp.reset();
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

auto ArchiveReader::read_log_shape_stats()
        -> ystdlib::error_handling::Result<clpp::LogShapeStatArray> {
    constexpr size_t cDecompressorFileReadBufferCapacity{64UL * 1024};
    auto reader{m_archive_reader_adaptor->checkout_reader_for_section(
            constants::cArchiveLogShapeStatsFile
    )};
    ZstdDecompressor decompressor{};
    decompressor.open(*reader, cDecompressorFileReadBufferCapacity);

    clpp::LogShapeStatArray stats;
    YSTDLIB_ERROR_HANDLING_TRYX(stats.decompress(decompressor));

    decompressor.close();
    m_archive_reader_adaptor->checkin_reader_for_section(constants::cArchiveLogShapeStatsFile);
    return stats;
}

auto ArchiveReader::read_parsing_spec() -> ystdlib::error_handling::Result<std::string> {
    if (false == m_clpp.has_value()) {
        return clpp::ClppErrorCode{clpp::ClppErrorCodeEnum::BadParam};
    }
    ensure_section_readable(constants::cArchiveParsingSpecFile);
    m_read_sections.emplace(constants::cArchiveParsingSpecFile);
    return ReaderUtils::read_parsing_spec(*m_archive_reader_adaptor);
}

auto ArchiveReader::read_parent_rule_shapes()
        -> ystdlib::error_handling::Result<clpp::ParentRuleShapesArray> {
    constexpr size_t cDecompressorFileReadBufferCapacity{64UL * 1024};
    auto reader{m_archive_reader_adaptor->checkout_reader_for_section(
            constants::cArchiveParentRuleShapesFile
    )};
    ZstdDecompressor decompressor{};
    decompressor.open(*reader, cDecompressorFileReadBufferCapacity);

    clpp::ParentRuleShapesArray shapes;
    YSTDLIB_ERROR_HANDLING_TRYX(shapes.decompress(decompressor));

    decompressor.close();
    m_archive_reader_adaptor->checkin_reader_for_section(constants::cArchiveParentRuleShapesFile);
    return shapes;
}
}  // namespace clp_s
