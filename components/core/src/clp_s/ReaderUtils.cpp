#include "ReaderUtils.hpp"

namespace clp_s {
std::shared_ptr<SchemaTree> ReaderUtils::read_schema_tree(std::string const& archives_dir) {
    FileReader schema_tree_reader;
    ZstdDecompressor schema_tree_decompressor;

    std::shared_ptr<SchemaTree> tree = std::make_shared<SchemaTree>();

    schema_tree_reader.open(archives_dir + "/schema_tree");
    schema_tree_decompressor.open(schema_tree_reader, cDecompressorFileReadBufferCapacity);

    size_t num_nodes;
    auto error_code = schema_tree_decompressor.try_read_numeric_value(num_nodes);
    if (ErrorCodeSuccess != error_code) {
        throw OperationFailed(error_code, __FILENAME__, __LINE__);
    }

    for (size_t i = 0; i < num_nodes; i++) {
        int32_t parent_id;
        size_t key_length;
        std::string key;
        uint8_t node_type;

        error_code = schema_tree_decompressor.try_read_numeric_value(parent_id);
        if (ErrorCodeSuccess != error_code) {
            throw OperationFailed(error_code, __FILENAME__, __LINE__);
        }

        error_code = schema_tree_decompressor.try_read_numeric_value(key_length);
        if (ErrorCodeSuccess != error_code) {
            throw OperationFailed(error_code, __FILENAME__, __LINE__);
        }

        error_code = schema_tree_decompressor.try_read_string(key_length, key);
        if (ErrorCodeSuccess != error_code) {
            throw OperationFailed(error_code, __FILENAME__, __LINE__);
        }

        error_code = schema_tree_decompressor.try_read_numeric_value(node_type);
        if (ErrorCodeSuccess != error_code) {
            throw OperationFailed(error_code, __FILENAME__, __LINE__);
        }

        tree->add_node(parent_id, (NodeType)node_type, key);
    }

    schema_tree_decompressor.close();
    schema_tree_reader.close();

    return tree;
}

std::shared_ptr<VariableDictionaryReader> ReaderUtils::get_variable_dictionary_reader(
        std::string const& archive_path
) {
    auto reader = std::make_shared<VariableDictionaryReader>();
    reader->open(archive_path + "/var.dict");
    return reader;
}

std::shared_ptr<LogTypeDictionaryReader> ReaderUtils::get_log_type_dictionary_reader(
        std::string const& archive_path
) {
    auto reader = std::make_shared<LogTypeDictionaryReader>();
    reader->open(archive_path + "/log.dict");
    return reader;
}

std::shared_ptr<LogTypeDictionaryReader> ReaderUtils::get_array_dictionary_reader(
        std::string const& archive_path
) {
    auto reader = std::make_shared<LogTypeDictionaryReader>();
    reader->open(archive_path + "/array.dict");
    return reader;
}

std::shared_ptr<ReaderUtils::SchemaMap> ReaderUtils::read_schemas(std::string const& archives_dir) {
    auto schemas_pointer = std::make_shared<SchemaMap>();
    SchemaMap& schemas = *schemas_pointer;
    FileReader schema_id_reader;
    ZstdDecompressor schema_id_decompressor;

    schema_id_reader.open(archives_dir + "/schema_ids");
    schema_id_decompressor.open(schema_id_reader, cDecompressorFileReadBufferCapacity);

    size_t schema_size;
    auto error_code = schema_id_decompressor.try_read_numeric_value(schema_size);
    if (ErrorCodeSuccess != error_code) {
        throw OperationFailed(error_code, __FILENAME__, __LINE__);
    }

    // TODO: consider decompressing all schemas into the same buffer and providing access to them
    // via const spans.
    for (size_t i = 0; i < schema_size; i++) {
        int32_t schema_id;
        error_code = schema_id_decompressor.try_read_numeric_value(schema_id);
        if (ErrorCodeSuccess != error_code) {
            throw OperationFailed(error_code, __FILENAME__, __LINE__);
        }

        size_t schema_node_size;
        error_code = schema_id_decompressor.try_read_numeric_value(schema_node_size);
        if (ErrorCodeSuccess != error_code) {
            throw OperationFailed(error_code, __FILENAME__, __LINE__);
        }

        auto& schema = schemas[schema_id];
        for (size_t j = 0; j < schema_node_size; j++) {
            int32_t node_id;
            error_code = schema_id_decompressor.try_read_numeric_value(node_id);
            if (ErrorCodeSuccess != error_code) {
                throw OperationFailed(error_code, __FILENAME__, __LINE__);
            }

            // Maintain schema ordering defined at compression time
            schema.insert_unordered(node_id);
        }
    }

    schema_id_decompressor.close();
    schema_id_reader.close();

    return schemas_pointer;
}

std::shared_ptr<TimestampDictionaryReader> ReaderUtils::read_timestamp_dictionary(
        std::string const& archives_dir
) {
    auto reader = std::make_shared<TimestampDictionaryReader>();
    reader->open(archives_dir + "/timestamp.dict");
    reader->read_new_entries();
    reader->close();

    return reader;
}

std::shared_ptr<TimestampDictionaryReader> ReaderUtils::read_local_timestamp_dictionary(
        std::string const& archive_path
) {
    auto reader = std::make_shared<TimestampDictionaryReader>();
    reader->open(archive_path + "/timestamp.dict");
    reader->read_local_entries();
    reader->close();

    return reader;
}

std::vector<std::string> ReaderUtils::get_archives(std::string const& archives_dir) {
    std::vector<std::string> archive_paths;

    if (false == boost::filesystem::is_directory(archives_dir)) {
        throw OperationFailed(ErrorCodeBadParam, __FILENAME__, __LINE__);
    }

    boost::filesystem::directory_iterator iter(archives_dir);
    boost::filesystem::directory_iterator end;
    for (; iter != end; ++iter) {
        if (boost::filesystem::is_directory(iter->path())) {
            archive_paths.push_back(iter->path().string());
        }
    }

    return archive_paths;
}

std::vector<int32_t> ReaderUtils::get_schemas(std::string const& archive_path) {
    std::vector<int32_t> schemas;
    std::string encoded_messages_dir = archive_path + "/encoded_messages";

    boost::filesystem::directory_iterator iter(encoded_messages_dir);
    boost::filesystem::directory_iterator end;

    for (; iter != end; ++iter) {
        if (boost::filesystem::is_regular_file(iter->path())) {
            std::string schema = iter->path().rbegin()->string();
            if (false == schema.empty() && std::all_of(schema.begin(), schema.end(), ::isdigit)) {
                schemas.push_back(std::stoi(schema));
            }
        }
    }

    return schemas;
}

BaseColumnReader* ReaderUtils::append_reader_column(
        SchemaReader* reader,
        int32_t column_id,
        std::shared_ptr<SchemaTree> const& schema_tree,
        std::shared_ptr<VariableDictionaryReader> const& var_dict,
        std::shared_ptr<LogTypeDictionaryReader> const& log_dict,
        std::shared_ptr<LogTypeDictionaryReader> const& array_dict,
        std::shared_ptr<TimestampDictionaryReader> const& timestamp_dict
) {
    BaseColumnReader* column_reader = nullptr;
    auto node = schema_tree->get_node(column_id);
    std::string key_name = node->get_key_name();
    switch (node->get_type()) {
        case NodeType::INTEGER:
            column_reader = new Int64ColumnReader(key_name, column_id);
            break;
        case NodeType::FLOAT:
            column_reader = new FloatColumnReader(key_name, column_id);
            break;
        case NodeType::CLPSTRING:
            column_reader = new ClpStringColumnReader(key_name, column_id, var_dict, log_dict);
            break;
        case NodeType::VARSTRING:
            column_reader = new VariableStringColumnReader(key_name, column_id, var_dict);
            break;
        case NodeType::BOOLEAN:
            reader->append_column(new BooleanColumnReader(key_name, column_id));
            break;
        case NodeType::ARRAY:
            column_reader
                    = new ClpStringColumnReader(key_name, column_id, var_dict, array_dict, true);
            break;
        case NodeType::DATESTRING:
            column_reader = new DateStringColumnReader(key_name, column_id, timestamp_dict);
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

void ReaderUtils::append_reader_columns(
        SchemaReader* reader,
        Schema const& columns,
        std::shared_ptr<SchemaTree> const& schema_tree,
        std::shared_ptr<VariableDictionaryReader> const& var_dict,
        std::shared_ptr<LogTypeDictionaryReader> const& log_dict,
        std::shared_ptr<LogTypeDictionaryReader> const& array_dict,
        std::shared_ptr<TimestampDictionaryReader> const& timestamp_dict,
        bool extract_timestamp
) {
    auto timestamp_column_ids = timestamp_dict->get_authoritative_timestamp_column_ids();

    for (int32_t column_id : columns) {
        BaseColumnReader* column_reader = append_reader_column(
                reader,
                column_id,
                schema_tree,
                var_dict,
                log_dict,
                array_dict,
                timestamp_dict
        );

        if (extract_timestamp && column_reader && timestamp_column_ids.count(column_id) > 0) {
            reader->mark_column_as_timestamp(column_reader);
        }
    }
}
}  // namespace clp_s
