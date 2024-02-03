#include "ArchiveWriter.hpp"

#include "SchemaTree.hpp"

namespace clp_s {
void ArchiveWriter::open(ArchiveWriterOption const& option) {
    m_id = option.id;
    m_compression_level = option.compression_level;
    auto archive_path
            = boost::filesystem::path(option.archives_dir) / boost::uuids::to_string(m_id);

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

    m_encoded_messages_dir = m_archive_path + "/encoded_messages";
    if (false == boost::filesystem::create_directory(m_encoded_messages_dir)) {
        throw OperationFailed(ErrorCodeErrno, __FILENAME__, __LINE__);
    }

    std::string var_dict_path = m_archive_path + "/var.dict";
    m_var_dict = std::make_shared<VariableDictionaryWriter>();
    m_var_dict->open(var_dict_path, m_compression_level, UINT64_MAX);

    std::string log_dict_path = m_archive_path + "/log.dict";
    m_log_dict = std::make_shared<LogTypeDictionaryWriter>();
    m_log_dict->open(log_dict_path, m_compression_level, UINT64_MAX);

    std::string array_dict_path = m_archive_path + "/array.dict";
    m_array_dict = std::make_shared<LogTypeDictionaryWriter>();
    m_array_dict->open(array_dict_path, m_compression_level, UINT64_MAX);

    std::string timestamp_local_dict_path = m_archive_path + "/timestamp.dict";
    m_timestamp_dict->open_local(timestamp_local_dict_path, m_compression_level);
}

size_t ArchiveWriter::close() {
    size_t compressed_size{0};
    compressed_size += m_var_dict->close();
    compressed_size += m_log_dict->close();
    compressed_size += m_array_dict->close();
    compressed_size += m_timestamp_dict->close_local();

    for (auto& i : m_schema_id_to_writer) {
        i.second->store();
        compressed_size += i.second->close();
        delete i.second;
    }

    m_schema_id_to_writer.clear();
    m_encoded_message_size = 0UL;
    return compressed_size;
}

void ArchiveWriter::append_message(
        int32_t schema_id,
        Schema const& schema,
        ParsedMessage& message
) {
    SchemaWriter* schema_writer;
    auto it = m_schema_id_to_writer.find(schema_id);
    if (it != m_schema_id_to_writer.end()) {
        schema_writer = it->second;
    } else {
        schema_writer = new SchemaWriter();
        schema_writer->open(
                m_encoded_messages_dir + "/" + std::to_string(schema_id),
                m_compression_level
        );
        initialize_schema_writer(schema_writer, schema);
        m_schema_id_to_writer[schema_id] = schema_writer;
    }

    m_encoded_message_size += schema_writer->append_message(message);
}

size_t ArchiveWriter::get_data_size() {
    return m_log_dict->get_data_size() + m_var_dict->get_data_size() + m_array_dict->get_data_size()
           + m_encoded_message_size;
}

void ArchiveWriter::initialize_schema_writer(SchemaWriter* writer, Schema const& schema) {
    for (int32_t id : schema) {
        auto node = m_schema_tree->get_node(id);
        std::string key_name = node->get_key_name();
        switch (node->get_type()) {
            case NodeType::INTEGER:
                writer->append_column(new Int64ColumnWriter(key_name, id));
                break;
            case NodeType::FLOAT:
                writer->append_column(new FloatColumnWriter(key_name, id));
                break;
            case NodeType::CLPSTRING:
                writer->append_column(
                        new ClpStringColumnWriter(key_name, id, m_var_dict, m_log_dict)
                );
                break;
            case NodeType::VARSTRING:
                writer->append_column(new VariableStringColumnWriter(key_name, id, m_var_dict));
                break;
            case NodeType::BOOLEAN:
                writer->append_column(new BooleanColumnWriter(key_name, id));
                break;
            case NodeType::ARRAY:
                writer->append_column(
                        new ClpStringColumnWriter(key_name, id, m_var_dict, m_array_dict)
                );
                break;
            case NodeType::DATESTRING:
                writer->append_column(new DateStringColumnWriter(key_name, id, m_timestamp_dict));
                break;
            case NodeType::FLOATDATESTRING:
                writer->append_column(
                        new FloatDateStringColumnWriter(key_name, id, m_timestamp_dict)
                );
                break;
            case NodeType::OBJECT:
            case NodeType::NULLVALUE:
                break;
        }
    }
}
}  // namespace clp_s
