#include "ReaderUtils.hpp"

#include <string>
#include <string_view>

#include "archive_constants.hpp"

namespace clp_s {
std::shared_ptr<SchemaTree> ReaderUtils::read_schema_tree(ArchiveReaderAdaptor& adaptor) {
    ZstdDecompressor schema_tree_decompressor;
    std::shared_ptr<SchemaTree> tree = std::make_shared<SchemaTree>();

    auto schema_tree_reader
            = adaptor.checkout_reader_for_section(constants::cArchiveSchemaTreeFile);
    schema_tree_decompressor.open(*schema_tree_reader, cDecompressorFileReadBufferCapacity);

    size_t num_nodes;
    auto error_code = schema_tree_decompressor.try_read_numeric_value(num_nodes);
    if (ErrorCodeSuccess != error_code) {
        throw OperationFailed(error_code, __FILENAME__, __LINE__);
    }

    std::string key;
    for (size_t i = 0; i < num_nodes; i++) {
        int32_t parent_id;
        size_t key_length;
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
    adaptor.checkin_reader_for_section(constants::cArchiveSchemaTreeFile);

    return tree;
}

std::shared_ptr<VariableDictionaryReader> ReaderUtils::get_variable_dictionary_reader(
        ArchiveReaderAdaptor& adaptor
) {
    auto reader = std::make_shared<VariableDictionaryReader>(adaptor);
    reader->open(constants::cArchiveVarDictFile);
    return reader;
}

std::shared_ptr<LogTypeDictionaryReader> ReaderUtils::get_log_type_dictionary_reader(
        ArchiveReaderAdaptor& adaptor
) {
    auto reader = std::make_shared<LogTypeDictionaryReader>(adaptor);
    reader->open(constants::cArchiveLogDictFile);
    return reader;
}

std::shared_ptr<LogTypeDictionaryReader> ReaderUtils::get_array_dictionary_reader(
        ArchiveReaderAdaptor& adaptor
) {
    auto reader = std::make_shared<LogTypeDictionaryReader>(adaptor);
    reader->open(constants::cArchiveArrayDictFile);
    return reader;
}

std::shared_ptr<ReaderUtils::SchemaMap> ReaderUtils::read_schemas(ArchiveReaderAdaptor& adaptor) {
    auto schemas_pointer = std::make_unique<SchemaMap>();
    SchemaMap& schemas = *schemas_pointer;
    ZstdDecompressor schema_id_decompressor;

    auto schema_id_reader = adaptor.checkout_reader_for_section(constants::cArchiveSchemaMapFile);
    schema_id_decompressor.open(*schema_id_reader, cDecompressorFileReadBufferCapacity);

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
                reinterpret_cast<char*>(schema.data()),
                sizeof(int32_t) * schema_node_size
        );
        if (ErrorCodeSuccess != error_code) {
            throw OperationFailed(error_code, __FILENAME__, __LINE__);
        }
        schema.set_num_ordered(num_ordered_nodes);
    }

    schema_id_decompressor.close();
    adaptor.checkin_reader_for_section(constants::cArchiveSchemaMapFile);

    return schemas_pointer;
}
}  // namespace clp_s
