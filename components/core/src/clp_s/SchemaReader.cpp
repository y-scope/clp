#include "SchemaReader.hpp"

#include <stack>

#include "BufferViewReader.hpp"
#include "Schema.hpp"

namespace clp_s {
void SchemaReader::append_column(BaseColumnReader* column_reader) {
    m_column_map[column_reader->get_id()] = column_reader;
    m_columns.push_back(column_reader);
}

void SchemaReader::append_unordered_column(BaseColumnReader* column_reader) {
    m_columns.push_back(column_reader);
}

void SchemaReader::mark_column_as_timestamp(BaseColumnReader* column_reader) {
    m_timestamp_column = column_reader;
    if (m_timestamp_column->get_type() == NodeType::DateString) {
        m_get_timestamp = [this]() {
            return static_cast<DateStringColumnReader*>(m_timestamp_column)
                    ->get_encoded_time(m_cur_message);
        };
    } else if (m_timestamp_column->get_type() == NodeType::Integer) {
        m_get_timestamp = [this]() {
            return std::get<int64_t>(static_cast<Int64ColumnReader*>(m_timestamp_column)
                                             ->extract_value(m_cur_message));
        };
    } else if (m_timestamp_column->get_type() == NodeType::Float) {
        m_get_timestamp = [this]() {
            return static_cast<epochtime_t>(
                    std::get<double>(static_cast<FloatColumnReader*>(m_timestamp_column)
                                             ->extract_value(m_cur_message))
            );
        };
    }
}

void SchemaReader::load(ZstdDecompressor& decompressor, size_t uncompressed_size) {
    if (uncompressed_size > m_table_buffer_size) {
        m_table_buffer = std::make_unique<char[]>(uncompressed_size);
        m_table_buffer_size = uncompressed_size;
    }
    auto error = decompressor.try_read_exact_length(m_table_buffer.get(), uncompressed_size);
    if (ErrorCodeSuccess != error) {
        throw OperationFailed(error, __FILENAME__, __LINE__);
    }

    BufferViewReader buffer_reader{m_table_buffer.get(), uncompressed_size};
    for (auto& reader : m_columns) {
        reader->load(buffer_reader, m_num_messages);
    }
    if (buffer_reader.get_remaining_size() > 0) {
        throw OperationFailed(ErrorCodeCorrupt, __FILENAME__, __LINE__);
    }
}

void SchemaReader::generate_json_string() {
    m_json_serializer.reset();
    m_json_serializer.begin_document();
    size_t column_id_index = 0;
    BaseColumnReader* column;
    JsonSerializer::Op op;
    while (m_json_serializer.get_next_op(op)) {
        switch (op) {
            case JsonSerializer::Op::BeginObject: {
                m_json_serializer.begin_object();
                break;
            }
            case JsonSerializer::Op::EndObject: {
                m_json_serializer.end_object();
                break;
            }
            case JsonSerializer::Op::BeginArray: {
                m_json_serializer.begin_array();
                break;
            }
            case JsonSerializer::Op::EndArray: {
                m_json_serializer.end_array();
                break;
            }
            case JsonSerializer::Op::AddIntField: {
                column = m_reordered_columns[column_id_index++];
                auto const& name = m_global_schema_tree->get_node(column->get_id()).get_key_name();
                m_json_serializer.append_key(name);
                m_json_serializer.append_value(
                        std::to_string(std::get<int64_t>(column->extract_value(m_cur_message)))
                );
                break;
            }
            case JsonSerializer::Op::AddIntValue: {
                column = m_reordered_columns[column_id_index++];
                m_json_serializer.append_value(
                        std::to_string(std::get<int64_t>(column->extract_value(m_cur_message)))
                );
                break;
            }
            case JsonSerializer::Op::AddFloatField: {
                column = m_reordered_columns[column_id_index++];
                auto const& name = m_global_schema_tree->get_node(column->get_id()).get_key_name();
                m_json_serializer.append_key(name);
                m_json_serializer.append_value(
                        std::to_string(std::get<double>(column->extract_value(m_cur_message)))
                );
                break;
            }
            case JsonSerializer::Op::AddFloatValue: {
                column = m_reordered_columns[column_id_index++];
                m_json_serializer.append_value(
                        std::to_string(std::get<double>(column->extract_value(m_cur_message)))
                );
                break;
            }
            case JsonSerializer::Op::AddBoolField: {
                column = m_reordered_columns[column_id_index++];
                auto const& name = m_global_schema_tree->get_node(column->get_id()).get_key_name();
                m_json_serializer.append_key(name);
                m_json_serializer.append_value(
                        std::get<uint8_t>(column->extract_value(m_cur_message)) != 0 ? "true"
                                                                                     : "false"
                );
                break;
            }
            case JsonSerializer::Op::AddBoolValue: {
                column = m_reordered_columns[column_id_index++];
                m_json_serializer.append_value(
                        std::get<uint8_t>(column->extract_value(m_cur_message)) != 0 ? "true"
                                                                                     : "false"
                );
                break;
            }
            case JsonSerializer::Op::AddStringField: {
                column = m_reordered_columns[column_id_index++];
                auto const& name = m_global_schema_tree->get_node(column->get_id()).get_key_name();
                m_json_serializer.append_key(name);
                m_json_serializer.append_value_from_column_with_quotes(column, m_cur_message);
                break;
            }
            case JsonSerializer::Op::AddStringValue: {
                column = m_reordered_columns[column_id_index++];
                m_json_serializer.append_value_from_column_with_quotes(column, m_cur_message);
                break;
            }
            case JsonSerializer::Op::AddArrayField: {
                column = m_reordered_columns[column_id_index++];
                m_json_serializer.append_key(
                        m_global_schema_tree->get_node(column->get_id()).get_key_name()
                );
                m_json_serializer.append_value_from_column(column, m_cur_message);
                break;
            }
            case JsonSerializer::Op::AddNullField: {
                m_json_serializer.append_key();
                m_json_serializer.append_value("null");
                break;
            }
            case JsonSerializer::Op::AddNullValue: {
                m_json_serializer.append_value("null");
                break;
            }
        }
    }

    m_json_serializer.end_document();
}

bool SchemaReader::get_next_message(std::string& message) {
    if (m_cur_message >= m_num_messages) {
        return false;
    }

    if (false == m_serializer_initialized) {
        initialize_serializer();
    }
    generate_json_string();

    message = m_json_serializer.get_serialized_string();

    if (message.back() != '\n') {
        message += '\n';
    }

    m_cur_message++;
    return true;
}

bool SchemaReader::get_next_message(std::string& message, FilterClass* filter) {
    while (m_cur_message < m_num_messages) {
        if (false == filter->filter(m_cur_message)) {
            m_cur_message++;
            continue;
        }

        if (m_should_marshal_records) {
            if (false == m_serializer_initialized) {
                initialize_serializer();
            }
            generate_json_string();
            message = m_json_serializer.get_serialized_string();

            if (message.back() != '\n') {
                message += '\n';
            }
        }

        m_cur_message++;
        return true;
    }

    return false;
}

bool SchemaReader::get_next_message_with_timestamp(
        std::string& message,
        epochtime_t& timestamp,
        FilterClass* filter
) {
    // TODO: If we already get max_num_results messages, we can skip messages
    // with the timestamp less than the smallest timestamp in the priority queue
    while (m_cur_message < m_num_messages) {
        if (false == filter->filter(m_cur_message)) {
            m_cur_message++;
            continue;
        }

        if (m_should_marshal_records) {
            if (false == m_serializer_initialized) {
                initialize_serializer();
            }
            generate_json_string();
            message = m_json_serializer.get_serialized_string();

            if (message.back() != '\n') {
                message += '\n';
            }
        }

        timestamp = m_get_timestamp();

        m_cur_message++;
        return true;
    }

    return false;
}

void SchemaReader::initialize_filter(FilterClass* filter) {
    filter->init(this, m_schema_id, m_columns);
}

void SchemaReader::generate_local_tree(int32_t global_id) {
    std::stack<int32_t> global_id_stack;
    global_id_stack.emplace(global_id);
    do {
        auto const& node = m_global_schema_tree->get_node(global_id_stack.top());
        int32_t parent_id = node.get_parent_id();

        auto it = m_global_id_to_local_id.find(parent_id);
        if (-1 != parent_id && it == m_global_id_to_local_id.end()) {
            global_id_stack.emplace(parent_id);
            continue;
        }

        int32_t local_id = m_local_schema_tree.add_node(
                parent_id == -1 ? -1 : m_global_id_to_local_id[parent_id],
                node.get_type(),
                node.get_key_name()
        );

        m_global_id_to_local_id[global_id_stack.top()] = local_id;
        m_local_id_to_global_id[local_id] = global_id_stack.top();
        global_id_stack.pop();
    } while (false == global_id_stack.empty());
}

void SchemaReader::mark_unordered_object(
        size_t column_reader_start,
        int32_t mst_subtree_root,
        std::span<int32_t> schema
) {
    m_global_id_to_unordered_object.emplace(
            mst_subtree_root,
            std::make_pair(column_reader_start, schema)
    );
}

int32_t SchemaReader::get_first_column_in_span(std::span<int32_t> schema) {
    for (int32_t column_id : schema) {
        if (false == Schema::schema_entry_is_unordered_object(column_id)) {
            return column_id;
        }
    }
    return -1;
}

void SchemaReader::initialize_serializer() {
    if (m_serializer_initialized) {
        return;
    }

    m_serializer_initialized = true;

    for (int32_t global_column_id : m_ordered_schema) {
        generate_local_tree(global_column_id);
    }

    for (auto it = m_global_id_to_unordered_object.begin();
         it != m_global_id_to_unordered_object.end();
         ++it)
    {
        generate_local_tree(it->first);
    }

    // TODO: this code will have to change once we allow mixing log lines parsed by different
    // parsers.
    generate_json_template(0);
}

void SchemaReader::generate_json_template(int32_t id) {
    auto const& node = m_local_schema_tree.get_node(id);
    auto const& children_ids = node.get_children_ids();

    for (int32_t child_id : children_ids) {
        int32_t child_global_id = m_local_id_to_global_id[child_id];
        auto const& child_node = m_local_schema_tree.get_node(child_id);
        std::string const& key = child_node.get_key_name();
        switch (child_node.get_type()) {
            case NodeType::Object: {
                m_json_serializer.add_op(JsonSerializer::Op::BeginObject);
                m_json_serializer.add_special_key(key);
                generate_json_template(child_id);
                m_json_serializer.add_op(JsonSerializer::Op::EndObject);
                break;
            }
            case NodeType::UnstructuredArray: {
                m_json_serializer.add_op(JsonSerializer::Op::AddArrayField);
                m_reordered_columns.push_back(m_column_map[child_global_id]);
                break;
            }
            case NodeType::StructuredArray: {
                // Note: Marshalling structured arrays is left intentionally stubbed out so that we
                // can split up the PR for supporting structurized arrays.
                m_json_serializer.add_op(JsonSerializer::Op::BeginArray);
                m_json_serializer.add_special_key(child_node.get_key_name());
                m_json_serializer.add_op(JsonSerializer::Op::EndArray);
                break;
            }
            case NodeType::Integer: {
                m_json_serializer.add_op(JsonSerializer::Op::AddIntField);
                m_reordered_columns.push_back(m_column_map[child_global_id]);
                break;
            }
            case NodeType::Float: {
                m_json_serializer.add_op(JsonSerializer::Op::AddFloatField);
                m_reordered_columns.push_back(m_column_map[child_global_id]);
                break;
            }
            case NodeType::Boolean: {
                m_json_serializer.add_op(JsonSerializer::Op::AddBoolField);
                m_reordered_columns.push_back(m_column_map[child_global_id]);
                break;
            }
            case NodeType::ClpString:
            case NodeType::VarString:
            case NodeType::DateString: {
                m_json_serializer.add_op(JsonSerializer::Op::AddStringField);
                m_reordered_columns.push_back(m_column_map[child_global_id]);
                break;
            }
            case NodeType::NullValue: {
                m_json_serializer.add_op(JsonSerializer::Op::AddNullField);
                m_json_serializer.add_special_key(key);
                break;
            }
            case NodeType::Unknown:
                break;
        }
    }
}
}  // namespace clp_s
