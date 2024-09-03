#include "JsonParser.hpp"

#include <iostream>
#include <stack>

#include <simdjson.h>
#include <spdlog/spdlog.h>

#include "archive_constants.hpp"
#include "JsonFileIterator.hpp"

namespace clp_s {
JsonParser::JsonParser(JsonParserOption const& option)
        : m_num_messages(0),
          m_target_encoded_size(option.target_encoded_size),
          m_max_document_size(option.max_document_size),
          m_timestamp_key(option.timestamp_key),
          m_structurize_arrays(option.structurize_arrays) {
    if (false == FileUtils::validate_path(option.file_paths)) {
        exit(1);
    }

    if (false == m_timestamp_key.empty()) {
        clp_s::StringUtils::tokenize_column_descriptor(m_timestamp_key, m_timestamp_column);
    }

    for (auto& file_path : option.file_paths) {
        FileUtils::find_all_files(file_path, m_file_paths);
    }

    m_archive_options.archives_dir = option.archives_dir;
    m_archive_options.compression_level = option.compression_level;
    m_archive_options.print_archive_stats = option.print_archive_stats;
    m_archive_options.id = m_generator();

    m_archive_writer = std::make_unique<ArchiveWriter>(option.metadata_db);
    m_archive_writer->open(m_archive_options);
}

void JsonParser::parse_obj_in_array(ondemand::object line, int32_t parent_node_id) {
    ondemand::object_iterator it = line.begin();
    if (it == line.end()) {
        m_current_schema.insert_unordered(parent_node_id);
        return;
    }

    std::stack<ondemand::object> object_stack;
    std::stack<ondemand::object_iterator> object_it_stack;

    std::stack<int32_t> node_id_stack;
    node_id_stack.push(parent_node_id);
    object_stack.push(std::move(line));
    object_it_stack.push(std::move(it));

    size_t object_start = m_current_schema.start_unordered_object(NodeType::Object);
    ondemand::field cur_field;
    ondemand::value cur_value;
    std::string cur_key;
    int32_t node_id;
    while (true) {
        while (false == object_stack.empty() && object_it_stack.top() == object_stack.top().end()) {
            object_stack.pop();
            node_id_stack.pop();
            object_it_stack.pop();
            if (false == object_it_stack.empty()) {
                ++object_it_stack.top();
            }
        }

        if (object_stack.empty()) {
            m_current_schema.end_unordered_object(object_start);
            return;
        }

        cur_field = *object_it_stack.top();
        cur_key = std::string_view(cur_field.unescaped_key(true));
        cur_value = cur_field.value();

        switch (cur_value.type()) {
            case ondemand::json_type::object: {
                node_id = m_archive_writer
                                  ->add_node(node_id_stack.top(), NodeType::Object, cur_key);
                object_stack.push(std::move(cur_value.get_object()));
                object_it_stack.push(std::move(object_stack.top().begin()));
                if (object_it_stack.top() == object_stack.top().end()) {
                    m_current_schema.insert_unordered(node_id);
                    object_stack.pop();
                    object_it_stack.pop();
                } else {
                    node_id_stack.push(node_id);
                    continue;
                }
                break;
            }
            case ondemand::json_type::array: {
                node_id = m_archive_writer->add_node(
                        node_id_stack.top(),
                        NodeType::StructuredArray,
                        cur_key
                );
                parse_array(cur_value.get_array(), node_id);
                break;
            }
            case ondemand::json_type::number: {
                ondemand::number number_value = cur_value.get_number();
                if (true == number_value.is_double()) {
                    double double_value = number_value.get_double();
                    m_current_parsed_message.add_unordered_value(double_value);
                    node_id = m_archive_writer
                                      ->add_node(node_id_stack.top(), NodeType::Float, cur_key);
                } else {
                    int64_t i64_value;
                    if (number_value.is_uint64()) {
                        i64_value = static_cast<int64_t>(number_value.get_uint64());
                    } else {
                        i64_value = number_value.get_int64();
                    }
                    m_current_parsed_message.add_unordered_value(i64_value);
                    node_id = m_archive_writer
                                      ->add_node(node_id_stack.top(), NodeType::Integer, cur_key);
                }
                m_current_schema.insert_unordered(node_id);
                break;
            }
            case ondemand::json_type::string: {
                std::string value = std::string(
                        cur_value.raw_json_token().substr(1, cur_value.raw_json_token().size() - 2)
                );
                if (value.find(' ') != std::string::npos) {
                    node_id = m_archive_writer
                                      ->add_node(node_id_stack.top(), NodeType::ClpString, cur_key);
                } else {
                    node_id = m_archive_writer
                                      ->add_node(node_id_stack.top(), NodeType::VarString, cur_key);
                }
                m_current_parsed_message.add_unordered_value(value);
                m_current_schema.insert_unordered(node_id);
                break;
            }
            case ondemand::json_type::boolean: {
                bool value = cur_value.get_bool();
                m_current_parsed_message.add_unordered_value(value);
                node_id = m_archive_writer
                                  ->add_node(node_id_stack.top(), NodeType::Boolean, cur_key);
                m_current_schema.insert_unordered(node_id);
                break;
            }
            case ondemand::json_type::null: {
                node_id = m_archive_writer
                                  ->add_node(node_id_stack.top(), NodeType::NullValue, cur_key);
                m_current_schema.insert_unordered(node_id);
                break;
            }
        }

        ++object_it_stack.top();
    }
}

void JsonParser::parse_array(ondemand::array array, int32_t parent_node_id) {
    ondemand::array_iterator it = array.begin();
    if (it == array.end()) {
        m_current_schema.insert_unordered(parent_node_id);
        return;
    }

    size_t array_start = m_current_schema.start_unordered_object(NodeType::StructuredArray);
    ondemand::value cur_value;
    int32_t node_id;
    for (; it != array.end(); ++it) {
        cur_value = *it;

        switch (cur_value.type()) {
            case ondemand::json_type::object: {
                node_id = m_archive_writer->add_node(parent_node_id, NodeType::Object, "");
                parse_obj_in_array(std::move(cur_value.get_object()), node_id);
                break;
            }
            case ondemand::json_type::array: {
                node_id = m_archive_writer->add_node(parent_node_id, NodeType::StructuredArray, "");
                parse_array(std::move(cur_value.get_array()), node_id);
                break;
            }
            case ondemand::json_type::number: {
                ondemand::number number_value = cur_value.get_number();
                if (true == number_value.is_double()) {
                    double double_value = number_value.get_double();
                    m_current_parsed_message.add_unordered_value(double_value);
                    node_id = m_archive_writer->add_node(parent_node_id, NodeType::Float, "");
                } else {
                    int64_t i64_value;
                    if (number_value.is_uint64()) {
                        i64_value = static_cast<int64_t>(number_value.get_uint64());
                    } else {
                        i64_value = number_value.get_int64();
                    }
                    m_current_parsed_message.add_unordered_value(i64_value);
                    node_id = m_archive_writer->add_node(parent_node_id, NodeType::Integer, "");
                }
                m_current_schema.insert_unordered(node_id);
                break;
            }
            case ondemand::json_type::string: {
                std::string value = std::string(
                        cur_value.raw_json_token().substr(1, cur_value.raw_json_token().size() - 2)
                );
                if (value.find(' ') != std::string::npos) {
                    node_id = m_archive_writer->add_node(parent_node_id, NodeType::ClpString, "");
                } else {
                    node_id = m_archive_writer->add_node(parent_node_id, NodeType::VarString, "");
                }
                m_current_parsed_message.add_unordered_value(value);
                m_current_schema.insert_unordered(node_id);
                break;
            }
            case ondemand::json_type::boolean: {
                bool value = cur_value.get_bool();
                m_current_parsed_message.add_unordered_value(value);
                node_id = m_archive_writer->add_node(parent_node_id, NodeType::Boolean, "");
                m_current_schema.insert_unordered(node_id);
                break;
            }
            case ondemand::json_type::null: {
                node_id = m_archive_writer->add_node(parent_node_id, NodeType::NullValue, "");
                m_current_schema.insert_unordered(node_id);
                break;
            }
        }
    }
    m_current_schema.end_unordered_object(array_start);
}

void JsonParser::parse_line(ondemand::value line, int32_t parent_node_id, std::string const& key) {
    int32_t node_id;
    std::stack<ondemand::object> object_stack;
    std::stack<int32_t> node_id_stack;
    std::stack<ondemand::object_iterator> object_it_stack;

    ondemand::field cur_field;

    std::string cur_key = key;
    node_id_stack.push(parent_node_id);

    bool can_match_timestamp = !m_timestamp_column.empty();
    bool may_match_timestamp = can_match_timestamp;
    int longest_matching_timestamp_prefix = 0;
    bool matches_timestamp = false;

    do {
        if (false == object_stack.empty()) {
            cur_field = *object_it_stack.top();
            cur_key = std::string(std::string_view(cur_field.unescaped_key(true)));
            line = cur_field.value();
            if (may_match_timestamp) {
                if (object_stack.size() <= m_timestamp_column.size()
                    && cur_key == m_timestamp_column[object_stack.size() - 1])
                {
                    if (object_stack.size() == m_timestamp_column.size()) {
                        // FIXME: technically need to handle the case where this
                        // isn't a string or number column by resetting matches_timestamp
                        // to false
                        matches_timestamp = true;
                    }
                } else {
                    longest_matching_timestamp_prefix = object_stack.size() - 1;
                    may_match_timestamp = false;
                }
            }
        }

        switch (line.type()) {
            case ondemand::json_type::object: {
                node_id = m_archive_writer
                                  ->add_node(node_id_stack.top(), NodeType::Object, cur_key);
                object_stack.push(std::move(line.get_object()));
                auto objref = object_stack.top();
                auto it = ondemand::object_iterator(objref.begin());
                if (it == objref.end()) {
                    m_current_schema.insert_ordered(node_id);
                    object_stack.pop();
                    break;
                } else {
                    object_it_stack.push(it);
                    node_id_stack.push(node_id);
                    continue;
                }
            }
            case ondemand::json_type::array: {
                if (m_structurize_arrays) {
                    node_id = m_archive_writer->add_node(
                            node_id_stack.top(),
                            NodeType::StructuredArray,
                            cur_key
                    );
                    parse_array(std::move(line.get_array()), node_id);
                } else {
                    std::string value
                            = std::string(std::string_view(simdjson::to_json_string(line)));
                    node_id = m_archive_writer->add_node(
                            node_id_stack.top(),
                            NodeType::UnstructuredArray,
                            cur_key
                    );
                    m_current_parsed_message.add_value(node_id, value);
                    m_current_schema.insert_ordered(node_id);
                }
                break;
            }
            case ondemand::json_type::number: {
                NodeType type;
                ondemand::number number_value = line.get_number();
                if (false == number_value.is_double()) {
                    // FIXME: should have separate integer and unsigned
                    // integer types to handle values greater than max int64
                    type = NodeType::Integer;
                } else {
                    type = NodeType::Float;
                }
                node_id = m_archive_writer->add_node(node_id_stack.top(), type, cur_key);

                if (type == NodeType::Integer) {
                    int64_t i64_value;
                    if (number_value.is_uint64()) {
                        i64_value = static_cast<int64_t>(number_value.get_uint64());
                    } else {
                        i64_value = line.get_int64();
                    }

                    m_current_parsed_message.add_value(node_id, i64_value);
                    if (matches_timestamp) {
                        m_archive_writer
                                ->ingest_timestamp_entry(m_timestamp_key, node_id, i64_value);
                        matches_timestamp = may_match_timestamp = can_match_timestamp = false;
                    }
                } else {
                    double double_value = line.get_double();
                    m_current_parsed_message.add_value(node_id, double_value);
                    if (matches_timestamp) {
                        m_archive_writer
                                ->ingest_timestamp_entry(m_timestamp_key, node_id, double_value);
                        matches_timestamp = may_match_timestamp = can_match_timestamp = false;
                    }
                }
                m_current_schema.insert_ordered(node_id);
                break;
            }
            case ondemand::json_type::string: {
                auto raw_json_token = line.raw_json_token();
                std::string value
                        = std::string(raw_json_token.substr(1, raw_json_token.rfind('"') - 1));

                if (matches_timestamp) {
                    node_id = m_archive_writer->add_node(
                            node_id_stack.top(),
                            NodeType::DateString,
                            cur_key
                    );
                    uint64_t encoding_id{0};
                    epochtime_t timestamp = m_archive_writer->ingest_timestamp_entry(
                            m_timestamp_key,
                            node_id,
                            value,
                            encoding_id
                    );
                    m_current_parsed_message.add_value(node_id, encoding_id, timestamp);
                    matches_timestamp = may_match_timestamp = can_match_timestamp = false;
                } else if (value.find(' ') != std::string::npos) {
                    node_id = m_archive_writer
                                      ->add_node(node_id_stack.top(), NodeType::ClpString, cur_key);
                    m_current_parsed_message.add_value(node_id, value);
                } else {
                    node_id = m_archive_writer
                                      ->add_node(node_id_stack.top(), NodeType::VarString, cur_key);
                    m_current_parsed_message.add_value(node_id, value);
                }

                m_current_schema.insert_ordered(node_id);
                break;
            }
            case ondemand::json_type::boolean: {
                bool value = line.get_bool();
                node_id = m_archive_writer
                                  ->add_node(node_id_stack.top(), NodeType::Boolean, cur_key);
                m_current_parsed_message.add_value(node_id, value);
                m_current_schema.insert_ordered(node_id);
                break;
            }
            case ondemand::json_type::null: {
                node_id = m_archive_writer
                                  ->add_node(node_id_stack.top(), NodeType::NullValue, cur_key);
                m_current_schema.insert_ordered(node_id);
                break;
            }
        }

        if (object_stack.empty()) {
            break;
        }

        bool hit_end;
        do {
            hit_end = false;
            ++object_it_stack.top();
            if (object_it_stack.top() == object_stack.top().end()) {
                object_it_stack.pop();
                object_stack.pop();
                node_id_stack.pop();
                hit_end = true;
            }
            if (can_match_timestamp
                && (object_it_stack.size() - 1) <= longest_matching_timestamp_prefix)
            {
                may_match_timestamp = true;
            }
        } while (!object_it_stack.empty() && hit_end);
    }

    while (!object_stack.empty());
}

bool JsonParser::parse() {
    for (auto& file_path : m_file_paths) {
        JsonFileIterator json_file_iterator(file_path, m_max_document_size);
        if (false == json_file_iterator.is_open()) {
            m_archive_writer->close();
            return false;
        }

        if (simdjson::error_code::SUCCESS != json_file_iterator.get_error()) {
            SPDLOG_ERROR(
                    "Encountered error - {} - while trying to parse {} after parsing 0 bytes",
                    simdjson::error_message(json_file_iterator.get_error()),
                    file_path
            );
            m_archive_writer->close();
            return false;
        }

        simdjson::ondemand::document_stream::iterator json_it;

        m_num_messages = 0;
        size_t bytes_consumed_up_to_prev_archive = 0;
        size_t bytes_consumed_up_to_prev_record = 0;
        while (json_file_iterator.get_json(json_it)) {
            m_current_schema.clear();

            auto ref = *json_it;
            auto is_scalar_result = ref.is_scalar();
            // If you don't check the error on is_scalar it will sometimes throw TAPE_ERROR when
            // converting to bool. The error being TAPE_ERROR or is_scalar() being true both mean
            // that this isn't a valid JSON document but they get set in different situations so we
            // need to check both here.
            if (is_scalar_result.error() || true == is_scalar_result.value()) {
                SPDLOG_ERROR(
                        "Encountered non-json-object while trying to parse {} after parsing {} "
                        "bytes",
                        file_path,
                        bytes_consumed_up_to_prev_record
                );
                m_archive_writer->close();
                return false;
            }

            // Some errors from simdjson are latent until trying to access invalid JSON fields.
            // Instead of checking for an error every time we access a JSON field in parse_line we
            // just catch simdjson_error here instead.
            try {
                parse_line(ref.value(), -1, "");
            } catch (simdjson::simdjson_error& error) {
                SPDLOG_ERROR(
                        "Encountered error - {} - while trying to parse {} after parsing {} bytes",
                        error.what(),
                        file_path,
                        bytes_consumed_up_to_prev_record
                );
                m_archive_writer->close();
                return false;
            }
            m_num_messages++;

            int32_t current_schema_id = m_archive_writer->add_schema(m_current_schema);
            m_current_parsed_message.set_id(current_schema_id);
            m_archive_writer
                    ->append_message(current_schema_id, m_current_schema, m_current_parsed_message);

            bytes_consumed_up_to_prev_record = json_file_iterator.get_num_bytes_consumed();
            if (m_archive_writer->get_data_size() >= m_target_encoded_size) {
                m_archive_writer->increment_uncompressed_size(
                        bytes_consumed_up_to_prev_record - bytes_consumed_up_to_prev_archive
                );
                bytes_consumed_up_to_prev_archive = bytes_consumed_up_to_prev_record;
                split_archive();
            }

            m_current_parsed_message.clear();
        }

        m_archive_writer->increment_uncompressed_size(
                json_file_iterator.get_num_bytes_read() - bytes_consumed_up_to_prev_archive
        );

        if (simdjson::error_code::SUCCESS != json_file_iterator.get_error()) {
            SPDLOG_ERROR(
                    "Encountered error - {} - while trying to parse {} after parsing {} bytes",
                    simdjson::error_message(json_file_iterator.get_error()),
                    file_path,
                    bytes_consumed_up_to_prev_record
            );
            m_archive_writer->close();
            return false;
        } else if (json_file_iterator.truncated_bytes() > 0) {
            // currently don't treat truncated bytes at the end of the file as an error
            SPDLOG_WARN(
                    "Truncated JSON  ({} bytes) at end of file {}",
                    json_file_iterator.truncated_bytes(),
                    file_path.c_str()
            );
        }
    }
    return true;
}

void JsonParser::store() {
    m_archive_writer->close();
}

void JsonParser::split_archive() {
    m_archive_writer->close();
    m_archive_options.id = m_generator();
    m_archive_writer->open(m_archive_options);
}

}  // namespace clp_s
