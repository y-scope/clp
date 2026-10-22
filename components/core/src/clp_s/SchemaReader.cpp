#include "SchemaReader.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <stack>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include <ystdlib/error_handling/Result.hpp>

#include <clp_s/archive_constants.hpp>
#include <clp_s/BufferViewReader.hpp>
#include <clp_s/Defs.hpp>
#include <clp_s/ErrorCode.hpp>
#include <clp_s/Schema.hpp>
#include <clp_s/SchemaTree.hpp>
#include <clp_s/search/Projection.hpp>
#include <clpp/Defs.hpp>
#include <clpp/ErrorCode.hpp>
#include <clpp/TextShape.hpp>

namespace clp_s {
namespace {
constexpr std::string_view cShapeKey{"shape"};
constexpr std::string_view cTextKey{"text"};

/**
 * Checks whether a sub-schema contains a projected node, i.e. a leaf entry or a nested
 * StructurizedText/ParentRule (at any depth) that the projection matches.
 * @param schema The sub-schema to scan.
 * @param projection
 * @return Whether any node in the sub-schema is projected.
 */
[[nodiscard]] auto contains_projected_node(SchemaView schema, search::Projection const& projection)
        -> bool;

[[nodiscard]] auto contains_projected_node(SchemaView schema, search::Projection const& projection)
        -> bool {
    return schema.any_node_id<true>([&](mpt_node_id_t node_id) -> bool {
        return projection.matches_node(node_id);
    });
}
}  // namespace

void SchemaReader::append_column(BaseColumnReader* column_reader) {
    m_column_map[column_reader->get_id()] = column_reader;
    m_columns.push_back(column_reader);
}

auto SchemaReader::append_unordered_column(std::unique_ptr<BaseColumnReader> column_reader)
        -> void {
    m_columns.push_back(column_reader.release());
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
            case JsonSerializer::Op::AddReconstructedTextField: {
                m_json_serializer.append_key();
                m_json_serializer.append_reconstructed_text_value(message_index);
                break;
            }
            case JsonSerializer::Op::AddReconstructedTextValue: {
                m_json_serializer.append_reconstructed_text_value(message_index);
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

auto SchemaReader::mark_unordered_object(
        size_t start_column_reader_idx,
        mpt_node_id_t subtree_root,
        SchemaView sub_schema,
        std::optional<clpp::log_shape_id_t> log_shape_id
) -> void {
    if (NodeType::ParentRule == m_global_schema_tree->get_node(subtree_root).get_type()) {
        return;
    }
    m_global_id_to_unordered_object.emplace(
            subtree_root,
            MarkedUnorderedObject{
                    .start_column_reader_idx = start_column_reader_idx,
                    .sub_schema = sub_schema,
                    .log_shape_id = log_shape_id
            }
    );
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
        } else if (NodeType::StructurizedArray == node.get_type()) {
            m_json_serializer.add_op(
                    no_name ? JsonSerializer::Op::BeginUnnamedArray : JsonSerializer::Op::BeginArray
            );
        }
    }
    path_to_intersection.clear();
}

auto SchemaReader::generate_structured_array_template(
        int32_t array_root_id,
        size_t start_column_reader_idx,
        SchemaView sub_schema
) -> ystdlib::error_handling::Result<size_t> {
    auto column_idx{start_column_reader_idx};
    std::vector<int32_t> path_to_intersection;
    int32_t depth = m_global_schema_tree->get_node(array_root_id).get_depth();

    for (auto const& entry : sub_schema) {
        if (auto const* obj{std::get_if<UnorderedObject>(&entry)}) {
            column_idx = YSTDLIB_ERROR_HANDLING_TRYX(
                    generate_array_nested_object_template(*obj, array_root_id, column_idx)
            );
            continue;
        }
        auto const global_column_id{std::get<mpt_node_id_t>(entry)};
        auto const& node = m_global_schema_tree->get_node(global_column_id);
        switch (node.get_type()) {
            case NodeType::Object: {
                find_intersection_and_fix_brackets(
                        array_root_id,
                        global_column_id,
                        path_to_intersection
                );
                for (int j = 0; j < (node.get_depth() - depth); ++j) {
                    m_json_serializer.add_op(JsonSerializer::Op::EndObject);
                }
                break;
            }
            case NodeType::StructurizedArray: {
                m_json_serializer.add_op(JsonSerializer::Op::BeginUnnamedArray);
                m_json_serializer.add_op(JsonSerializer::Op::EndArray);
                break;
            }
            case NodeType::DeltaInteger:
            case NodeType::Integer: {
                m_json_serializer.add_op(JsonSerializer::Op::AddIntValue);
                m_reordered_columns.push_back(m_columns.at(column_idx++));
                break;
            }
            case NodeType::Float: {
                m_json_serializer.add_op(JsonSerializer::Op::AddFloatValue);
                m_reordered_columns.push_back(m_columns.at(column_idx++));
                break;
            }
            case NodeType::FormattedFloat:
            case NodeType::DictionaryFloat: {
                m_json_serializer.add_op(JsonSerializer::Op::AddFormattedFloatValue);
                m_reordered_columns.push_back(m_columns.at(column_idx++));
                break;
            }
            case NodeType::Boolean: {
                m_json_serializer.add_op(JsonSerializer::Op::AddBoolValue);
                m_reordered_columns.push_back(m_columns.at(column_idx++));
                break;
            }
            case NodeType::ClpString:
            case NodeType::VarString: {
                m_json_serializer.add_op(JsonSerializer::Op::AddStringValue);
                m_reordered_columns.push_back(m_columns.at(column_idx++));
                break;
            }
            case NodeType::NullValue: {
                m_json_serializer.add_op(JsonSerializer::Op::AddNullValue);
                break;
            }
            case NodeType::DeprecatedDateString:
            case NodeType::UnstructuredArray:
            case NodeType::Metadata:
            case NodeType::StructurizedText:
            case NodeType::ParentRule:
            case NodeType::Timestamp:
            case NodeType::Unknown:
                break;
        }
    }
    return column_idx;
}

auto SchemaReader::generate_array_nested_object_template(
        UnorderedObject const& obj,
        int32_t array_root_id,
        size_t start_column_reader_idx
) -> ystdlib::error_handling::Result<size_t> {
    if (NodeType::StructurizedText == obj.type) {
        if (false == obj.log_shape_id.has_value()) {
            return clpp::ClppErrorCode{clpp::ClppErrorCodeEnum::Corrupt};
        }
        auto const end_idx{compute_end_column_reader_idx(start_column_reader_idx, obj.sub_schema)};
        YSTDLIB_ERROR_HANDLING_TRYV(m_json_serializer.add_reconstructed_text_value_op(
                YSTDLIB_ERROR_HANDLING_TRYX(get_log_shape(obj.log_shape_id.value())),
                YSTDLIB_ERROR_HANDLING_TRYX(scope_column_readers(start_column_reader_idx, end_idx))
        ));
        return end_idx;
    }

    auto const is_array{NodeType::StructurizedArray == obj.type};
    if (false == is_array && NodeType::Object != obj.type) {
        return clpp::ClppErrorCode{clpp::ClppErrorCodeEnum::Corrupt};
    }
    auto const first_node_id{obj.sub_schema.first_node_id()};
    if (false == first_node_id.has_value()) {
        return clpp::ClppErrorCode{clpp::ClppErrorCodeEnum::Corrupt};
    }
    auto const sub_root{m_global_schema_tree->find_matching_subtree_root_in_subtree(
            array_root_id,
            first_node_id.value(),
            obj.type
    )};
    if (constants::cRootNodeId == sub_root) {
        return clpp::ClppErrorCode{clpp::ClppErrorCodeEnum::Corrupt};
    }

    m_json_serializer.add_op(
            is_array ? JsonSerializer::Op::BeginUnnamedArray
                     : JsonSerializer::Op::BeginUnnamedObject
    );
    auto const end_idx{YSTDLIB_ERROR_HANDLING_TRYX(
            is_array ? generate_structured_array_template(
                               sub_root,
                               start_column_reader_idx,
                               obj.sub_schema
                       )
                     : generate_structured_object_template(
                               sub_root,
                               start_column_reader_idx,
                               obj.sub_schema
                       )
    )};
    m_json_serializer.add_op(
            is_array ? JsonSerializer::Op::EndArray : JsonSerializer::Op::EndObject
    );
    return end_idx;
}

auto SchemaReader::generate_structured_object_template(
        int32_t object_root,
        size_t start_column_reader_idx,
        SchemaView sub_schema
) -> ystdlib::error_handling::Result<size_t> {
    auto root{object_root};
    auto column_idx{start_column_reader_idx};
    std::vector<int32_t> path_to_intersection;

    for (auto const& entry : sub_schema) {
        if (auto const* obj{std::get_if<UnorderedObject>(&entry)}) {
            column_idx = YSTDLIB_ERROR_HANDLING_TRYX(generate_object_nested_object_template(
                    *obj,
                    object_root,
                    root,
                    path_to_intersection,
                    column_idx
            ));
            continue;
        }
        auto const& node = m_global_schema_tree->get_node(std::get<mpt_node_id_t>(entry));
        auto const next_root{node.get_parent_id()};
        find_intersection_and_fix_brackets(root, next_root, path_to_intersection);
        root = next_root;
        switch (node.get_type()) {
            case NodeType::Object: {
                m_json_serializer.add_op(JsonSerializer::Op::BeginObject);
                m_json_serializer.add_special_key(node.get_key_name());
                m_json_serializer.add_op(JsonSerializer::Op::EndObject);
                break;
            }
            case NodeType::StructurizedArray: {
                m_json_serializer.add_op(JsonSerializer::Op::BeginArray);
                m_json_serializer.add_special_key(node.get_key_name());
                m_json_serializer.add_op(JsonSerializer::Op::EndArray);
                break;
            }
            case NodeType::DeltaInteger:
            case NodeType::Integer: {
                m_json_serializer.add_op(JsonSerializer::Op::AddIntField);
                m_reordered_columns.push_back(m_columns.at(column_idx++));
                break;
            }
            case NodeType::Float: {
                m_json_serializer.add_op(JsonSerializer::Op::AddFloatField);
                m_reordered_columns.push_back(m_columns.at(column_idx++));
                break;
            }
            case NodeType::FormattedFloat:
            case NodeType::DictionaryFloat: {
                m_json_serializer.add_op(JsonSerializer::Op::AddFormattedFloatField);
                m_reordered_columns.push_back(m_columns.at(column_idx++));
                break;
            }
            case NodeType::Boolean: {
                m_json_serializer.add_op(JsonSerializer::Op::AddBoolField);
                m_reordered_columns.push_back(m_columns.at(column_idx++));
                break;
            }
            case NodeType::ClpString:
            case NodeType::VarString: {
                m_json_serializer.add_op(JsonSerializer::Op::AddStringField);
                m_reordered_columns.push_back(m_columns.at(column_idx++));
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
            case NodeType::StructurizedText:
            case NodeType::ParentRule:
            case NodeType::Timestamp:
            case NodeType::Unknown:
                break;
        }
    }
    find_intersection_and_fix_brackets(root, object_root, path_to_intersection);
    return column_idx;
}

auto SchemaReader::generate_object_nested_object_template(
        UnorderedObject const& obj,
        int32_t object_root,
        int32_t& cur_root,
        std::vector<int32_t>& path_to_intersection,
        size_t start_column_reader_idx
) -> ystdlib::error_handling::Result<size_t> {
    if (NodeType::StructurizedText == obj.type) {
        if (false == obj.node_id.has_value() || false == obj.log_shape_id.has_value()) {
            return clpp::ClppErrorCode{clpp::ClppErrorCodeEnum::Corrupt};
        }
        auto const& text_node{m_global_schema_tree->get_node(obj.node_id.value())};
        auto const parent_id{text_node.get_parent_id()};
        find_intersection_and_fix_brackets(cur_root, parent_id, path_to_intersection);
        cur_root = parent_id;
        auto const end_idx{compute_end_column_reader_idx(start_column_reader_idx, obj.sub_schema)};
        YSTDLIB_ERROR_HANDLING_TRYV(m_json_serializer.add_reconstructed_text_field_op(
                text_node.get_key_name(),
                YSTDLIB_ERROR_HANDLING_TRYX(get_log_shape(obj.log_shape_id.value())),
                YSTDLIB_ERROR_HANDLING_TRYX(scope_column_readers(start_column_reader_idx, end_idx))
        ));
        return end_idx;
    }

    if (NodeType::StructurizedArray != obj.type) {
        return clpp::ClppErrorCode{clpp::ClppErrorCodeEnum::Corrupt};
    }
    auto const first_node_id{obj.sub_schema.first_node_id()};
    if (false == first_node_id.has_value()) {
        return clpp::ClppErrorCode{clpp::ClppErrorCodeEnum::Corrupt};
    }
    auto const array_root{m_global_schema_tree->find_matching_subtree_root_in_subtree(
            object_root,
            first_node_id.value(),
            NodeType::StructurizedArray
    )};
    if (constants::cRootNodeId == array_root) {
        return clpp::ClppErrorCode{clpp::ClppErrorCodeEnum::Corrupt};
    }
    find_intersection_and_fix_brackets(cur_root, array_root, path_to_intersection);
    auto const end_idx{YSTDLIB_ERROR_HANDLING_TRYX(
            generate_structured_array_template(array_root, start_column_reader_idx, obj.sub_schema)
    )};
    m_json_serializer.add_op(JsonSerializer::Op::EndArray);
    cur_root = m_global_schema_tree->get_node(array_root).get_parent_id();
    return end_idx;
}

auto SchemaReader::compute_end_column_reader_idx(
        size_t start_column_reader_idx,
        SchemaView sub_schema
) const -> size_t {
    auto end_idx{start_column_reader_idx};
    sub_schema.for_each_node_id([&](mpt_node_id_t node_id) -> void {
        if (unordered_node_type_consumes_column(m_global_schema_tree->get_node(node_id).get_type()))
        {
            ++end_idx;
        }
    });
    return end_idx;
}

auto SchemaReader::scope_column_readers(
        size_t start_column_reader_idx,
        size_t end_column_reader_idx
) const -> ystdlib::error_handling::Result<std::span<BaseColumnReader* const>> {
    if (end_column_reader_idx > m_columns.size() || start_column_reader_idx > end_column_reader_idx)
    {
        return clpp::ClppErrorCode{clpp::ClppErrorCodeEnum::Corrupt};
    }
    return std::span<BaseColumnReader* const>{m_columns}.subspan(
            start_column_reader_idx,
            end_column_reader_idx - start_column_reader_idx
    );
}

auto SchemaReader::get_log_shape(clpp::log_shape_id_t log_shape_id) const
        -> ystdlib::error_handling::Result<clpp::TextShape<std::string_view>> {
    if (nullptr == m_log_shape_dict) {
        return clpp::ClppErrorCode{clpp::ClppErrorCodeEnum::Failure};
    }
    if (log_shape_id >= m_log_shape_dict->get_entries().size()) {
        return clpp::ClppErrorCode{clpp::ClppErrorCodeEnum::Corrupt};
    }
    return clpp::TextShape<std::string_view>{m_log_shape_dict->get_value(log_shape_id)};
}

auto SchemaReader::get_parent_rule_shape(
        clpp::log_shape_id_t log_shape_id,
        std::string_view parent_rule_column_name,
        size_t leaf_position,
        size_t num_placeholders
) const -> ystdlib::error_handling::Result<clpp::TextShape<std::string_view>> {
    if (nullptr == m_parent_rule_shapes_array) {
        return clpp::ClppErrorCode{clpp::ClppErrorCodeEnum::Failure};
    }
    if (log_shape_id >= m_parent_rule_shapes_array->size()) {
        return clpp::ClppErrorCode{clpp::ClppErrorCodeEnum::Corrupt};
    }
    auto const parent_rule_shape{YSTDLIB_ERROR_HANDLING_TRYX(get_log_shape(log_shape_id))
                                         .narrow_to_parent_rule(
                                                 m_parent_rule_shapes_array->get(log_shape_id),
                                                 parent_rule_column_name,
                                                 leaf_position,
                                                 num_placeholders
                                         )};
    if (parent_rule_shape.empty()) {
        return clpp::ClppErrorCode{clpp::ClppErrorCodeEnum::Corrupt};
    }
    return parent_rule_shape;
}

void SchemaReader::initialize_serializer() {
    if (m_serializer_initialized) {
        return;
    }

    for (int32_t global_column_id : m_ordered_schema) {
        if (m_projection->matches_node(global_column_id)) {
            generate_local_tree(global_column_id);
        }
    }

    for (auto const& [root_id, unordered_object] : m_global_id_to_unordered_object) {
        if (m_projection->matches_node(root_id)
            || contains_projected_node(unordered_object.sub_schema, *m_projection))
        {
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
        if (auto const result{generate_json_template(subtree_root)}; result.has_error()) {
            throw std::runtime_error(
                    "generate_json_template failed with: " + result.error().message()
            );
        }
    }

    m_serializer_initialized = true;
}

auto SchemaReader::generate_json_template(int32_t id) -> ystdlib::error_handling::Result<void> {
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
                YSTDLIB_ERROR_HANDLING_TRYV(generate_json_template(child_id));
                m_json_serializer.add_op(JsonSerializer::Op::EndObject);
                break;
            }
            case NodeType::UnstructuredArray: {
                m_json_serializer.add_op(JsonSerializer::Op::AddArrayField);
                m_reordered_columns.push_back(m_column_map[child_global_id]);
                break;
            }
            case NodeType::StructurizedArray: {
                m_json_serializer.add_op(JsonSerializer::Op::BeginArray);
                m_json_serializer.add_special_key(key);
                auto structured_it = m_global_id_to_unordered_object.find(child_global_id);
                if (m_global_id_to_unordered_object.end() != structured_it) {
                    auto const& record{structured_it->second};
                    std::ignore = YSTDLIB_ERROR_HANDLING_TRYX(generate_structured_array_template(
                            child_global_id,
                            record.start_column_reader_idx,
                            record.sub_schema
                    ));
                }
                m_json_serializer.add_op(JsonSerializer::Op::EndArray);
                break;
            }
            case NodeType::StructurizedText: {
                YSTDLIB_ERROR_HANDLING_TRYV(generate_structurized_text_template(child_global_id));
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
            case NodeType::ParentRule:
            case NodeType::Unknown: {
                break;
            }
        }
    }
    return ystdlib::error_handling::success();
}

auto SchemaReader::collect_scope_entries(
        SchemaView schema,
        mpt_node_id_t scope_node_id,
        size_t start_column_reader_idx,
        bool ancestor_decomposed
) -> ystdlib::error_handling::Result<SchemaSpanContents> {
    SchemaSpanContents scope;
    size_t column_reader_idx{start_column_reader_idx};
    bool const scope_decomposed{
            ancestor_decomposed
            || (m_projection
                && m_projection->is_projected_as(
                        scope_node_id,
                        search::Projection::NodeModes::Mode::Decompose
                ))
    };
    for (auto const& entry : schema) {
        if (auto const* obj{std::get_if<UnorderedObject>(&entry)}) {
            auto const end_idx{compute_end_column_reader_idx(column_reader_idx, obj->sub_schema)};
            if (NodeType::ParentRule == obj->type) {
                if (false == obj->node_id.has_value()) {
                    return clpp::ClppErrorCode{clpp::ClppErrorCodeEnum::Corrupt};
                }
                auto const parent_rule_id{obj->node_id.value()};
                auto const [it, inserted]{
                        scope.parent_rule_occurrences.try_emplace(parent_rule_id)
                };
                if (inserted) {
                    scope.parent_rule_insertion_order.push_back(parent_rule_id);
                }
                it->second.push_back(
                        ParentRuleOccurrence{
                                .sub_schema = obj->sub_schema,
                                .start_column_reader_idx = column_reader_idx,
                                .end_column_reader_idx = end_idx
                        }
                );
            }
            column_reader_idx = end_idx;
            continue;
        }

        auto const& node{m_global_schema_tree->get_node(std::get<mpt_node_id_t>(entry))};
        if (false == unordered_node_type_consumes_column(node.get_type())) {
            continue;
        }
        if (scope_decomposed || (m_projection && m_projection->should_emit_value(node))) {
            scope.entries.push_back(
                    DecompositionTarget{
                            .reader_idx = column_reader_idx,
                            .name = node.get_key_name(),
                            .type = node.get_type()
                    }
            );
        }
        ++column_reader_idx;
    }
    return scope;
}

auto SchemaReader::add_parent_rule_array_ops(
        SchemaSpanContents const& scope,
        clpp::log_shape_id_t log_shape_id,
        size_t structurized_text_column_start,
        bool ancestor_decomposed
) -> ystdlib::error_handling::Result<void> {
    for (auto const parent_rule_id : scope.parent_rule_insertion_order) {
        auto const parent_rule_modes{
                m_projection ? m_projection->get_node_modes(parent_rule_id)
                             : search::Projection::NodeModes{}
        };
        auto const& parent_rule_node{m_global_schema_tree->get_node(parent_rule_id)};
        auto const& occurrences{scope.parent_rule_occurrences.at(parent_rule_id)};
        bool const emit_text{m_projection && m_projection->should_emit_value(parent_rule_node)};
        bool const emit_shape{
                parent_rule_modes.contains(search::Projection::NodeModes::Mode::Shape)
        };
        bool const emit_parent_rule{
                emit_text || emit_shape || ancestor_decomposed
                || (m_projection
                    && std::ranges::any_of(occurrences, [&](auto const& occurrence) -> bool {
                           return contains_projected_node(occurrence.sub_schema, *m_projection);
                       }))
        };
        if (false == emit_parent_rule) {
            continue;
        }

        m_json_serializer.add_op(JsonSerializer::Op::BeginArray);
        m_json_serializer.add_special_key(parent_rule_node.get_key_name());

        auto const parent_rule_column_name{
                m_global_schema_tree->build_ls_rule_name(parent_rule_id)
        };
        bool const decompose_children{
                ancestor_decomposed
                || parent_rule_modes.contains(search::Projection::NodeModes::Mode::Decompose)
        };
        for (auto const& occurrence : occurrences) {
            m_json_serializer.add_op(JsonSerializer::Op::BeginUnnamedObject);
            YSTDLIB_ERROR_HANDLING_TRYV(add_parent_rule_occurrence_ops(
                    parent_rule_id,
                    occurrence,
                    parent_rule_column_name,
                    log_shape_id,
                    structurized_text_column_start,
                    emit_text,
                    emit_shape,
                    decompose_children
            ));
            m_json_serializer.add_op(JsonSerializer::Op::EndObject);
        }

        m_json_serializer.add_op(JsonSerializer::Op::EndArray);
    }
    return ystdlib::error_handling::success();
}

auto SchemaReader::add_parent_rule_occurrence_ops(
        mpt_node_id_t parent_rule_id,
        ParentRuleOccurrence const& occurrence,
        std::string_view parent_rule_column_name,
        clpp::log_shape_id_t log_shape_id,
        size_t structurized_text_column_start,
        bool emit_text,
        bool emit_shape,
        bool decompose_children
) -> ystdlib::error_handling::Result<void> {
    if (emit_text || emit_shape) {
        auto const parent_rule_shape{YSTDLIB_ERROR_HANDLING_TRYX(get_parent_rule_shape(
                log_shape_id,
                parent_rule_column_name,
                occurrence.start_column_reader_idx - structurized_text_column_start,
                occurrence.end_column_reader_idx - occurrence.start_column_reader_idx
        ))};
        if (emit_text) {
            YSTDLIB_ERROR_HANDLING_TRYV(m_json_serializer.add_reconstructed_text_field_op(
                    cTextKey,
                    parent_rule_shape,
                    YSTDLIB_ERROR_HANDLING_TRYX(scope_column_readers(
                            occurrence.start_column_reader_idx,
                            occurrence.end_column_reader_idx
                    ))
            ));
        }
        if (emit_shape) {
            m_json_serializer.add_constant_string_field_op(cShapeKey, parent_rule_shape.view());
        }
    }

    YSTDLIB_ERROR_HANDLING_TRYV(add_decomposed_scope_ops(
            occurrence.sub_schema,
            parent_rule_id,
            occurrence.start_column_reader_idx,
            log_shape_id,
            structurized_text_column_start,
            decompose_children
    ));
    return ystdlib::error_handling::success();
}

auto SchemaReader::add_decomposed_scope_ops(
        SchemaView schema,
        mpt_node_id_t scope_node_id,
        size_t start_column_reader_idx,
        clpp::log_shape_id_t log_shape_id,
        size_t structurized_text_column_start,
        bool ancestor_decomposed
) -> ystdlib::error_handling::Result<void> {
    auto scope{YSTDLIB_ERROR_HANDLING_TRYX(collect_scope_entries(
            schema,
            scope_node_id,
            start_column_reader_idx,
            ancestor_decomposed
    ))};
    YSTDLIB_ERROR_HANDLING_TRYV(add_grouped_leaf_array_ops(scope.entries));
    return add_parent_rule_array_ops(
            scope,
            log_shape_id,
            structurized_text_column_start,
            ancestor_decomposed
    );
}

auto SchemaReader::add_grouped_leaf_array_ops(std::vector<DecompositionTarget>& entries)
        -> ystdlib::error_handling::Result<void> {
    std::stable_sort(entries.begin(), entries.end(), [](auto const& a, auto const& b) -> bool {
        return a.name < b.name;
    });

    for (size_t i{0}; i < entries.size();) {
        auto const& col_name{entries.at(i).name};
        m_json_serializer.add_special_key(col_name);
        m_json_serializer.add_op(JsonSerializer::Op::BeginArray);

        size_t j{i};
        while (j < entries.size() && entries.at(j).name == col_name) {
            switch (entries.at(j).type) {
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
            m_reordered_columns.push_back(m_columns.at(entries.at(j).reader_idx));
            ++j;
        }

        m_json_serializer.add_op(JsonSerializer::Op::EndArray);
        i = j;
    }
    return ystdlib::error_handling::success();
}

auto SchemaReader::generate_structurized_text_template(mpt_node_id_t structurized_text_id)
        -> ystdlib::error_handling::Result<void> {
    auto structurized_text_it{m_global_id_to_unordered_object.find(structurized_text_id)};
    if (m_global_id_to_unordered_object.end() == structurized_text_it) {
        return clpp::ClppErrorCode{clpp::ClppErrorCodeEnum::Failure};
    }
    auto const& record{structurized_text_it->second};
    if (false == record.log_shape_id.has_value()) {
        return clpp::ClppErrorCode{clpp::ClppErrorCodeEnum::Corrupt};
    }
    auto const log_shape_id{record.log_shape_id.value()};
    auto const structurized_text_column_start{record.start_column_reader_idx};
    auto const schema{record.sub_schema};
    auto const structurized_text_column_end{
            compute_end_column_reader_idx(structurized_text_column_start, schema)
    };

    auto const& structurized_text_node{m_global_schema_tree->get_node(structurized_text_id)};
    auto const key_name{structurized_text_node.get_key_name()};

    bool const emit_bare_text{
            m_extract_mode || nullptr == m_projection
            || search::Projection::Mode::Default == m_projection->get_mode()
    };
    if (emit_bare_text) {
        return m_json_serializer.add_reconstructed_text_field_op(
                key_name,
                YSTDLIB_ERROR_HANDLING_TRYX(get_log_shape(log_shape_id)),
                YSTDLIB_ERROR_HANDLING_TRYX(scope_column_readers(
                        structurized_text_column_start,
                        structurized_text_column_end
                ))
        );
    }

    auto const modes{m_projection->get_node_modes(structurized_text_id)};
    bool const has_shape{modes.contains(search::Projection::NodeModes::Mode::Shape)};
    bool const has_decompose{modes.contains(search::Projection::NodeModes::Mode::Decompose)};
    bool const emit_text{m_projection->should_emit_value(structurized_text_node)};

    m_json_serializer.add_op(JsonSerializer::Op::BeginObject);
    m_json_serializer.add_special_key(key_name);

    if (emit_text || has_shape) {
        auto const log_shape{YSTDLIB_ERROR_HANDLING_TRYX(get_log_shape(log_shape_id))};
        if (emit_text) {
            YSTDLIB_ERROR_HANDLING_TRYV(m_json_serializer.add_reconstructed_text_field_op(
                    cTextKey,
                    log_shape,
                    YSTDLIB_ERROR_HANDLING_TRYX(scope_column_readers(
                            structurized_text_column_start,
                            structurized_text_column_end
                    ))
            ));
        }
        if (has_shape) {
            m_json_serializer.add_constant_string_field_op(cShapeKey, log_shape.view());
        }
    }

    YSTDLIB_ERROR_HANDLING_TRYV(add_decomposed_scope_ops(
            schema,
            structurized_text_id,
            structurized_text_column_start,
            log_shape_id,
            structurized_text_column_start,
            has_decompose
    ));

    m_json_serializer.add_op(JsonSerializer::Op::EndObject);
    return ystdlib::error_handling::success();
}
}  // namespace clp_s
