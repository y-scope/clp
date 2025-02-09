#ifndef CLP_S_JSONSERIALIZER_HPP
#define CLP_S_JSONSERIALIZER_HPP

#include <string>
#include <string_view>
#include <vector>

#include "ColumnReader.hpp"
#include "Utils.hpp"

namespace clp_s {

class JsonSerializer {
public:
    enum Op : uint8_t {
        BeginObject,
        EndObject,
        AddIntField,
        AddFloatField,
        AddBoolField,
        AddStringField,
        AddArrayField,
        AddNullField,
        AddIntValue,
        AddFloatValue,
        AddBoolValue,
        AddStringValue,
        AddNullValue,
        BeginArray,
        EndArray,
        BeginUnnamedObject,
        BeginUnnamedArray,
    };

    static int64_t const cReservedLength = 4096;

    explicit JsonSerializer(int64_t reserved_length = cReservedLength) {
        m_json_string.reserve(cReservedLength);
    }

    std::string& get_serialized_string() { return m_json_string; }

    /**
     * Resets the JsonSerializer for the next record.
     */
    void reset() {
        m_json_string.clear();
        m_op_list_index = 0;
        m_special_keys_index = 0;
    }

    /**
     * Clears the contents of the JsonSerializer to make room for a new set of operations.
     */
    void clear() {
        reset();
        m_op_list.clear();
        m_special_keys.clear();
    }

    void add_op(Op op) { m_op_list.push_back(op); }

    std::vector<Op>& get_op_list() { return m_op_list; }

    bool get_next_op(Op& op) {
        if (m_op_list_index < m_op_list.size()) {
            op = m_op_list[m_op_list_index++];
            return true;
        }
        return false;
    }

    void add_special_key(std::string_view const key) {
        std::string tmp;
        StringUtils::escape_json_string(tmp, key);
        m_special_keys.emplace_back(tmp);
    }

    void begin_object() {
        append_key();
        m_json_string += "{";
    }

    void begin_document() { m_json_string += "{"; }

    void end_document() {
        if ('{' != m_json_string.back()) {
            m_json_string[m_json_string.size() - 1] = '}';
        } else {
            m_json_string += '}';
        }
    }

    void end_object() {
        if (m_op_list[m_op_list_index - 2] != BeginObject
            && m_op_list[m_op_list_index - 2] != BeginUnnamedObject)
        {
            m_json_string.pop_back();
        }
        m_json_string += "},";
    }

    void begin_array_document() { m_json_string += "["; }

    void begin_array() {
        append_key();
        m_json_string += "[";
    }

    void end_array() {
        if (m_op_list[m_op_list_index - 2] != BeginArray
            && m_op_list[m_op_list_index - 2] != BeginUnnamedArray)
        {
            m_json_string.pop_back();
        }
        m_json_string += "],";
    }

    void append_key() { append_escaped_key(m_special_keys[m_special_keys_index++]); }

    void append_key(std::string_view const key) {
        m_json_string += "\"";
        StringUtils::escape_json_string(m_json_string, key);
        m_json_string += "\":";
    }

    void append_value(std::string_view const value) {
        m_json_string += value;
        m_json_string += ",";
    }

    void append_value_from_column(clp_s::BaseColumnReader* column, uint64_t cur_message) {
        column->extract_string_value_into_buffer(cur_message, m_json_string);
        m_json_string += ",";
    }

    void
    append_value_from_column_with_quotes(clp_s::BaseColumnReader* column, uint64_t cur_message) {
        m_json_string += "\"";
        column->extract_escaped_string_value_into_buffer(cur_message, m_json_string);
        m_json_string += "\",";
    }

private:
    void append_escaped_key(std::string_view const key) {
        m_json_string.push_back('"');
        m_json_string.append(key);
        m_json_string.append("\":");
    }

    std::string m_json_string;
    std::vector<Op> m_op_list;
    std::vector<std::string> m_special_keys;

    size_t m_op_list_index{0};
    size_t m_special_keys_index{0};
};

}  // namespace clp_s

#endif  // CLP_S_JSONSERIALIZER_HPP
