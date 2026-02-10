#ifndef CLP_S_PARSEDMESSAGE_HPP
#define CLP_S_PARSEDMESSAGE_HPP

#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include <clp/Defs.h>
#include <clp/ffi/EncodedTextAst.hpp>
#include <clp_s/Defs.hpp>
#include <clp_s/DictionaryEntry.hpp>
#include <clp_s/FloatFormatEncoding.hpp>

namespace clp_s {
class ParsedMessage {
public:
    /**
     * Tracks building up a ClpString node during parsing a log event.
     */
    struct ClpString {
        clp_s::LogTypeDictionaryEntry m_logtype;
        std::vector<clp::encoded_variable_t> m_encoded_vars;
        std::vector<std::string_view> m_var_type_names;
    };

    using variable_t = std::
            variant<int64_t,
                    double,
                    std::string,
                    clp::ffi::EightByteEncodedTextAst,
                    clp::ffi::FourByteEncodedTextAst,
                    bool,
                    std::pair<uint64_t, epochtime_t>,
                    std::pair<epochtime_t, uint64_t>,
                    std::pair<double, float_format_t>,
                    ClpString>;

    void set_id(int32_t schema_id) { m_schema_id = schema_id; }

    /**
     * Adds a value to the message for a given MST node ID.
     * @tparam T
     * @param node_id
     * @param value
     */
    template <typename T>
    inline void add_value(int32_t node_id, T const& value) {
        m_message.emplace(node_id, value);
    }

    inline void add_value(int32_t node_id, std::string_view value) {
        m_message.emplace(node_id, std::string{value});
    }

    /**
     * Adds a timestamp value and its encoding to the message for a given MST node ID.
     * @param node_id
     * @param encoding_id
     * @param value
     */
    inline void add_value(int32_t node_id, uint64_t encoding_id, epochtime_t value) {
        m_message.emplace(node_id, std::make_pair(encoding_id, value));
    }

    /**
     * Adds a float and its format to the message for a given MST node ID.
     * @param node_id
     * @param value
     * @param format
     */
    inline void add_value(int32_t node_id, double value, float_format_t format) {
        m_message.emplace(node_id, std::make_pair(value, format));
    }

    /**
     * Adds a value to the unordered region of the message. The order in which unordered values are
     * added to the message must match the order in which the corresponding MST node IDs are added
     * to the unordered region of the schema.
     * @param value
     */
    template <typename T>
    inline void add_unordered_value(T const& value) {
        m_unordered_message.emplace_back(value);
    }

    inline void add_unordered_value(std::string_view value) {
        m_unordered_message.emplace_back(std::string{value});
    }

    /**
     * Adds a float and its format to the unordered region of the message.
     * @param node_id
     * @param value
     * @param format
     */
    inline void add_unordered_value(double value, float_format_t format) {
        m_unordered_message.emplace_back(std::make_pair(value, format));
    }

    /**
     * Clears the message
     */
    void clear() {
        m_schema_id = -1;
        m_message.clear();
        m_unordered_message.clear();
    }

    /**
     * @return The content of the message
     */
    std::map<int32_t, variable_t>& get_content() { return m_message; }

    /**
     * @return the unordered content of the message
     */
    std::vector<variable_t>& get_unordered_content() { return m_unordered_message; }

private:
    int32_t m_schema_id{-1};
    std::map<int32_t, variable_t> m_message;
    std::vector<variable_t> m_unordered_message;
};
}  // namespace clp_s

#endif  // CLP_S_PARSEDMESSAGE_HPP
