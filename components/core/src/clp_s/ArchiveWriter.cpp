#include "ArchiveWriter.hpp"

#include "SchemaTree.hpp"

namespace clp_s {
void ArchiveWriter::open(ArchiveWriterOption const& option) {
    m_id = option.id;
    m_compression_level = option.compression_level;
    auto archive_path = boost::filesystem::path(option.archive_dir) / boost::uuids::to_string(m_id);

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

void ArchiveWriter::close() {
    auto changed_nodes = m_schema_tree->modify_nodes_based_on_frequency(m_num_encoded_messages);
    m_var_dict->close();
    m_log_dict->close();
    m_array_dict->close();
    m_timestamp_dict->close_local();

    FileWriter table_file_writer;
    table_file_writer.open(
            m_encoded_messages_dir + "/table",
            FileWriter::OpenMode::CreateForWriting
    );

    FileWriter metadata_file_writer;
    metadata_file_writer.open(
            m_encoded_messages_dir + "/metadata",
            FileWriter::OpenMode::CreateForWriting
    );
    ZstdCompressor metadata_compressor;
    metadata_compressor.open(metadata_file_writer, m_compression_level);

    std::map<int32_t, std::pair<int32_t, std::vector<std::pair<int32_t, int32_t>>>>
            schema_id_to_schema_changes;
    for (auto it = m_schema_map->schema_map_begin(); it != m_schema_map->schema_map_end(); it++) {
        std::vector<std::pair<int32_t, int32_t>> schema_changes;
        for (auto& diff : changed_nodes) {
            if (it->first.count(diff.first)) {
                schema_changes.push_back(diff);
            }
        }

        if (schema_changes.empty()) {
            continue;
        }

        std::set<int32_t> new_schema = it->first;
        for (auto& change : schema_changes) {
            new_schema.erase(change.first);
            new_schema.insert(change.second);
        }

        int32_t new_schema_id = m_schema_map->add_schema(new_schema);
        schema_id_to_schema_changes[it->second] = {new_schema_id, std::move(schema_changes)};
    }

    std::map<int32_t, std::vector<SchemaWriter*>> updated_schema_id_to_writer;

    for (auto& i : m_schema_id_to_writer) {
        int32_t schema_id = i.first;
        auto change_it = schema_id_to_schema_changes.find(i.first);
        if (change_it != schema_id_to_schema_changes.end()) {
            updated_schema_id_to_writer[change_it->second.first].push_back(i.second);
            i.second->update_schema(m_schema_tree, change_it->second.second);
        } else {
            m_schema_map->mark_used(schema_id);
            //            i.second->open(
            //                    m_encoded_messages_dir + "/" + std::to_string(schema_id),
            //                    m_compression_level
            //            );
            size_t pos = table_file_writer.get_pos();
            metadata_compressor.write_numeric_value(schema_id);
            metadata_compressor.write_numeric_value(pos);
            i.second->open(m_compression_level);
            i.second->store(table_file_writer);
            i.second->close();
            delete i.second;
        }
    }

    for (auto& it : updated_schema_id_to_writer) {
        auto sit = it.second.begin();
        SchemaWriter* writer = *sit;
        ++sit;
        for (; sit != it.second.end(); ++sit) {
            writer->combine(*sit);
        }
        m_schema_map->mark_used(it.first);
        //        writer->open(m_encoded_messages_dir + "/" + std::to_string(it.first),
        //        m_compression_level);

        size_t pos = table_file_writer.get_pos();
        metadata_compressor.write_numeric_value(it.first);
        metadata_compressor.write_numeric_value(pos);
        writer->open(m_compression_level);
        writer->store(table_file_writer);
        writer->close();
        delete writer;
    }

    table_file_writer.close();
    metadata_compressor.close();
    metadata_file_writer.close();

    m_schema_id_to_writer.clear();
    m_encoded_message_size = 0UL;
}

void ArchiveWriter::append_message(
        int32_t schema_id,
        std::set<int32_t>& schema,
        ParsedMessage& message
) {
    SchemaWriter* schema_writer;
    auto it = m_schema_id_to_writer.find(schema_id);
    if (it != m_schema_id_to_writer.end()) {
        schema_writer = it->second;
    } else {
        schema_writer = new SchemaWriter();
        initialize_schema_writer(schema_writer, schema);
        m_schema_id_to_writer[schema_id] = schema_writer;
    }

    m_num_encoded_messages += 1;
    m_encoded_message_size += schema_writer->append_message(message);
}

size_t ArchiveWriter::get_data_size() {
    return m_log_dict->get_data_size() + m_var_dict->get_data_size() + m_array_dict->get_data_size()
           + m_encoded_message_size;
}

void ArchiveWriter::initialize_schema_writer(SchemaWriter* writer, std::set<int32_t>& schema) {
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
                writer->append_column(new VariableStringColumnWriter(
                        key_name,
                        id,
                        m_var_dict,
                        m_schema_tree->get_node(id)
                ));
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
