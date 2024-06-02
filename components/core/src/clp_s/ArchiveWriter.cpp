#include "ArchiveWriter.hpp"

#include <json/single_include/nlohmann/json.hpp>

#include "archive_constants.hpp"
#include "Defs.hpp"
#include "SchemaTree.hpp"

namespace clp_s {
void ArchiveWriter::open(ArchiveWriterOption const& option) {
    m_id = boost::uuids::to_string(option.id);
    m_compression_level = option.compression_level;
    m_print_archive_stats = option.print_archive_stats;
    auto archive_path = boost::filesystem::path(option.archives_dir) / m_id;

    boost::system::error_code boost_error_code;
    bool path_exists = boost::filesystem::exists(archive_path, boost_error_code);
    if (path_exists) {
        SPDLOG_ERROR("Archive path already exists: {}", archive_path.c_str());
        throw OperationFailed(ErrorCodeUnsupported, __FILENAME__, __LINE__);
    }

    m_archive_path = archive_path.string();
    if (false == boost::filesystem::create_directory(m_archive_path)) {
        throw OperationFailed(ErrorCodeErrno, __FILENAME__, __LINE__);
    }

    std::string var_dict_path = m_archive_path + constants::cArchiveVarDictFile;
    m_var_dict = std::make_shared<VariableDictionaryWriter>();
    m_var_dict->open(var_dict_path, m_compression_level, UINT64_MAX);

    std::string log_dict_path = m_archive_path + constants::cArchiveLogDictFile;
    m_log_dict = std::make_shared<LogTypeDictionaryWriter>();
    m_log_dict->open(log_dict_path, m_compression_level, UINT64_MAX);

    std::string array_dict_path = m_archive_path + constants::cArchiveArrayDictFile;
    m_array_dict = std::make_shared<LogTypeDictionaryWriter>();
    m_array_dict->open(array_dict_path, m_compression_level, UINT64_MAX);

    std::string timestamp_dict_path = m_archive_path + constants::cArchiveTimestampDictFile;
    m_timestamp_dict = std::make_shared<TimestampDictionaryWriter>();
    m_timestamp_dict->open(timestamp_dict_path, m_compression_level);
}

void ArchiveWriter::close() {
    m_compressed_size += m_var_dict->close();
    m_compressed_size += m_log_dict->close();
    m_compressed_size += m_array_dict->close();
    m_compressed_size += m_timestamp_dict->close();
    m_compressed_size += m_schema_tree.store(m_archive_path, m_compression_level);
    m_compressed_size += m_schema_map.store(m_archive_path, m_compression_level);
    m_compressed_size += store_tables();

    if (m_metadata_db) {
        update_metadata_db();
    }

    if (m_print_archive_stats) {
        print_archive_stats();
    }

    m_id_to_schema_writer.clear();
    m_schema_tree.clear();
    m_schema_map.clear();
    m_encoded_message_size = 0UL;
    m_uncompressed_size = 0UL;
    m_compressed_size = 0UL;
}

void ArchiveWriter::append_message(
        int32_t schema_id,
        Schema const& schema,
        ParsedMessage& message
) {
    SchemaWriter* schema_writer;
    auto it = m_id_to_schema_writer.find(schema_id);
    if (it != m_id_to_schema_writer.end()) {
        schema_writer = it->second;
    } else {
        schema_writer = new SchemaWriter();
        initialize_schema_writer(schema_writer, schema);
        m_id_to_schema_writer[schema_id] = schema_writer;
    }

    m_encoded_message_size += schema_writer->append_message(message);
}

size_t ArchiveWriter::get_data_size() {
    return m_log_dict->get_data_size() + m_var_dict->get_data_size() + m_array_dict->get_data_size()
           + m_encoded_message_size;
}

void ArchiveWriter::initialize_schema_writer(SchemaWriter* writer, Schema const& schema) {
    for (int32_t id : schema) {
        if (Schema::schema_entry_is_unordered_object(id)) {
            continue;
        }
        auto const& node = m_schema_tree.get_node(id);
        switch (node.get_type()) {
            case NodeType::Integer:
                writer->append_column(new Int64ColumnWriter(id));
                break;
            case NodeType::Float:
                writer->append_column(new FloatColumnWriter(id));
                break;
            case NodeType::ClpString:
                writer->append_column(new ClpStringColumnWriter(id, m_var_dict, m_log_dict));
                break;
            case NodeType::VarString:
                writer->append_column(new VariableStringColumnWriter(id, m_var_dict));
                break;
            case NodeType::Boolean:
                writer->append_column(new BooleanColumnWriter(id));
                break;
            case NodeType::UnstructuredArray:
                writer->append_column(new ClpStringColumnWriter(id, m_var_dict, m_array_dict));
                break;
            case NodeType::DateString:
                writer->append_column(new DateStringColumnWriter(id));
                break;
            case NodeType::StructuredArray:
            case NodeType::Object:
            case NodeType::NullValue:
            case NodeType::Unknown:
                break;
        }
    }
}

size_t ArchiveWriter::store_tables() {
    size_t compressed_size = 0;
    m_tables_file_writer.open(
            m_archive_path + constants::cArchiveTablesFile,
            FileWriter::OpenMode::CreateForWriting
    );
    m_table_metadata_file_writer.open(
            m_archive_path + constants::cArchiveTableMetadataFile,
            FileWriter::OpenMode::CreateForWriting
    );
    m_table_metadata_compressor.open(m_table_metadata_file_writer, m_compression_level);
    m_table_metadata_compressor.write_numeric_value(m_id_to_schema_writer.size());
    for (auto& i : m_id_to_schema_writer) {
        m_table_metadata_compressor.write_numeric_value(i.first);
        m_table_metadata_compressor.write_numeric_value(i.second->get_num_messages());
        m_table_metadata_compressor.write_numeric_value(m_tables_file_writer.get_pos());

        m_tables_compressor.open(m_tables_file_writer, m_compression_level);
        size_t uncompressed_size = i.second->store(m_tables_compressor);
        m_tables_compressor.close();
        delete i.second;

        m_table_metadata_compressor.write_numeric_value(uncompressed_size);
    }
    m_table_metadata_compressor.close();

    compressed_size += m_table_metadata_file_writer.get_pos();
    compressed_size += m_tables_file_writer.get_pos();

    m_table_metadata_file_writer.close();
    m_tables_file_writer.close();

    return compressed_size;
}

void ArchiveWriter::update_metadata_db() {
    m_metadata_db->open();
    clp::streaming_archive::ArchiveMetadata metadata(
            cArchiveFormatDevelopmentVersionFlag,
            "",
            0ULL
    );
    metadata.increment_static_compressed_size(m_compressed_size);
    metadata.increment_static_uncompressed_size(m_uncompressed_size);
    metadata.expand_time_range(
            m_timestamp_dict->get_begin_timestamp(),
            m_timestamp_dict->get_end_timestamp()
    );

    m_metadata_db->add_archive(m_id, metadata);
    m_metadata_db->close();
}

void ArchiveWriter::print_archive_stats() {
    nlohmann::json json_msg;
    json_msg["id"] = m_id;
    json_msg["uncompressed_size"] = m_uncompressed_size;
    json_msg["size"] = m_compressed_size;
    std::cout << json_msg.dump(-1, ' ', true, nlohmann::json::error_handler_t::ignore) << std::endl;
}
}  // namespace clp_s
