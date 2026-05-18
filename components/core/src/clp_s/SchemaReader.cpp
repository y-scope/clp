#include "SchemaReader.hpp"

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <span>
#include <stack>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

#include <ystdlib/error_handling/Result.hpp>

#include <clp_s/archive_constants.hpp>
#include <clp_s/BufferViewReader.hpp>
#include <clp_s/ErrorCode.hpp>
#include <clp_s/Schema.hpp>
#include <clp_s/SchemaTree.hpp>
#include <clp_s/search/Projection.hpp>
#include <clpp/Defs.hpp>
#include <clpp/ErrorCode.hpp>

namespace clp_s {
namespace {
/**
 * Finds the LogTypeID node in a schema and parses its dictionary ID.
 * @param schema
 * @param tree
 * @return A pair of (log_type_id, found).
 */
[[nodiscard]] auto
find_log_type_id_in_schema(std::span<SchemaNode::id_t> schema, SchemaTree const& tree)
        -> std::pair<clpp::log_shape_id_t, bool>;
/**
 * @param type
 * @return true if the given node type corresponds to a scalar column that consumes a column
 * reader.
 */
[[nodiscard]] auto node_type_consumes_column(NodeType type) -> bool;

[[nodiscard]] auto
find_log_type_id_in_schema(std::span<SchemaNode::id_t> schema, SchemaTree const& tree)
        -> std::pair<clpp::log_shape_id_t, bool> {
    for (auto global_column_id : schema) {
        if (Schema::schema_entry_is_unordered_object(global_column_id)) {
            continue;
        }
        auto const& node = tree.get_node(global_column_id);
        if (NodeType::LogTypeID == node.get_type()) {
            auto const& key_name = node.get_key_name();
            clpp::log_shape_id_t log_type_id{};
            auto [ptr, ec] = std::from_chars(
                    key_name.data(),
                    key_name.data() + key_name.size(),
                    log_type_id
            );
            if (std::errc() == ec && ptr == key_name.data() + key_name.size()) {
                return {log_type_id, true};
            }
            break;
        }
    }
    return {0, false};
}

[[nodiscard]] auto node_type_consumes_column(NodeType type) -> bool {
    switch (type) {
        case NodeType::LogTypeID:
        case NodeType::LogMessage:
        case NodeType::LogType:
        case NodeType::ParentRule: {
            return false;
        }
        default: {
            return true;
        }
    }
}
}  // namespace

void SchemaReader::append_column(BaseColumnReader* column_reader) {
    m_column_map[column_reader->get_id()] = column_reader;
    m_columns.push_back(column_reader);
}

void SchemaReader::append_unordered_column(BaseColumnReader* column_reader) {
    m_columns.push_back(column_reader);
}

void SchemaReader::mark_column_as_timestamp(BaseColumnReader* column_reader) {
    constexpr epochtime_t cNanosecondsInMillisecond{1000 * 1000LL};
    constexpr epochtime_t cMillisecondsInSecond{1000LL};
    m_timestamp_column = column_reader;
    if (m_timestamp_column->get_type() == NodeType::Timestamp) {
        m_get_timestamp = [this]() {
            return static_cast<TimestampColumnReader*>(m_timestamp_column)
                           ->get_encoded_time(m_cur_message)
                   / cNanosecondsInMillisecond;
        };
    } else if (m_timestamp_column->get_type() == NodeType::DeprecatedDateString) {
        m_get_timestamp = [this]() {
            return static_cast<DeprecatedDateStringColumnReader*>(m_timestamp_column)
                    ->get_encoded_time(m_cur_message);
            ;
        };
    } else if (m_timestamp_column->get_type() == NodeType::Integer) {
        m_get_timestamp = [this]() {
            return std::get<int64_t>(static_cast<Int64ColumnReader*>(m_timestamp_column)
                                             ->extract_value(m_cur_message));
        };
    } else if (m_timestamp_column->get_type() == NodeType::DeltaInteger) {
        m_get_timestamp = [this]() {
            return std::get<int64_t>(static_cast<DeltaEncodedInt64ColumnReader*>(m_timestamp_column)
                                             ->extract_value(m_cur_message));
        };
    } else if (m_timestamp_column->get_type() == NodeType::Float) {
        m_get_timestamp = [this]() {
            return static_cast<epochtime_t>(
                    std::get<double>(static_cast<FloatColumnReader*>(m_timestamp_column)
                                             ->extract_value(m_cur_message))
                    * cMillisecondsInSecond
            );
        };
    }
}

int64_t SchemaReader::get_next_log_event_idx() const {
    if (nullptr != m_log_event_idx_column) {
        return std::get<int64_t>(m_log_event_idx_column->extract_value(m_cur_message));
    }
    return 0;
}

void
SchemaReader::load(std::shared_ptr<char[]> stream_buffer, size_t offset, size_t uncompressed_size) {
    m_stream_buffer = stream_buffer;
    BufferViewReader buffer_reader{m_stream_buffer.get() + offset, uncompressed_size};
    for (auto& reader : m_columns) {
        reader->load(buffer_reader, m_num_messages);
    }
    if (buffer_reader.get_remaining_size() > 0) {
        throw OperationFailed(ErrorCodeCorrupt, __FILENAME__, __LINE__);
    }
}

auto SchemaReader::generate_json_string(uint64_t message_index) -> std::string {
    m_json_serializer.reset();
    m_json_serializer.begin_document();
    size_t column_id_index{0};
    size_t reconstruction_target_index{0};
    BaseColumnReader* column{nullptr};
    JsonSerializer::Op op{};
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
            case JsonSerializer::Op::BeginUnnamedObject: {
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
            case JsonSerializer::Op::BeginUnnamedArray: {
                m_json_serializer.begin_array_document();
                break;
            }
            case JsonSerializer::Op::AddIntField: {
                column = m_reordered_columns[column_id_index++];
                auto name = m_global_schema_tree->get_node(column->get_id()).get_key_name();
                m_json_serializer.append_key(name);
                m_json_serializer.append_value(
                        std::to_string(std::get<int64_t>(column->extract_value(message_index)))
                );
                break;
            }
            case JsonSerializer::Op::AddIntValue: {
                column = m_reordered_columns[column_id_index++];
                m_json_serializer.append_value(
                        std::to_string(std::get<int64_t>(column->extract_value(message_index)))
                );
                break;
            }
            case JsonSerializer::Op::AddFloatField: {
                column = m_reordered_columns[column_id_index++];
                auto name = m_global_schema_tree->get_node(column->get_id()).get_key_name();
                m_json_serializer.append_key(name);
                m_json_serializer.append_value(
                        std::to_string(std::get<double>(column->extract_value(message_index)))
                );
                break;
            }
            case JsonSerializer::Op::AddFloatValue: {
                column = m_reordered_columns[column_id_index++];
                m_json_serializer.append_value(
                        std::to_string(std::get<double>(column->extract_value(message_index)))
                );
                break;
            }
            case JsonSerializer::Op::AddFormattedFloatField: {
                column = m_reordered_columns[column_id_index++];
                auto name = m_global_schema_tree->get_node(column->get_id()).get_key_name();
                m_json_serializer.append_key(name);
                m_json_serializer.append_value_from_column(column, message_index);
                break;
            }
            case JsonSerializer::Op::AddFormattedFloatValue: {
                column = m_reordered_columns[column_id_index++];
                m_json_serializer.append_value_from_column(column, message_index);
                break;
            }
            case JsonSerializer::Op::AddBoolField: {
                column = m_reordered_columns[column_id_index++];
                auto name = m_global_schema_tree->get_node(column->get_id()).get_key_name();
                m_json_serializer.append_key(name);
                m_json_serializer.append_value(
                        std::get<uint8_t>(column->extract_value(message_index)) != 0 ? "true"
                                                                                     : "false"
                );
                break;
            }
            case JsonSerializer::Op::AddBoolValue: {
                column = m_reordered_columns[column_id_index++];
                m_json_serializer.append_value(
                        std::get<uint8_t>(column->extract_value(message_index)) != 0 ? "true"
                                                                                     : "false"
                );
                break;
            }
            case JsonSerializer::Op::AddStringField: {
                column = m_reordered_columns[column_id_index++];
                auto name = m_global_schema_tree->get_node(column->get_id()).get_key_name();
                m_json_serializer.append_key(name);
                m_json_serializer.append_value_from_column_with_quotes(column, message_index);
                break;
            }
            case JsonSerializer::Op::AddStringValue: {
                column = m_reordered_columns[column_id_index++];
                m_json_serializer.append_value_from_column_with_quotes(column, message_index);
                break;
            }
            case JsonSerializer::Op::AddArrayField: {
                column = m_reordered_columns[column_id_index++];
                m_json_serializer.append_key(
                        m_global_schema_tree->get_node(column->get_id()).get_key_name()
                );
                m_json_serializer.append_value_from_column(column, message_index);
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
            case JsonSerializer::Op::AddConstantStringField: {
                m_json_serializer.append_constant_string_field();
                break;
            }
            case JsonSerializer::Op::AddReconstructedLogShapeField: {
                auto const& [log_msg_id, parent_rule_fqn]{
                        m_reconstruction_targets[reconstruction_target_index]
                };
                ++reconstruction_target_index;
                m_json_serializer.append_key();
                m_json_serializer.append_quoted_value(
                        reconstruct_log_shape(log_msg_id, parent_rule_fqn, message_index)
                );
                break;
            }
            case JsonSerializer::Op::AddLiteralField: {
                column = m_reordered_columns[column_id_index++];
                auto const key{m_global_schema_tree->get_node(column->get_id()).get_key_name()};
                m_json_serializer.append_key(key);
                m_json_serializer.append_value_from_column(column, message_index);
                break;
            }
        }
    }

    m_json_serializer.end_document();
    return m_json_serializer.get_serialized_string();
}

bool SchemaReader::get_next_message(std::string& message) {
    if (m_cur_message >= m_num_messages) {
        return false;
    }

    if (false == m_serializer_initialized) {
        initialize_serializer();
    }
    message = generate_json_string(m_cur_message);

    if (message.back() != '\n') {
        message += '\n';
    }

    ++m_cur_message;
    return true;
}

bool SchemaReader::get_next_message(std::string& message, FilterClass& filter) {
    while (m_cur_message < m_num_messages && false == filter.filter(m_cur_message)) {
        ++m_cur_message;
    }

    if (m_cur_message >= m_num_messages) {
        return false;
    }

    if (m_should_marshal_records) {
        if (false == m_serializer_initialized) {
            initialize_serializer();
        }
        message = generate_json_string(m_cur_message);

        if (message.back() != '\n') {
            message += '\n';
        }
    }

    ++m_cur_message;
    return true;
}

bool SchemaReader::get_next_message_with_metadata(
        std::string& message,
        epochtime_t& timestamp,
        int64_t& log_event_idx
) {
    if (m_cur_message >= m_num_messages) {
        return false;
    }

    if (m_should_marshal_records) {
        if (false == m_serializer_initialized) {
            initialize_serializer();
        }
        message = generate_json_string(m_cur_message);

        if (message.back() != '\n') {
            message += '\n';
        }
    }

    timestamp = m_get_timestamp();
    log_event_idx = get_next_log_event_idx();

    ++m_cur_message;
    return true;
}

bool SchemaReader::get_next_message_with_metadata(
        std::string& message,
        epochtime_t& timestamp,
        int64_t& log_event_idx,
        FilterClass& filter
) {
    // TODO: If we already get max_num_results messages, we can skip messages
    // with the timestamp less than the smallest timestamp in the priority queue
    while (m_cur_message < m_num_messages && false == filter.filter(m_cur_message)) {
        ++m_cur_message;
    }

    if (m_cur_message >= m_num_messages) {
        return false;
    }

    if (m_should_marshal_records) {
        if (false == m_serializer_initialized) {
            initialize_serializer();
        }
        message = generate_json_string(m_cur_message);

        if (message.back() != '\n') {
            message += '\n';
        }
    }

    timestamp = m_get_timestamp();
    log_event_idx = get_next_log_event_idx();

    ++m_cur_message;
    return true;
}

void SchemaReader::initialize_filter(FilterClass& filter) {
    filter.init(this, m_columns);
}

void SchemaReader::initialize_filter_with_column_map(FilterClass& filter) {
    filter.init(this, m_column_map);
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

auto SchemaReader::is_node_projected(SchemaNode::id_t node_id) -> bool {
    if (nullptr == m_projection) {
        return false;
    }
    SchemaNode::id_t cur_id{node_id};
    while (-1 != cur_id) {
        auto const& node{m_global_schema_tree->get_node(cur_id)};
        if (m_projection->matches_node(cur_id)) {
            if (m_projection->is_projected_as(cur_id, search::Projection::NodeProjection::Shape)) {
                return node_id == cur_id;
            }
            return true;
        }
        if (NodeType::LogMessage == node.get_type()) {
            break;
        }
        cur_id = node.get_parent_id();
    }
    return false;
}

void SchemaReader::find_intersection_and_fix_brackets(
        int32_t cur_root,
        int32_t next_root,
        std::vector<int32_t>& path_to_intersection
) {
    auto const* cur_node = &m_global_schema_tree->get_node(cur_root);
    auto const* next_node = &m_global_schema_tree->get_node(next_root);
    while (cur_node->get_parent_id() != next_node->get_parent_id()) {
        if (cur_node->get_depth() > next_node->get_depth()) {
            cur_root = cur_node->get_parent_id();
            cur_node = &m_global_schema_tree->get_node(cur_root);
            m_json_serializer.add_op(JsonSerializer::Op::EndObject);
        } else if (cur_node->get_depth() < next_node->get_depth()) {
            path_to_intersection.push_back(next_root);
            next_root = next_node->get_parent_id();
            next_node = &m_global_schema_tree->get_node(next_root);
        } else {
            cur_root = cur_node->get_parent_id();
            cur_node = &m_global_schema_tree->get_node(cur_root);
            m_json_serializer.add_op(JsonSerializer::Op::EndObject);
            path_to_intersection.push_back(next_root);
            next_root = next_node->get_parent_id();
            next_node = &m_global_schema_tree->get_node(next_root);
        }
    }

    // The loop above ends when the parent of next node and cur node matches. When these two nodes
    // have the same parent but are different nodes we need to close the last bracket for the
    // previous node, and add the first key for next node.
    if (cur_node != next_node) {
        m_json_serializer.add_op(JsonSerializer::Op::EndObject);
        path_to_intersection.push_back(next_node->get_id());
    }

    for (auto it = path_to_intersection.rbegin(); it != path_to_intersection.rend(); ++it) {
        auto const& node = m_global_schema_tree->get_node(*it);
        bool no_name = true;
        if (false == node.get_key_name().empty()) {
            m_json_serializer.add_special_key(node.get_key_name());
            no_name = false;
        }
        if (NodeType::Object == node.get_type()) {
            m_json_serializer.add_op(
                    no_name ? JsonSerializer::Op::BeginUnnamedObject
                            : JsonSerializer::Op::BeginObject
            );
        } else if (NodeType::StructuredArray == node.get_type()) {
            m_json_serializer.add_op(
                    no_name ? JsonSerializer::Op::BeginUnnamedArray : JsonSerializer::Op::BeginArray
            );
        }
    }
    path_to_intersection.clear();
}

size_t SchemaReader::generate_structured_array_template(
        int32_t array_root,
        size_t column_start,
        std::span<int32_t> schema
) {
    size_t column_idx = column_start;
    std::vector<int32_t> path_to_intersection;
    int32_t depth = m_global_schema_tree->get_node(array_root).get_depth();

    for (size_t i = 0; i < schema.size(); ++i) {
        int32_t global_column_id = schema[i];
        if (Schema::schema_entry_is_unordered_object(global_column_id)) {
            auto type = Schema::get_unordered_object_type(global_column_id);
            size_t length = Schema::get_unordered_object_length(global_column_id);
            auto sub_object_schema = schema.subspan(i + 1, length);
            if (NodeType::StructuredArray == type) {
                int32_t sub_array_root
                        = m_global_schema_tree->find_matching_subtree_root_in_subtree(
                                array_root,
                                get_first_column_in_span(sub_object_schema),
                                NodeType::StructuredArray
                        );
                m_json_serializer.add_op(JsonSerializer::Op::BeginUnnamedArray);
                column_idx = generate_structured_array_template(
                        sub_array_root,
                        column_idx,
                        sub_object_schema
                );
                m_json_serializer.add_op(JsonSerializer::Op::EndArray);
            } else if (NodeType::Object == type) {
                int32_t object_root = m_global_schema_tree->find_matching_subtree_root_in_subtree(
                        array_root,
                        get_first_column_in_span(sub_object_schema),
                        NodeType::Object
                );
                m_json_serializer.add_op(JsonSerializer::Op::BeginUnnamedObject);
                column_idx = generate_structured_object_template(
                        object_root,
                        column_idx,
                        sub_object_schema
                );
                m_json_serializer.add_op(JsonSerializer::Op::EndObject);
            }
            i += length;
        } else {
            auto const& node = m_global_schema_tree->get_node(global_column_id);
            switch (node.get_type()) {
                case NodeType::Object: {
                    find_intersection_and_fix_brackets(
                            array_root,
                            global_column_id,
                            path_to_intersection
                    );
                    for (int j = 0; j < (node.get_depth() - depth); ++j) {
                        m_json_serializer.add_op(JsonSerializer::Op::EndObject);
                    }
                    break;
                }
                case NodeType::StructuredArray: {
                    m_json_serializer.add_op(JsonSerializer::Op::BeginUnnamedArray);
                    m_json_serializer.add_op(JsonSerializer::Op::EndArray);
                    break;
                }
                case NodeType::DeltaInteger:
                case NodeType::Integer: {
                    m_json_serializer.add_op(JsonSerializer::Op::AddIntValue);
                    m_reordered_columns.push_back(m_columns[column_idx++]);
                    break;
                }
                case NodeType::Float: {
                    m_json_serializer.add_op(JsonSerializer::Op::AddFloatValue);
                    m_reordered_columns.push_back(m_columns[column_idx++]);
                    break;
                }
                case NodeType::FormattedFloat:
                case NodeType::DictionaryFloat: {
                    m_json_serializer.add_op(JsonSerializer::Op::AddFormattedFloatValue);
                    m_reordered_columns.push_back(m_columns[column_idx++]);
                    break;
                }
                case NodeType::Boolean: {
                    m_json_serializer.add_op(JsonSerializer::Op::AddBoolValue);
                    m_reordered_columns.push_back(m_columns[column_idx++]);
                    break;
                }
                case NodeType::ClpString:
                case NodeType::VarString: {
                    m_json_serializer.add_op(JsonSerializer::Op::AddStringValue);
                    m_reordered_columns.push_back(m_columns[column_idx++]);
                    break;
                }
                case NodeType::NullValue: {
                    m_json_serializer.add_op(JsonSerializer::Op::AddNullValue);
                    break;
                }
                case NodeType::DeprecatedDateString:
                case NodeType::UnstructuredArray:
                case NodeType::Metadata:
                case NodeType::LogMessage:
                case NodeType::LogType:
                case NodeType::LogTypeID:
                case NodeType::ParentRule:
                case NodeType::Timestamp:
                case NodeType::Unknown:
                    break;
            }
        }
    }
    return column_idx;
}

size_t SchemaReader::generate_structured_object_template(
        int32_t object_root,
        size_t column_start,
        std::span<int32_t> schema
) {
    int32_t root = object_root;
    size_t column_idx = column_start;
    std::vector<int32_t> path_to_intersection;

    for (size_t i = 0; i < schema.size(); ++i) {
        int32_t global_column_id = schema[i];
        if (Schema::schema_entry_is_unordered_object(global_column_id)) {
            // It should only be possible to encounter arrays inside of structured objects
            size_t array_length = Schema::get_unordered_object_length(global_column_id);
            auto array_schema = schema.subspan(i + 1, array_length);
            // we can guarantee that the last array we hit on the path to object root must be the
            // right one because otherwise we'd be inside the structured array generator
            int32_t array_root = m_global_schema_tree->find_matching_subtree_root_in_subtree(
                    object_root,
                    get_first_column_in_span(array_schema),
                    NodeType::StructuredArray
            );

            find_intersection_and_fix_brackets(root, array_root, path_to_intersection);
            column_idx = generate_structured_array_template(array_root, column_idx, array_schema);
            m_json_serializer.add_op(JsonSerializer::Op::EndArray);
            i += array_length;
            // root is parent of the array object since we close the array bracket above
            auto const& node = m_global_schema_tree->get_node(array_root);
            root = node.get_parent_id();
        } else {
            auto const& node = m_global_schema_tree->get_node(global_column_id);
            int32_t next_root = node.get_parent_id();
            find_intersection_and_fix_brackets(root, next_root, path_to_intersection);
            root = next_root;
            switch (node.get_type()) {
                case NodeType::Object: {
                    m_json_serializer.add_op(JsonSerializer::Op::BeginObject);
                    m_json_serializer.add_special_key(node.get_key_name());
                    m_json_serializer.add_op(JsonSerializer::Op::EndObject);
                    break;
                }
                case NodeType::StructuredArray: {
                    m_json_serializer.add_op(JsonSerializer::Op::BeginArray);
                    m_json_serializer.add_special_key(node.get_key_name());
                    m_json_serializer.add_op(JsonSerializer::Op::EndArray);
                    break;
                }
                case NodeType::DeltaInteger:
                case NodeType::Integer: {
                    m_json_serializer.add_op(JsonSerializer::Op::AddIntField);
                    m_reordered_columns.push_back(m_columns[column_idx++]);
                    break;
                }
                case NodeType::Float: {
                    m_json_serializer.add_op(JsonSerializer::Op::AddFloatField);
                    m_reordered_columns.push_back(m_columns[column_idx++]);
                    break;
                }
                case NodeType::FormattedFloat:
                case NodeType::DictionaryFloat: {
                    m_json_serializer.add_op(JsonSerializer::Op::AddFormattedFloatField);
                    m_reordered_columns.push_back(m_columns[column_idx++]);
                    break;
                }
                case NodeType::Boolean: {
                    m_json_serializer.add_op(JsonSerializer::Op::AddBoolField);
                    m_reordered_columns.push_back(m_columns[column_idx++]);
                    break;
                }
                case NodeType::ClpString:
                case NodeType::VarString: {
                    m_json_serializer.add_op(JsonSerializer::Op::AddStringField);
                    m_reordered_columns.push_back(m_columns[column_idx++]);
                    break;
                }
                case NodeType::NullValue: {
                    m_json_serializer.add_op(JsonSerializer::Op::AddNullField);
                    m_json_serializer.add_special_key(node.get_key_name());
                    break;
                }
                case NodeType::DeprecatedDateString:
                case NodeType::UnstructuredArray:
                case NodeType::Metadata:
                case NodeType::LogMessage:
                case NodeType::LogType:
                case NodeType::LogTypeID:
                case NodeType::ParentRule:
                case NodeType::Timestamp:
                case NodeType::Unknown:
                    break;
            }
        }
    }
    find_intersection_and_fix_brackets(root, object_root, path_to_intersection);
    return column_idx;
}

void SchemaReader::initialize_serializer() {
    if (m_serializer_initialized) {
        return;
    }

    m_serializer_initialized = true;

    for (int32_t global_column_id : m_ordered_schema) {
        if (m_projection->matches_node(global_column_id)) {
            generate_local_tree(global_column_id);
        }
    }

    for (auto const& entry : m_global_id_to_unordered_object) {
        auto const root_id{entry.first};
        auto const& root_node{m_global_schema_tree->get_node(root_id)};
        if (NodeType::LogMessage == root_node.get_type()) {
            auto const& schema{entry.second.second};
            bool need_log_message{false};
            if (m_projection->matches_node(root_id)) {
                generate_local_tree(root_id);
                need_log_message = true;
            }
            for (auto const child_id : schema) {
                if (Schema::schema_entry_is_unordered_object(child_id)) {
                    continue;
                }
                if (is_node_projected(child_id)) {
                    generate_local_tree(child_id);
                    need_log_message = true;
                }
            }
            if (need_log_message && false == m_projection->matches_node(root_id)) {
                generate_local_tree(root_id);
            }
        } else if (m_projection->matches_node(root_id)) {
            generate_local_tree(root_id);
        }
    }

    // TODO: this code will have to change once we allow mixing log lines parsed by different
    // parsers and if we add support for serializing auto-generated keys in regular JSON.
    if (auto subtree_root = m_local_schema_tree.get_object_subtree_node_id_for_namespace(
                constants::cDefaultNamespace
        );
        -1 != subtree_root)
    {
        generate_json_template(subtree_root);
    }
}

void SchemaReader::generate_json_template(int32_t id) {
    auto const& node = m_local_schema_tree.get_node(id);
    auto const& children_ids = node.get_children_ids();

    for (int32_t child_id : children_ids) {
        int32_t child_global_id = m_local_id_to_global_id[child_id];
        auto const& child_node = m_local_schema_tree.get_node(child_id);
        auto key = child_node.get_key_name();
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
                m_json_serializer.add_op(JsonSerializer::Op::BeginArray);
                m_json_serializer.add_special_key(key);
                auto structured_it = m_global_id_to_unordered_object.find(child_global_id);
                if (m_global_id_to_unordered_object.end() != structured_it) {
                    size_t column_start = structured_it->second.first;
                    std::span<int32_t> structured_schema = structured_it->second.second;
                    generate_structured_array_template(
                            child_global_id,
                            column_start,
                            structured_schema
                    );
                }
                m_json_serializer.add_op(JsonSerializer::Op::EndArray);
                break;
            }
            case NodeType::LogMessage: {
                if (auto const result{generate_log_message_template(child_global_id)};
                    result.has_error())
                {
                    throw(std::runtime_error(
                            "generate_log_message_template failed with: " + result.error().message()
                    ));
                }
                break;
            }
            case NodeType::DeltaInteger:
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
            case NodeType::FormattedFloat:
            case NodeType::DictionaryFloat: {
                m_json_serializer.add_op(JsonSerializer::Op::AddFormattedFloatField);
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
            case NodeType::DeprecatedDateString: {
                m_json_serializer.add_op(JsonSerializer::Op::AddStringField);
                m_reordered_columns.push_back(m_column_map[child_global_id]);
                break;
            }
            case NodeType::Timestamp: {
                m_json_serializer.add_op(JsonSerializer::Op::AddLiteralField);
                m_reordered_columns.emplace_back(m_column_map.at(child_global_id));
                break;
            }
            case NodeType::NullValue: {
                m_json_serializer.add_op(JsonSerializer::Op::AddNullField);
                m_json_serializer.add_special_key(key);
                break;
            }
            case NodeType::Metadata:
            case NodeType::LogType:
            case NodeType::LogTypeID:
            case NodeType::ParentRule:
            case NodeType::Unknown: {
                break;
            }
        }
    }
}

auto SchemaReader::compute_log_message_modes(SchemaNode::id_t log_msg_node_id) -> LogMessageModes {
    LogMessageModes modes;
    modes.has_default = m_projection
                        && m_projection->is_projected_as(
                                log_msg_node_id,
                                search::Projection::NodeProjection::Default
                        );
    modes.has_decomposed = m_projection
                           && m_projection->is_projected_as(
                                   log_msg_node_id,
                                   search::Projection::NodeProjection::Decomposed
                           );
    modes.has_shape = m_projection
                      && m_projection->is_projected_as(
                              log_msg_node_id,
                              search::Projection::NodeProjection::Shape
                      );
    modes.is_return_all = m_projection && m_projection->matches_node(log_msg_node_id)
                          && false == modes.has_default && false == modes.has_decomposed
                          && false == modes.has_shape;
    modes.effective_has_default = modes.has_default || modes.is_return_all;
    return modes;
}

auto SchemaReader::scan_child_projection_modes(
        std::span<SchemaNode::id_t> schema,
        SchemaNode::id_t log_msg_node_id
) -> ChildProjectionModes {
    ChildProjectionModes modes;
    for (auto global_column_id : schema) {
        if (Schema::schema_entry_is_unordered_object(global_column_id)) {
            continue;
        }
        auto const& node{m_global_schema_tree->get_node(global_column_id)};
        bool child_would_emit{false};
        if (m_projection) {
            if (search::Projection::Mode::ReturnSelectedColumns
                        == m_projection->get_projection_mode()
                && m_projection->matches_node(global_column_id))
            {
                child_would_emit = true;
            }
            if (NodeType::ParentRule == node.get_type()) {
                if (m_projection->has_any_projection(global_column_id)) {
                    child_would_emit = true;
                }
            }
            if (NodeType::LogTypeID == node.get_type()) {
                if (m_projection->is_projected_as(
                            log_msg_node_id,
                            search::Projection::NodeProjection::Decomposed
                    )
                    || m_projection->is_projected_as(
                            log_msg_node_id,
                            search::Projection::NodeProjection::Shape
                    ))
                {
                    child_would_emit = true;
                }
            }
        }
        if (child_would_emit) {
            modes.any_child_projected = true;
        }
        if (NodeType::ParentRule == node.get_type()) {
            if (m_projection
                && m_projection->is_projected_as(
                        global_column_id,
                        search::Projection::NodeProjection::Decomposed
                ))
            {
                modes.any_child_decomposed = true;
            }
            if (m_projection
                && m_projection->is_projected_as(
                        global_column_id,
                        search::Projection::NodeProjection::Shape
                ))
            {
                modes.any_child_shape = true;
            }
        }
    }
    return modes;
}

void SchemaReader::emit_parent_rule_shape(
        SchemaNode::id_t log_msg_node_id,
        SchemaNode::id_t global_column_id,
        clpp::log_shape_id_t log_type_id,
        bool found_log_type_id,
        absl::flat_hash_set<SchemaNode::id_t>& emitted_schema_parent_rules
) {
    if (emitted_schema_parent_rules.contains(global_column_id)) {
        return;
    }
    emitted_schema_parent_rules.insert(global_column_id);

    auto const& node{m_global_schema_tree->get_node(global_column_id)};

    bool const pr_has_default{
            m_projection
            && m_projection->is_projected_as(
                    global_column_id,
                    search::Projection::NodeProjection::Default
            )
    };
    bool const pr_has_decomposed{
            m_projection
            && m_projection->is_projected_as(
                    global_column_id,
                    search::Projection::NodeProjection::Decomposed
            )
    };
    bool const pr_has_shape{
            m_projection
            && m_projection->is_projected_as(
                    global_column_id,
                    search::Projection::NodeProjection::Shape
            )
    };

    if (pr_has_default) {
        auto const parent_fqn{m_global_schema_tree->build_qualified_name(global_column_id)};
        m_json_serializer.add_special_key(node.get_key_name());
        m_json_serializer.add_op(JsonSerializer::Op::AddReconstructedLogShapeField);
        m_reconstruction_targets.emplace_back(log_msg_node_id, parent_fqn);
    }
    if (pr_has_decomposed || pr_has_shape) {
        if (found_log_type_id && nullptr != m_typed_log_dict && nullptr != m_logtype_metadata
            && static_cast<logtype_id_t>(log_type_id) < m_logtype_metadata->size())
        {
            auto const& metadata{m_logtype_metadata->at(static_cast<logtype_id_t>(log_type_id))};
            auto const parent_fqn{m_global_schema_tree->build_qualified_name(global_column_id)};
            for (auto const& match : metadata.get_parent_matches()) {
                if (match.m_name == parent_fqn) {
                    auto const& log_type_template{m_typed_log_dict->get_value(log_type_id)};
                    if (match.m_start < log_type_template.size()
                        && match.m_start + match.m_size <= log_type_template.size())
                    {
                        m_json_serializer.add_constant_string_field(
                                std::string(node.get_key_name()) + std::string(clpp::cShapeSuffix),
                                log_type_template.substr(match.m_start, match.m_size)
                        );
                    }
                    break;
                }
            }
        }
    }
}

auto SchemaReader::emit_log_type_shape(clpp::log_shape_id_t log_type_id)
        -> ystdlib::error_handling::Result<void> {
    if (nullptr == m_typed_log_dict) {
        return clpp::ClppErrorCode{clpp::ClppErrorCodeEnum::Failure};
    }
    auto const& log_type_str{m_typed_log_dict->get_value(log_type_id)};
    m_json_serializer.add_constant_string_field(clpp::cShapeSuffix, log_type_str);
    return ystdlib::error_handling::success();
}

auto SchemaReader::collect_leaf_entries(
        std::span<SchemaNode::id_t> schema,
        SchemaNode::id_t log_msg_node_id,
        size_t& column_idx
) -> std::vector<LeafEntry> {
    std::vector<LeafEntry> entries;
    for (auto global_column_id : schema) {
        if (Schema::schema_entry_is_unordered_object(global_column_id)) {
            continue;
        }

        auto const& node{m_global_schema_tree->get_node(global_column_id)};

        if (NodeType::ParentRule == node.get_type() || NodeType::LogTypeID == node.get_type()) {
            continue;
        }

        bool const consumes_column{node_type_consumes_column(node.get_type())};

        bool should_emit{false};
        if (m_projection
            && search::Projection::Mode::ReturnSelectedColumns
                       == m_projection->get_projection_mode()
            && m_projection->matches_node(global_column_id))
        {
            should_emit = true;
        }
        SchemaNode::id_t ancestor_id{node.get_parent_id()};
        while (-1 != ancestor_id && ancestor_id != log_msg_node_id) {
            auto const& ancestor{m_global_schema_tree->get_node(ancestor_id)};
            if (NodeType::ParentRule == ancestor.get_type() && m_projection
                && m_projection->is_projected_as(
                        ancestor_id,
                        search::Projection::NodeProjection::Decomposed
                ))
            {
                should_emit = true;
                break;
            }
            ancestor_id = ancestor.get_parent_id();
        }
        if (m_projection
            && m_projection->is_projected_as(
                    log_msg_node_id,
                    search::Projection::NodeProjection::Decomposed
            ))
        {
            should_emit = true;
        }

        if (false == should_emit) {
            if (consumes_column) {
                ++column_idx;
            }
            continue;
        }

        if (consumes_column) {
            entries.push_back(
                    LeafEntry{
                            global_column_id,
                            column_idx,
                            m_global_schema_tree->build_qualified_name(global_column_id),
                            node.get_type()
                    }
            );
            ++column_idx;
        }
    }
    return entries;
}

auto SchemaReader::emit_grouped_leaf_entries(std::vector<LeafEntry>& entries)
        -> ystdlib::error_handling::Result<void> {
    std::stable_sort(entries.begin(), entries.end(), [](auto const& a, auto const& b) -> bool {
        return a.fqn < b.fqn;
    });

    for (size_t i{0}; i < entries.size();) {
        auto const& fqn{entries[i].fqn};
        m_json_serializer.add_special_key(fqn);
        m_json_serializer.add_op(JsonSerializer::Op::BeginArray);

        size_t j{i};
        while (j < entries.size() && entries[j].fqn == fqn) {
            switch (entries[j].type) {
                case NodeType::DeltaInteger:
                case NodeType::Integer: {
                    m_json_serializer.add_op(JsonSerializer::Op::AddIntValue);
                    break;
                }
                case NodeType::Float: {
                    m_json_serializer.add_op(JsonSerializer::Op::AddFloatValue);
                    break;
                }
                case NodeType::FormattedFloat:
                case NodeType::DictionaryFloat: {
                    m_json_serializer.add_op(JsonSerializer::Op::AddFormattedFloatValue);
                    break;
                }
                case NodeType::Boolean: {
                    m_json_serializer.add_op(JsonSerializer::Op::AddBoolValue);
                    break;
                }
                case NodeType::ClpString:
                case NodeType::VarString: {
                    m_json_serializer.add_op(JsonSerializer::Op::AddStringValue);
                    break;
                }
                default: {
                    return clpp::ClppErrorCode{clpp::ClppErrorCodeEnum::Unsupported};
                }
            }
            m_reordered_columns.push_back(m_columns[entries[j].column_idx]);
            ++j;
        }

        m_json_serializer.add_op(JsonSerializer::Op::EndArray);
        i = j;
    }
    return ystdlib::error_handling::success();
}

auto SchemaReader::generate_log_message_template(SchemaNode::id_t log_msg_node_id)
        -> ystdlib::error_handling::Result<size_t> {
    auto log_msg_it{m_global_id_to_unordered_object.find(log_msg_node_id)};
    if (m_global_id_to_unordered_object.end() == log_msg_it) {
        return clpp::ClppErrorCode{clpp::ClppErrorCodeEnum::Failure};
    }

    auto column_idx{log_msg_it->second.first};
    auto const schema{log_msg_it->second.second};

    auto const [log_type_id, found_log_type_id]{
            find_log_type_id_in_schema(schema, *m_global_schema_tree)
    };

    auto const modes{compute_log_message_modes(log_msg_node_id)};
    auto const key_name{m_global_schema_tree->get_node(log_msg_node_id).get_key_name()};

    if (modes.effective_has_default) {
        m_json_serializer.add_special_key(key_name);
        m_json_serializer.add_op(JsonSerializer::Op::AddReconstructedLogShapeField);
        m_reconstruction_targets.emplace_back(log_msg_node_id, "");
    }

    auto const child_modes{scan_child_projection_modes(schema, log_msg_node_id)};
    bool const has_any_decomposed{modes.has_decomposed || child_modes.any_child_decomposed};
    bool const has_any_shape{modes.has_shape || child_modes.any_child_shape};

    if (modes.effective_has_default && false == has_any_decomposed && false == has_any_shape
        && false == child_modes.any_child_projected)
    {
        for (auto global_column_id : schema) {
            if (Schema::schema_entry_is_unordered_object(global_column_id)) {
                continue;
            }
            auto const& node{m_global_schema_tree->get_node(global_column_id)};
            if (node_type_consumes_column(node.get_type())) {
                ++column_idx;
            }
        }
        return column_idx;
    }

    if (false == modes.effective_has_default && false == has_any_decomposed
        && false == has_any_shape && false == child_modes.any_child_projected)
    {
        return column_idx;
    }
    auto const object_key{
            modes.effective_has_default
                    ? std::string(key_name)
                              + std::string(
                                      has_any_decomposed ? clpp::cDecomposedSuffix
                                                         : clpp::cShapeSuffix
                              )
                    : std::string(key_name)
    };

    m_json_serializer.add_op(JsonSerializer::Op::BeginObject);
    m_json_serializer.add_special_key(object_key);

    absl::flat_hash_set<SchemaNode::id_t> emitted_schema_parent_rules;

    for (auto global_column_id : schema) {
        if (Schema::schema_entry_is_unordered_object(global_column_id)) {
            continue;
        }

        auto const& node{m_global_schema_tree->get_node(global_column_id)};

        if (NodeType::ParentRule == node.get_type()) {
            emit_parent_rule_shape(
                    log_msg_node_id,
                    global_column_id,
                    log_type_id,
                    found_log_type_id,
                    emitted_schema_parent_rules
            );
            continue;
        }

        if (NodeType::LogTypeID == node.get_type()) {
            if (m_projection
                && (m_projection->is_projected_as(
                            log_msg_node_id,
                            search::Projection::NodeProjection::Shape
                    )
                    || m_projection->is_projected_as(
                            log_msg_node_id,
                            search::Projection::NodeProjection::Decomposed
                    )))
            {
                if (auto const result{emit_log_type_shape(log_type_id)}; result.has_error()) {
                    return result.error();
                }
            }
            continue;
        }
    }

    auto entries{collect_leaf_entries(schema, log_msg_node_id, column_idx)};
    if (auto const result{emit_grouped_leaf_entries(entries)}; result.has_error()) {
        return result.error();
    }

    m_json_serializer.add_op(JsonSerializer::Op::EndObject);
    return column_idx;
}

auto SchemaReader::reconstruct_log_shape(
        SchemaNode::id_t log_msg_node_id,
        std::string_view parent_rule_fqn,
        uint64_t message_index
) -> std::string {
    auto log_msg_it{m_global_id_to_unordered_object.find(log_msg_node_id)};
    if (m_global_id_to_unordered_object.end() == log_msg_it) {
        return {};
    }

    auto const& schema{log_msg_it->second.second};
    auto const [log_type_id, found_log_type_id]{
            find_log_type_id_in_schema(schema, *m_global_schema_tree)
    };

    if (false == found_log_type_id || nullptr == m_typed_log_dict) {
        return {};
    }

    auto const log_type_template{m_typed_log_dict->get_value(log_type_id)};
    if (log_type_template.empty()) {
        return {};
    }

    std::string_view template_to_scan{log_type_template};
    if (false == parent_rule_fqn.empty()) {
        if (nullptr == m_logtype_metadata) {
            return {};
        }
        auto const& metadata{m_logtype_metadata->at(static_cast<logtype_id_t>(log_type_id))};
        bool found_match{false};
        for (auto const& match : metadata.get_parent_matches()) {
            if (match.m_name == parent_rule_fqn) {
                if (match.m_start < log_type_template.size()
                    && match.m_start + match.m_size <= log_type_template.size())
                {
                    template_to_scan = std::string_view{
                            log_type_template.data() + match.m_start,
                            match.m_size
                    };
                }
                found_match = true;
                break;
            }
        }
        if (false == found_match) {
            return {};
        }
    }

    size_t column_idx{log_msg_it->second.first};
    std::unordered_map<std::string, std::vector<std::string>> fqn_to_values;
    for (auto global_column_id : schema) {
        if (Schema::schema_entry_is_unordered_object(global_column_id)) {
            continue;
        }
        auto const& node{m_global_schema_tree->get_node(global_column_id)};
        if (NodeType::LogTypeID == node.get_type()
            || false == node_type_consumes_column(node.get_type()))
        {
            continue;
        }
        auto fqn{m_global_schema_tree->build_qualified_name(global_column_id)};
        auto* column{m_columns[column_idx]};
        std::string value;
        column->extract_string_value_into_buffer(message_index, value);
        fqn_to_values[fqn].push_back(std::move(value));
        ++column_idx;
    }

    std::unordered_map<std::string, size_t> fqn_to_next_index;
    std::string raw_text;
    size_t pos{0};
    while (pos < template_to_scan.size()) {
        auto pct{template_to_scan.find('%', pos)};
        if (std::string_view::npos == pct) {
            raw_text.append(template_to_scan.substr(pos));
            break;
        }
        raw_text.append(template_to_scan.substr(pos, pct - pos));
        auto end_pct{template_to_scan.find('%', pct + 1)};
        if (std::string_view::npos == end_pct) {
            raw_text.append(template_to_scan.substr(pct));
            break;
        }
        auto const fqn{std::string(template_to_scan.substr(pct + 1, end_pct - pct - 1))};
        auto it{fqn_to_values.find(fqn)};
        if (fqn_to_values.end() != it) {
            auto& next_index{fqn_to_next_index[fqn]};
            if (next_index < it->second.size()) {
                raw_text.append(it->second[next_index++]);
            } else {
                raw_text.append("%").append(fqn).append("%");
            }
        } else {
            raw_text.append("%").append(fqn).append("%");
        }
        pos = end_pct + 1;
    }
    return raw_text;
}
}  // namespace clp_s
