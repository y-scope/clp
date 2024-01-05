#include "ColumnWriter.hpp"

namespace clp_s {
template <typename T>
static void write_numeric_value(std::vector<uint8_t>& target, T value) {
    target.resize(target.size() + sizeof(T));
    uint8_t* addr = &target.back() - sizeof(T) + 1;
    memcpy(addr, &value, sizeof(T));
}

static void write_numeric_values(std::vector<uint8_t>& target, int64_t* value, size_t size) {
    target.resize(target.size() + sizeof(int64_t) * size);
    uint8_t* addr = &target.back() - sizeof(int64_t) * size + 1;
    memcpy(addr, value, sizeof(int64_t) * size);
}

static void write_string(std::vector<uint8_t>& target, std::string const& value) {
    target.resize(target.size() + value.size() + sizeof(uint16_t));
    uint8_t* addr = &target.back() - (value.size() + sizeof(uint16_t)) + 1;
    uint16_t size = value.size();
    memcpy(addr, &size, sizeof(uint16_t));
    addr += sizeof(uint16_t);

    memcpy(addr, value.data(), size);
}

void Int64ColumnWriter::add_value(
        std::variant<int64_t, double, std::string, bool>& value,
        size_t& size
) {
    size = sizeof(int64_t);
    m_values.push_back(std::get<int64_t>(value));
}

void Int64ColumnWriter::write_local_value(
        std::vector<uint8_t>& dest,
        size_t index,
        size_t& total_bytes
) {
    total_bytes += sizeof(int64_t);
    write_numeric_value(dest, m_values[index]);
}

void Int64ColumnWriter::combine(BaseColumnWriter* writer_base) {
    Int64ColumnWriter* writer = dynamic_cast<Int64ColumnWriter*>(writer_base);
    m_values.insert(m_values.end(), writer->m_values.begin(), writer->m_values.end());
}

void Int64ColumnWriter::store(ZstdCompressor& compressor) {
    compressor.write(
            reinterpret_cast<char const*>(m_values.data()),
            m_values.size() * sizeof(int64_t)
    );
}

void FloatColumnWriter::add_value(
        std::variant<int64_t, double, std::string, bool>& value,
        size_t& size
) {
    size = sizeof(double);
    m_values.push_back(std::get<double>(value));
}

void FloatColumnWriter::write_local_value(
        std::vector<uint8_t>& dest,
        size_t index,
        size_t& total_bytes
) {
    total_bytes += sizeof(double);
    write_numeric_value(dest, m_values[index]);
}

void FloatColumnWriter::combine(BaseColumnWriter* writer_base) {
    FloatColumnWriter* writer = dynamic_cast<FloatColumnWriter*>(writer_base);
    m_values.insert(m_values.end(), writer->m_values.begin(), writer->m_values.end());
}

void FloatColumnWriter::store(ZstdCompressor& compressor) {
    compressor.write(
            reinterpret_cast<char const*>(m_values.data()),
            m_values.size() * sizeof(double)
    );
}

void BooleanColumnWriter::add_value(
        std::variant<int64_t, double, std::string, bool>& value,
        size_t& size
) {
    size = sizeof(uint8_t);
    m_values.push_back(std::get<bool>(value) ? 1 : 0);
}

void BooleanColumnWriter::write_local_value(
        std::vector<uint8_t>& dest,
        size_t index,
        size_t& total_bytes
) {
    total_bytes += sizeof(uint8_t);
    write_numeric_value(dest, m_values[index]);
}

void BooleanColumnWriter::combine(BaseColumnWriter* writer_base) {
    BooleanColumnWriter* writer = dynamic_cast<BooleanColumnWriter*>(writer_base);
    m_values.insert(m_values.end(), writer->m_values.begin(), writer->m_values.end());
}

void BooleanColumnWriter::store(ZstdCompressor& compressor) {
    compressor.write(
            reinterpret_cast<char const*>(m_values.data()),
            m_values.size() * sizeof(uint8_t)
    );
}

void ClpStringColumnWriter::add_value(
        std::variant<int64_t, double, std::string, bool>& value,
        size_t& size
) {
    size = sizeof(int64_t);
    std::string string_var = std::get<std::string>(value);
    uint64_t id;
    uint64_t offset = m_encoded_vars.size();
    VariableEncoder::encode_and_add_to_dictionary(
            string_var,
            m_logtype_entry,
            *m_var_dict,
            m_encoded_vars
    );
    m_log_dict->add_entry(m_logtype_entry, id);
    auto encoded_id = encode_log_dict_id(id, offset);
    m_logtypes.push_back(encoded_id);
    size += sizeof(int64_t) * (m_encoded_vars.size() - offset);
}

void ClpStringColumnWriter::write_local_value(
        std::vector<uint8_t>& dest,
        size_t index,
        size_t& total_bytes
) {
    uint64_t encoded_id = m_logtypes[index];
    int32_t encoded_log_type = get_encoded_log_dict_id(encoded_id);
    int64_t encoded_offset = get_encoded_offset(encoded_id);
    int32_t size = 0;
    if (m_logtypes.size() > (index + 1)) {
        size = get_encoded_offset(m_logtypes[index + 1]) - encoded_offset;
    } else {
        size = m_encoded_vars.size() - encoded_offset;
    }
    total_bytes += sizeof(encoded_log_type);
    write_numeric_value(dest, encoded_log_type);
    if (size > 0) {
        total_bytes += sizeof(int64_t) * size;
        write_numeric_values(dest, &m_encoded_vars.data()[encoded_offset], size);
    }
}

void ClpStringColumnWriter::combine(BaseColumnWriter* writer_base) {
    ClpStringColumnWriter* writer = dynamic_cast<ClpStringColumnWriter*>(writer_base);
    size_t current_vars_size = m_encoded_vars.size();
    m_encoded_vars.insert(
            m_encoded_vars.end(),
            writer->m_encoded_vars.begin(),
            writer->m_encoded_vars.end()
    );

    m_logtypes.reserve(m_logtypes.size() + writer->m_logtypes.size());

    for (uint64_t encoded_id : writer->m_logtypes) {
        auto id = get_encoded_log_dict_id(encoded_id);
        auto offset = get_encoded_offset(encoded_id);
        m_logtypes.push_back(encode_log_dict_id(id, offset + current_vars_size));
    }
}

void ClpStringColumnWriter::store(ZstdCompressor& compressor) {
    compressor.write(
            reinterpret_cast<char const*>(m_logtypes.data()),
            m_logtypes.size() * sizeof(int64_t)
    );
    compressor.write_numeric_value(m_encoded_vars.size());
    compressor.write(
            reinterpret_cast<char const*>(m_encoded_vars.data()),
            m_encoded_vars.size() * sizeof(int64_t)
    );
}

void VariableStringColumnWriter::add_value(
        std::variant<int64_t, double, std::string, bool>& value,
        size_t& size
) {
    size = sizeof(int64_t);
    std::string string_var = std::get<std::string>(value);
    uint64_t id;
    m_var_dict->add_entry(string_var, id);
    m_variables.push_back(id);
    m_schema_node->mark_node_value(id, string_var);
}

void VariableStringColumnWriter::write_local_value(
        std::vector<uint8_t>& dest,
        size_t index,
        size_t& total_bytes
) {
    total_bytes += sizeof(int64_t);
    write_numeric_value(dest, m_variables[index]);
}

void VariableStringColumnWriter::combine(BaseColumnWriter* writer_base) {
    VariableStringColumnWriter* writer = dynamic_cast<VariableStringColumnWriter*>(writer_base);
    m_variables.insert(m_variables.end(), writer->m_variables.begin(), writer->m_variables.end());
}

void VariableStringColumnWriter::store(ZstdCompressor& compressor) {
    compressor.write(
            reinterpret_cast<char const*>(m_variables.data()),
            m_variables.size() * sizeof(int64_t)
    );
}

void DateStringColumnWriter::add_value(
        std::variant<int64_t, double, std::string, bool>& value,
        size_t& size
) {
    size = 2 * sizeof(int64_t);
    std::string string_timestamp = std::get<std::string>(value);

    uint64_t encoding_id;
    epochtime_t timestamp = m_timestamp_dict->ingest_entry(m_name, string_timestamp, encoding_id);

    m_timestamps.push_back(timestamp);
    m_timestamp_encodings.push_back(encoding_id);
}

void DateStringColumnWriter::write_local_value(
        std::vector<uint8_t>& dest,
        size_t index,
        size_t& total_bytes
) {
    total_bytes += 2 * sizeof(int64_t);
    write_numeric_value(dest, m_timestamps[index]);
    write_numeric_value(dest, m_timestamp_encodings[index]);
}

void DateStringColumnWriter::combine(BaseColumnWriter* writer_base) {
    DateStringColumnWriter* writer = dynamic_cast<DateStringColumnWriter*>(writer_base);
    m_timestamps
            .insert(m_timestamps.end(), writer->m_timestamps.begin(), writer->m_timestamps.end());
    m_timestamp_encodings.insert(
            m_timestamp_encodings.end(),
            writer->m_timestamp_encodings.begin(),
            writer->m_timestamp_encodings.end()
    );
}

void DateStringColumnWriter::store(ZstdCompressor& compressor) {
    compressor.write(
            reinterpret_cast<char const*>(m_timestamps.data()),
            m_timestamps.size() * sizeof(int64_t)
    );
    compressor.write(
            reinterpret_cast<char const*>(m_timestamp_encodings.data()),
            m_timestamp_encodings.size() * sizeof(int64_t)
    );
}

void FloatDateStringColumnWriter::add_value(
        std::variant<int64_t, double, std::string, bool>& value,
        size_t& size
) {
    size = sizeof(double);
    double timestamp = std::get<double>(value);

    m_timestamp_dict->ingest_entry(m_name, timestamp);

    m_timestamps.push_back(timestamp);
}

void FloatDateStringColumnWriter::write_local_value(
        std::vector<uint8_t>& dest,
        size_t index,
        size_t& total_bytes
) {
    total_bytes += sizeof(double);
    write_numeric_value(dest, m_timestamps[index]);
}

void FloatDateStringColumnWriter::combine(BaseColumnWriter* writer_base) {
    FloatDateStringColumnWriter* writer = dynamic_cast<FloatDateStringColumnWriter*>(writer_base);
    m_timestamps
            .insert(m_timestamps.end(), writer->m_timestamps.begin(), writer->m_timestamps.end());
}

void FloatDateStringColumnWriter::store(ZstdCompressor& compressor) {
    compressor.write(
            reinterpret_cast<char const*>(m_timestamps.data()),
            m_timestamps.size() * sizeof(double)
    );
}

void TruncatedObjectColumnWriter::merge_column(
        BaseColumnWriter* writer,
        std::shared_ptr<SchemaTree> global_tree
) {
    auto const& global_node = global_tree->get_node(writer->get_id());
    int32_t parent_id = global_node->get_parent_id();
    if (parent_id != -1 && m_global_id_to_local.find(parent_id) == m_global_id_to_local.end()) {
        if (global_tree->get_node(parent_id)->get_state() == NodeValueState::TRUNCATED) {
            merge_column(parent_id, global_tree);
        }
    }

    auto it = m_global_id_to_local.find(parent_id);
    if (it != m_global_id_to_local.end()) {
        parent_id = it->second;
    } else {
        parent_id = -1;
    }

    int32_t local_id = m_local_tree.add_node(
            parent_id,
            global_node->get_type(),
            global_node->get_key_name()
    );
    m_local_id_to_column[local_id] = writer;
}

void TruncatedObjectColumnWriter::merge_column(
        int32_t global_id,
        std::shared_ptr<SchemaTree> global_tree
) {
    auto const& global_node = global_tree->get_node(global_id);
    int32_t parent_id = global_node->get_parent_id();
    if (parent_id != -1
        && global_tree->get_node(parent_id)->get_state() == NodeValueState::TRUNCATED)
    {
        if (m_global_id_to_local.find(parent_id) == m_global_id_to_local.end()) {
            merge_column(parent_id, global_tree);
        }

        auto it = m_global_id_to_local.find(parent_id);
        int32_t new_parent_id = -1;
        if (it != m_global_id_to_local.end()) {
            new_parent_id = it->second;
        }
        m_local_tree.add_node(new_parent_id, global_node->get_type(), global_node->get_key_name());
    }
}

void TruncatedObjectColumnWriter::merge_null_column(
        int32_t id,
        std::shared_ptr<SchemaTree> global_tree
) {
    auto const& global_node = global_tree->get_node(id);
    int32_t parent_id = global_node->get_parent_id();
    if (parent_id != -1 && m_global_id_to_local.find(parent_id) == m_global_id_to_local.end()) {
        if (global_tree->get_node(parent_id)->get_state() == NodeValueState::TRUNCATED) {
            merge_column(parent_id, global_tree);
        }
    }

    auto it = m_global_id_to_local.find(parent_id);
    if (it != m_global_id_to_local.end()) {
        parent_id = it->second;
    } else {
        parent_id = -1;
    }

    int32_t local_id = m_local_tree.add_node(
            parent_id,
            global_node->get_type(),
            global_node->get_key_name()
    );
    m_local_id_to_column[local_id] = nullptr;
}

void TruncatedObjectColumnWriter::merge_object_column(
        int32_t id,
        std::shared_ptr<SchemaTree> global_tree
) {
    auto const& global_node = global_tree->get_node(id);
    int32_t parent_id = global_node->get_parent_id();
    bool parent_truncated = false;
    if (parent_id != -1 && m_global_id_to_local.find(parent_id) == m_global_id_to_local.end()) {
        if (global_tree->get_node(parent_id)->get_state() == NodeValueState::TRUNCATED) {
            merge_column(parent_id, global_tree);
            parent_truncated = true;
        }
    }

    auto it = m_global_id_to_local.find(parent_id);
    if (it != m_global_id_to_local.end()) {
        parent_id = it->second;
    } else {
        parent_id = -1;
    }

    if (parent_truncated) {
        int32_t local_id = m_local_tree.add_node(
                parent_id,
                global_node->get_type(),
                global_node->get_key_name()
        );
    }
}

void TruncatedObjectColumnWriter::local_merge_column_values(uint64_t num_messages) {
    m_schemas.resize(num_messages);
    m_values.resize(num_messages);
    std::vector<uint8_t> schema({0, 0});

    std::set<int32_t> visited;
    for (auto const& node : m_local_tree.get_nodes()) {
        visit(schema, visited, node);
    }

    for (std::vector<uint8_t>& s : m_schemas) {
        s = schema;
        m_num_bytes += schema.size();
    }

    memcpy(schema.data(), &m_num_nodes, sizeof(m_num_nodes));
}

void TruncatedObjectColumnWriter::visit(
        std::vector<uint8_t>& schema,
        std::set<int32_t>& visited,
        std::shared_ptr<SchemaNode> const& node
) {
    if (visited.count(node->get_id())) {
        return;
    }

    visited.insert(node->get_id());

    // this assumes that each subtree is at most 65536 nodes at a time,
    // which is probably unsafe, but good enough for now
    m_num_nodes += 1;
    uint8_t node_type = static_cast<uint8_t>(node->get_type());
    schema.push_back(node_type);
    write_string(schema, node->get_key_name());

    if (node->get_type() != NodeType::OBJECT) {
        size_t idx = 0;
        BaseColumnWriter* old_writer = m_local_id_to_column.at(node->get_id());
        for (auto it = m_values.begin(); it != m_values.end(); ++it) {
            if (old_writer != nullptr) {
                old_writer->write_local_value(*it, idx, m_num_bytes);
            }
            ++idx;
        }
    }

    for (int32_t child : node->get_children_ids()) {
        visit(schema, visited, m_local_tree.get_node(child));
    }
}

void TruncatedObjectColumnWriter::combine(BaseColumnWriter* writer_base) {
    TruncatedObjectColumnWriter* writer = dynamic_cast<TruncatedObjectColumnWriter*>(writer_base);
    m_values.splice(m_values.end(), writer->m_values);
    m_schemas.splice(m_schemas.end(), writer->m_schemas);
    m_num_bytes += writer->m_num_bytes;
}

void TruncatedObjectColumnWriter::store(ZstdCompressor& compressor) {
    compressor.write_numeric_value(m_num_bytes);
    auto sit = m_schemas.begin();
    auto vit = m_values.begin();
    for (; sit != m_schemas.end();) {
        compressor.write((char const*)(*sit).data(), (*sit).size());
        compressor.write((char const*)(*vit).data(), (*vit).size());
        ++sit;
        ++vit;
    }
}

}  // namespace clp_s
