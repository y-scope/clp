#include "ArchiveReader.hpp"

#include "ReaderUtils.hpp"

namespace clp_s {
void ArchiveReader::open(ArchiveReaderOption& option) {
    // Open dictionary readers
    m_archive_path = option.archive_path;

    m_var_dict = ReaderUtils::get_variable_dictionary_reader(m_archive_path);
    m_log_dict = ReaderUtils::get_log_type_dictionary_reader(m_archive_path);
    m_array_dict = ReaderUtils::get_array_dictionary_reader(m_archive_path);

    m_var_dict->read_new_entries();
    m_log_dict->read_new_entries();
    m_array_dict->read_new_entries();

    std::string encoded_messages_dir = m_archive_path + "/encoded_messages";
    if (false == boost::filesystem::exists(encoded_messages_dir)) {
        throw OperationFailed(ErrorCodeErrno, __FILENAME__, __LINE__);
    }

    std::set<int32_t> schema_ids;
    boost::filesystem::directory_iterator iter(encoded_messages_dir);
    boost::filesystem::directory_iterator end;

    // Get all schema ids
    for (; iter != end; ++iter) {
        if (boost::filesystem::is_regular_file(iter->path())) {
            std::string path = iter->path().rbegin()->string();
            if (false == path.empty() && std::all_of(path.begin(), path.end(), ::isdigit)) {
                schema_ids.insert(std::stoi(path));
            }
        }
    }

    if (schema_ids.empty()) {
        throw OperationFailed(ErrorCodeFileNotFound, __FILENAME__, __LINE__);
    }

    // Open schema readers and load encoded messages
    for (int32_t schema_id : schema_ids) {
        auto& schema = m_schema_map[schema_id];
        auto schema_reader = new SchemaReader(m_schema_tree, schema_id);
        schema_reader->open(encoded_messages_dir + "/" + std::to_string(schema_id));

        ReaderUtils::append_reader_columns(
                schema_reader,
                schema,
                m_schema_tree,
                m_var_dict,
                m_log_dict,
                m_array_dict,
                m_timestamp_dict,
                false
        );

        schema_reader->load();
        m_schema_id_to_reader[schema_id] = schema_reader;
    }
}

void ArchiveReader::store(FileWriter& writer) {
    std::string message;
    for (auto& i : m_schema_id_to_reader) {
        while (i.second->get_next_message(message)) {
            writer.write(message.c_str(), message.length());
        }
    }
}

void ArchiveReader::close() {
    m_var_dict->close();
    m_log_dict->close();

    for (auto& i : m_schema_id_to_reader) {
        i.second->close();
        delete i.second;
    }

    m_schema_id_to_reader.clear();
}
}  // namespace clp_s
