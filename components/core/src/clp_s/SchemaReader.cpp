#include "SchemaReader.hpp"

#include <stack>

#include "Schema.hpp"

namespace clp_s {
void SchemaReader::append_column(BaseColumnReader* column_reader) {
    m_column_map[column_reader->get_id()] = column_reader;
    m_columns.push_back(column_reader);
    // The local schema tree is only necessary for generating the JSON template to marshal records.
    if (m_should_marshal_records) {
        generate_local_tree(column_reader->get_id(), NodeType::Unknown);
    }
}

int32_t SchemaReader::append_unordered_column(BaseColumnReader* column_reader, NodeType node_type) {
    m_columns.push_back(column_reader);
    if (m_should_marshal_records) {
        return generate_local_tree(column_reader->get_id(), node_type);
    }
    return column_reader->get_id();
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

void SchemaReader::append_column(int32_t id) {
    // The local schema tree is only necessary for generating the JSON template to marshal records.
    if (m_should_marshal_records) {
        generate_local_tree(id, NodeType::Unknown);
    }
}

int32_t SchemaReader::append_unordered_column(int32_t id, NodeType node_type) {
    return generate_local_tree(id, node_type);
}

void SchemaReader::load(ZstdDecompressor& decompressor) {
    for (auto& reader : m_columns) {
        reader->load(decompressor, m_num_messages);
    }

    if (m_should_marshal_records) {
        generate_json_template(0);
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
            case JsonSerializer::Op::BeginDocument: {
                m_json_serializer.begin_document();
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
            case JsonSerializer::Op::BeginArrayDocument: {
                m_json_serializer.begin_array_document();
                break;
            }
            case JsonSerializer::Op::AddIntField: {
                column = m_reordered_columns[column_id_index++];
                auto const& name = column->get_name();
                if (false == name.empty()) {
                    m_json_serializer.append_key(column->get_name());
                }
                m_json_serializer.append_value(
                        std::to_string(std::get<int64_t>(column->extract_value(m_cur_message)))
                );
                break;
            }
            case JsonSerializer::Op::AddFloatField: {
                column = m_reordered_columns[column_id_index++];
                auto const& name = column->get_name();
                if (false == name.empty()) {
                    m_json_serializer.append_key(column->get_name());
                }
                m_json_serializer.append_value(
                        std::to_string(std::get<double>(column->extract_value(m_cur_message)))
                );
                break;
            }
            case JsonSerializer::Op::AddBoolField: {
                column = m_reordered_columns[column_id_index++];
                auto const& name = column->get_name();
                if (false == name.empty()) {
                    m_json_serializer.append_key(column->get_name());
                }
                m_json_serializer.append_value(
                        std::get<uint8_t>(column->extract_value(m_cur_message)) != 0 ? "true"
                                                                                     : "false"
                );
                break;
            }
            case JsonSerializer::Op::AddStringField: {
                column = m_reordered_columns[column_id_index++];
                auto const& name = column->get_name();
                if (false == name.empty()) {
                    m_json_serializer.append_key(column->get_name());
                }
                m_json_serializer.append_value_with_quotes(
                        std::get<std::string>(column->extract_value(m_cur_message))
                );
                break;
            }
            case JsonSerializer::Op::AddArrayField: {
                column = m_reordered_columns[column_id_index++];
                m_json_serializer.append_key(column->get_name());
                m_json_serializer.append_value(
                        std::get<std::string>(column->extract_value(m_cur_message))
                );
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
        if (false == filter->filter(m_cur_message, m_extracted_values)) {
            m_cur_message++;
            continue;
        }

        if (m_should_marshal_records) {
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
        if (false == filter->filter(m_cur_message, m_extracted_values)) {
            m_cur_message++;
            continue;
        }

        if (m_should_marshal_records) {
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
    filter->init(this, m_schema_id, m_column_map);
}

int32_t SchemaReader::generate_local_tree(int32_t global_id, NodeType node_type) {
    auto it = m_global_id_to_local_id.find(global_id);
    if (m_global_id_to_local_id.end() != it) {
        return INT32_MAX;
    }
    std::stack<int32_t> global_id_stack;
    global_id_stack.emplace(global_id);
    int32_t smallest_matching_mst_node_id = INT32_MAX;
    do {
        auto node = m_global_schema_tree->get_node(global_id_stack.top());
        int32_t parent_id = node->get_parent_id();

        auto it = m_global_id_to_local_id.find(parent_id);
        if (-1 != parent_id && it == m_global_id_to_local_id.end()) {
            global_id_stack.emplace(parent_id);
            continue;
        }

        int32_t local_id = m_local_schema_tree->add_node(
                parent_id == -1 ? -1 : m_global_id_to_local_id[parent_id],
                node->get_type(),
                node->get_key_name()
        );
        if (node->get_type() == node_type && local_id < smallest_matching_mst_node_id) {
            smallest_matching_mst_node_id = local_id;
        }
        m_global_id_to_local_id[global_id_stack.top()] = local_id;
        m_local_id_to_global_id[local_id] = global_id_stack.top();
        global_id_stack.pop();
    } while (false == global_id_stack.empty());
    return smallest_matching_mst_node_id;
}

void SchemaReader::mark_unordered_object(
        size_t column_reader_start,
        int32_t mst_subtree_root,
        Span<int32_t> schema
) {
    m_local_id_to_unordered_object.emplace(
            mst_subtree_root,
            std::make_pair(column_reader_start, schema)
    );
}

struct InternalGeneratorState {
    size_t end_pos;
    size_t repetitions;
    JsonSerializer::Op operation;
};

void SchemaReader::generate_structured_array_template(int32_t id) {
    auto object_info_it = m_local_id_to_unordered_object.find(id);
    if (m_local_id_to_unordered_object.end() == object_info_it) {
        return;
    }

    std::stack<int32_t> parent_context;
    parent_context.push(id);
    std::stack<InternalGeneratorState> state;

    std::vector<size_t> running_object_prefix;
    size_t column_start = object_info_it->second.first;
    Span<int32_t> schema = object_info_it->second.second;
    for (size_t i = 0; i < schema.size(); ++i) {
        int32_t column_id = schema[i];
        if (Schema::schema_entry_is_unordered_object(column_id)) {
            running_object_prefix.push_back(i);
            continue;
        }

        while (false == state.empty() && i > state.top().end_pos) {
            for (size_t i = 0; i < state.top().repetitions; ++i) {
                m_json_serializer.add_op(state.top().operation);
            }
            state.pop();
            parent_context.pop();
        }

        if (false == running_object_prefix.empty()) {
            for (size_t entry_idx : running_object_prefix) {
                int32_t unordered_object = schema[entry_idx];
                size_t end_pos = Schema::get_unordered_object_length(unordered_object) + entry_idx;
                switch (Schema::get_unordered_object_type(unordered_object)) {
                    case NodeType::StructuredArray: {
                        // m_json_serializer.add_op(JsonSerializer::Op::BeginArrayDocument);
                        auto r = generate_unordered_prefix(column_id, parent_context.top());
                        parent_context.push(r.second);
                        state.emplace(InternalGeneratorState{
                                end_pos,
                                r.first,
                                JsonSerializer::Op::EndArray
                        });
                        break;
                    }
                    case NodeType::Object: {
                        // m_json_serializer.add_op(JsonSerializer::Op::BeginDocument);
                        auto r = generate_unordered_prefix(column_id, parent_context.top());
                        parent_context.push(r.second);
                        state.emplace(InternalGeneratorState{
                                end_pos,
                                r.first,
                                JsonSerializer::Op::EndObject
                        });
                        break;
                    }
                    default:
                        // FIXME: this should be unreachable, so we should throw
                        return;
                }
            }
            running_object_prefix.clear();
        }

        auto node = m_local_schema_tree->get_node(column_id);
        std::string const& key = node->get_key_name();
        switch (node->get_type()) {
            case NodeType::Object: {
                if (key.empty()) {
                    m_json_serializer.add_op(JsonSerializer::Op::BeginDocument);
                    m_json_serializer.add_op(JsonSerializer::Op::EndObject);
                    break;
                }
                auto r = generate_unordered_prefix(column_id, parent_context.top());
                m_json_serializer.add_op(JsonSerializer::Op::BeginDocument);
                m_json_serializer.add_special_key(key);
                m_json_serializer.add_op(JsonSerializer::Op::EndObject);
                for (size_t i = 0; i < r.first; ++i) {
                    m_json_serializer.add_op(JsonSerializer::Op::EndObject);
                }
            }
            case NodeType::StructuredArray: {
                // I'm pretty sure this is the only case for structured arrays because of how the
                // parser was written
                m_json_serializer.add_op(JsonSerializer::Op::BeginArray);
                m_json_serializer.add_op(JsonSerializer::Op::EndArray);
            }
            case NodeType::Integer: {
                m_json_serializer.add_op(JsonSerializer::Op::AddIntField);
                m_reordered_columns.push_back(m_columns[column_start++]);
                break;
            }
            case NodeType::Float: {
                m_json_serializer.add_op(JsonSerializer::Op::AddFloatField);
                m_reordered_columns.push_back(m_columns[column_start++]);
                break;
            }
            case NodeType::Boolean: {
                m_json_serializer.add_op(JsonSerializer::Op::AddBoolField);
                m_reordered_columns.push_back(m_columns[column_start++]);
                break;
            }
            case NodeType::ClpString:
            case NodeType::VarString: {
                m_json_serializer.add_op(JsonSerializer::Op::AddStringField);
                m_reordered_columns.push_back(m_columns[column_start++]);
                break;
            }
            case NodeType::NullValue: {
                if (key.empty()) {
                    m_json_serializer.add_op(JsonSerializer::Op::AddNullValue);
                } else {
                    m_json_serializer.add_op(JsonSerializer::Op::AddNullField);
                    m_json_serializer.add_special_key(key);
                }
                break;
            }
            case NodeType::DateString:
            case NodeType::UnstructuredArray:
            case NodeType::Unknown:
                break;
        }
    }
    while (false == state.empty()) {
        for (size_t i = 0; i < state.top().repetitions; ++i) {
            m_json_serializer.add_op(state.top().operation);
        }
        state.pop();
    }
}

std::pair<size_t, int32_t> SchemaReader::generate_unordered_prefix(int32_t id, int32_t root_id) {
    int32_t cur_id = id;
    std::vector<int32_t> path_from_root;
    while (id != root_id) {
        auto node = m_local_schema_tree->get_node(id);
        // FIXME: this is probably incorrect, and this function probably doesn't need to be doing
        // so much back and forth in general -- it would be way easier to just build the path from
        // field to root in one go + track the types here
        if (node->get_key_name().empty() && node->get_parent_id() != root_id) {
            path_from_root.clear();
        }
        id = node->get_parent_id();
        path_from_root.push_back(id);
    }

    // FIXME
    // size_t num_closing_brackets = path_from_root.size();
    path_from_root.pop_back();
    size_t num_closing_brackets = path_from_root.size();
    int32_t new_parent_context = id;
    for (auto it = path_from_root.rbegin(); it != path_from_root.rend(); ++it) {
        new_parent_context = *it;
        auto node = m_local_schema_tree->get_node(*it);
        std::string const& key = node->get_key_name();
        switch (node->get_type()) {
            case NodeType::Object:
                if (key.empty()) {
                    // this branch shouldn't be reachable in the current implementation, but we
                    // leave it here for completeness
                    m_json_serializer.add_op(JsonSerializer::BeginDocument);
                } else {
                    m_json_serializer.add_op(JsonSerializer::Op::BeginObject);
                    m_json_serializer.add_special_key(node->get_key_name());
                }
                break;
            case NodeType::StructuredArray:
                if (key.empty()) {
                    m_json_serializer.add_op(JsonSerializer::BeginArrayDocument);
                } else {
                    m_json_serializer.add_op(JsonSerializer::Op::BeginArray);
                    m_json_serializer.add_special_key(node->get_key_name());
                }
            default:
                // FIXME: this should be unreachable, so we should throw
                return {0, 0};
        }
    }
    return {num_closing_brackets, new_parent_context};
}

void SchemaReader::generate_json_template(int32_t id) {
    auto node = m_local_schema_tree->get_node(id);
    auto children_ids = node->get_children_ids();

    for (int32_t child_id : children_ids) {
        int32_t child_global_id = m_local_id_to_global_id[child_id];
        auto child_node = m_local_schema_tree->get_node(child_id);
        std::string const& key = child_node->get_key_name();
        switch (child_node->get_type()) {
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
                auto node = m_local_schema_tree->get_node(id);
                m_json_serializer.add_op(JsonSerializer::Op::BeginArray);
                m_json_serializer.add_special_key(child_node->get_key_name());
                generate_structured_array_template(child_id);
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
