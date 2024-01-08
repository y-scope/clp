#include "SchemaReader.hpp"

namespace clp_s {
void SchemaReader::open(std::string path) {
    m_path = std::move(path);
    m_local_schema_tree = std::make_shared<SchemaTree>();
}

void SchemaReader::close() {
    for (auto& i : m_columns) {
        delete i;
    }

    m_column_map.clear();
    m_global_id_to_local_id.clear();
}

void SchemaReader::append_column(BaseColumnReader* column_reader) {
    m_column_map[column_reader->get_id()] = column_reader;
    m_columns.push_back(column_reader);
    generate_local_tree(column_reader->get_id());
}

void SchemaReader::append_column(int32_t id) {
    generate_local_tree(id);
}

void SchemaReader::load() {
    constexpr size_t cDecompressorFileReadBufferCapacity = 64 * 1024;  // 64 KB

    m_file_reader.open(m_path);
    m_decompressor.open(m_file_reader, cDecompressorFileReadBufferCapacity);

    m_file_reader.seek_from_begin(0);
    m_file_reader.read_numeric_value(m_num_messages, false);

    for (auto& reader : m_columns) {
        reader->load(m_decompressor, m_num_messages);
    }

    m_decompressor.close();
    m_file_reader.close();

    generate_json_template(0);
}

bool SchemaReader::get_next_message(std::string& message) {
    if (m_cur_message >= m_num_messages) {
        return false;
    }

    m_json_serializer->reset();
    m_json_serializer->begin_document();
    size_t column_id_index = 0;
    BaseColumnReader* column;
    JsonSerializer::Op op;
    while (m_json_serializer->get_next_op(op)) {
        switch (op) {
            case JsonSerializer::Op::BeginObject: {
                m_json_serializer->begin_object();
                break;
            }
            case JsonSerializer::Op::EndObject: {
                m_json_serializer->end_object();
                break;
            }
            case JsonSerializer::Op::AddIntField: {
                column = m_reordered_columns[column_id_index++];
                m_json_serializer->append_key(column->get_name());
                m_json_serializer->append_value(
                        std::to_string(std::get<int64_t>(column->extract_value(m_cur_message)))
                );
                break;
            }
            case JsonSerializer::Op::AddFloatField: {
                column = m_reordered_columns[column_id_index++];
                m_json_serializer->append_key(column->get_name());
                m_json_serializer->append_value(
                        std::to_string(std::get<double>(column->extract_value(m_cur_message)))
                );
                break;
            }
            case JsonSerializer::Op::AddBoolField: {
                column = m_reordered_columns[column_id_index++];
                m_json_serializer->append_key(column->get_name());
                m_json_serializer->append_value(
                        std::get<uint8_t>(column->extract_value(m_cur_message)) != 0 ? "true"
                                                                                     : "false"
                );
                break;
            }
            case JsonSerializer::Op::AddStringField: {
                column = m_reordered_columns[column_id_index++];
                m_json_serializer->append_key(column->get_name());
                m_json_serializer->append_value_with_quotes(
                        std::get<std::string>(column->extract_value(m_cur_message))
                );
                break;
            }
            case JsonSerializer::Op::AddArrayField: {
                column = m_reordered_columns[column_id_index++];
                m_json_serializer->append_key(column->get_name());
                m_json_serializer->append_value(
                        std::get<std::string>(column->extract_value(m_cur_message))
                );
                break;
            }
            case JsonSerializer::Op::AddNullField: {
                m_json_serializer->append_key();
                m_json_serializer->append_value("null");
                break;
            }
        }
    }

    m_json_serializer->end_document();

    message = m_json_serializer->get_serialized_string();

    if (message.back() != '\n') {
        message += '\n';
    }

    m_cur_message++;
    return true;
}

bool SchemaReader::get_next_message(std::string& message, FilterClass* filter) {
    while (m_cur_message < m_num_messages) {
        if (false == filter->filter(m_cur_message, m_extracted_values)) {
            m_cur_message++;
            continue;
        }

        m_json_serializer->reset();
        m_json_serializer->begin_document();
        size_t column_id_index = 0;
        BaseColumnReader* column;
        JsonSerializer::Op op;
        while (m_json_serializer->get_next_op(op)) {
            switch (op) {
                case JsonSerializer::Op::BeginObject: {
                    m_json_serializer->begin_object();
                    break;
                }
                case JsonSerializer::Op::EndObject: {
                    m_json_serializer->end_object();
                    break;
                }
                case JsonSerializer::Op::AddIntField: {
                    column = m_reordered_columns[column_id_index++];
                    m_json_serializer->append_key(column->get_name());
                    m_json_serializer->append_value(
                            std::to_string(std::get<int64_t>(m_extracted_values[column->get_id()]))
                    );
                    break;
                }
                case JsonSerializer::Op::AddFloatField: {
                    column = m_reordered_columns[column_id_index++];
                    m_json_serializer->append_key(column->get_name());
                    m_json_serializer->append_value(
                            std::to_string(std::get<double>(m_extracted_values[column->get_id()]))
                    );
                    break;
                }
                case JsonSerializer::Op::AddBoolField: {
                    column = m_reordered_columns[column_id_index++];
                    m_json_serializer->append_key(column->get_name());
                    m_json_serializer->append_value(
                            std::get<uint8_t>(m_extracted_values[column->get_id()]) != 0 ? "true"
                                                                                         : "false"
                    );
                    break;
                }
                case JsonSerializer::Op::AddStringField: {
                    column = m_reordered_columns[column_id_index++];
                    m_json_serializer->append_key(column->get_name());
                    m_json_serializer->append_value_with_quotes(
                            std::get<std::string>(m_extracted_values[column->get_id()])
                    );
                    break;
                }
                case JsonSerializer::Op::AddArrayField: {
                    column = m_reordered_columns[column_id_index++];
                    m_json_serializer->append_key(column->get_name());
                    m_json_serializer->append_value(
                            std::get<std::string>(m_extracted_values[column->get_id()])
                    );
                    break;
                }
                case JsonSerializer::Op::AddNullField: {
                    m_json_serializer->append_key();
                    m_json_serializer->append_value("null");
                    break;
                }
            }
        }

        m_json_serializer->end_document();

        message = m_json_serializer->get_serialized_string();

        if (message.back() != '\n') {
            message += '\n';
        }

        m_cur_message++;
        return true;
    }

    return false;
}

void SchemaReader::initialize_filter(FilterClass* filter) {
    filter->init(this, m_schema_id, m_column_map);
}

void SchemaReader::generate_local_tree(int32_t global_id) {
    auto node = m_global_schema_tree->get_node(global_id);
    int32_t parent_id = node->get_parent_id();

    if (parent_id != -1 && m_global_id_to_local_id.find(parent_id) == m_global_id_to_local_id.end())
    {
        generate_local_tree(parent_id);
    }

    int32_t local_id = m_local_schema_tree->add_node(
            parent_id == -1 ? -1 : m_global_id_to_local_id[parent_id],
            node->get_type(),
            node->get_key_name()
    );
    m_global_id_to_local_id[global_id] = local_id;
    m_local_id_to_global_id[local_id] = global_id;
}

void SchemaReader::generate_json_template(int32_t id) {
    auto node = m_local_schema_tree->get_node(id);
    auto children_ids = node->get_children_ids();

    for (int32_t child_id : children_ids) {
        int32_t child_global_id = m_local_id_to_global_id[child_id];
        auto child_node = m_local_schema_tree->get_node(child_id);
        std::string const& key = child_node->get_key_name();
        switch (child_node->get_type()) {
            case NodeType::OBJECT: {
                m_json_serializer->add_op(JsonSerializer::Op::BeginObject);
                m_json_serializer->add_special_key(key);
                generate_json_template(child_id);
                m_json_serializer->add_op(JsonSerializer::Op::EndObject);
                break;
            }
            case NodeType::ARRAY: {
                m_json_serializer->add_op(JsonSerializer::Op::AddArrayField);
                m_reordered_columns.push_back(m_column_map[child_global_id]);
                break;
            }
            case NodeType::INTEGER: {
                m_json_serializer->add_op(JsonSerializer::Op::AddIntField);
                m_reordered_columns.push_back(m_column_map[child_global_id]);
                break;
            }
            case NodeType::FLOAT: {
                m_json_serializer->add_op(JsonSerializer::Op::AddFloatField);
                m_reordered_columns.push_back(m_column_map[child_global_id]);
                break;
            }
            case NodeType::BOOLEAN: {
                m_json_serializer->add_op(JsonSerializer::Op::AddBoolField);
                m_reordered_columns.push_back(m_column_map[child_global_id]);
                break;
            }
            case NodeType::CLPSTRING:
            case NodeType::VARSTRING:
            case NodeType::DATESTRING:
            case NodeType::FLOATDATESTRING: {
                m_json_serializer->add_op(JsonSerializer::Op::AddStringField);
                m_reordered_columns.push_back(m_column_map[child_global_id]);
                break;
            }
            case NodeType::NULLVALUE: {
                m_json_serializer->add_op(JsonSerializer::Op::AddNullField);
                m_json_serializer->add_special_key(key);
                break;
            }
        }
    }
}
}  // namespace clp_s
