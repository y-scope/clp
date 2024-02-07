#ifndef CLP_S_JSONSERIALIZER_HPP
#define CLP_S_JSONSERIALIZER_HPP

#include <string>
#include <vector>

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
    };

    static int64_t const cReservedLength = 4096;

    explicit JsonSerializer(int64_t reserved_length = cReservedLength) : m_special_keys_index(0) {
        m_json_string.reserve(cReservedLength);
    }

    std::string& get_serialized_string() { return m_json_string; }

    void reset() {
        m_json_string.clear();
        m_op_list_index = 0;
        m_special_keys_index = 0;
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

    void add_special_key(std::string const& key) { m_special_keys.push_back(key); }

    void begin_object() {
        append_key();
        m_json_string += "{";
    }

    void begin_document() { m_json_string += "{"; }

    void end_document() { m_json_string[m_json_string.size() - 1] = '}'; }

    void end_object() {
        if (m_op_list[m_op_list_index - 2] != BeginObject) {
            m_json_string.pop_back();
        }
        m_json_string += "},";
    }

    void append_key() { append_key(m_special_keys[m_special_keys_index++]); }

    void append_key(std::string const& key) { m_json_string += "\"" + key + "\":"; }

    void append_value(std::string const& value) { m_json_string += value + ","; }

    void append_value_with_quotes(std::string const& value) {
        m_json_string += "\"" + value + "\",";
    }

private:
    std::string m_json_string;
    std::vector<Op> m_op_list;
    std::vector<std::string> m_special_keys;

    size_t m_op_list_index;
    size_t m_special_keys_index;
};

#endif  // CLP_S_JSONSERIALIZER_HPP
