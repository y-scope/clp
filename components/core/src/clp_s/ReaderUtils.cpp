#include "ReaderUtils.hpp"

#include "archive_constants.hpp"

namespace clp_s {
std::shared_ptr<SchemaTree> ReaderUtils::read_schema_tree(std::string const& archives_dir) {
    FileReader schema_tree_reader;
    ZstdDecompressor schema_tree_decompressor;

    std::shared_ptr<SchemaTree> tree = std::make_shared<SchemaTree>();

    schema_tree_reader.open(archives_dir + constants::cArchiveSchemaTreeFile);
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
    reader->open(archive_path + constants::cArchiveVarDictFile);
    return reader;
}

std::shared_ptr<LogTypeDictionaryReader> ReaderUtils::get_log_type_dictionary_reader(
        std::string const& archive_path
) {
    auto reader = std::make_shared<LogTypeDictionaryReader>();
    reader->open(archive_path + constants::cArchiveLogDictFile);
    return reader;
}

std::shared_ptr<LogTypeDictionaryReader> ReaderUtils::get_array_dictionary_reader(
        std::string const& archive_path
) {
    auto reader = std::make_shared<LogTypeDictionaryReader>();
    reader->open(archive_path + constants::cArchiveArrayDictFile);
    return reader;
}

std::shared_ptr<TimestampDictionaryReader> ReaderUtils::get_timestamp_dictionary_reader(
        std::string const& archive_path
) {
    auto reader = std::make_shared<TimestampDictionaryReader>();
    reader->open(archive_path + constants::cArchiveTimestampDictFile);
    return reader;
}

std::shared_ptr<ReaderUtils::SchemaMap> ReaderUtils::read_schemas(std::string const& archives_dir) {
    auto schemas_pointer = std::make_unique<SchemaMap>();
    SchemaMap& schemas = *schemas_pointer;
    FileReader schema_id_reader;
    ZstdDecompressor schema_id_decompressor;

    schema_id_reader.open(archives_dir + constants::cArchiveSchemaMapFile);
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

        uint32_t schema_node_size;
        error_code = schema_id_decompressor.try_read_numeric_value(schema_node_size);
        if (ErrorCodeSuccess != error_code) {
            throw OperationFailed(error_code, __FILENAME__, __LINE__);
        }

        uint32_t num_ordered_nodes;
        error_code = schema_id_decompressor.try_read_numeric_value(num_ordered_nodes);
        if (ErrorCodeSuccess != error_code) {
            throw OperationFailed(error_code, __FILENAME__, __LINE__);
        }

        auto& schema = schemas[schema_id];
        if (0 == schema_node_size) {
            continue;
        }
        schema.resize(schema_node_size);
        error_code = schema_id_decompressor.try_read_exact_length(
                reinterpret_cast<char*>(schema.begin().base()),
                sizeof(int32_t) * schema_node_size
        );
        if (ErrorCodeSuccess != error_code) {
            throw OperationFailed(error_code, __FILENAME__, __LINE__);
        }
        schema.set_num_ordered(num_ordered_nodes);
    }

    schema_id_decompressor.close();
    schema_id_reader.close();

    return schemas_pointer;
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

}  // namespace clp_s
