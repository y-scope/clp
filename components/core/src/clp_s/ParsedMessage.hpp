#ifndef CLP_S_PARSEDMESSAGE_HPP
#define CLP_S_PARSEDMESSAGE_HPP

#include <map>
#include <string>
#include <utility>
#include <variant>

namespace clp_s {
class ParsedMessage {
public:
    // Constructor
    ParsedMessage() : m_schema_id(-1) {}

    // Destructor
    ~ParsedMessage() = default;

    void set_id(int32_t schema_id) { m_schema_id = schema_id; }

    /**
     * Adds a value with different types to the message
     * @param node_id
     * @param value
     */
    inline void add_value(int32_t node_id, int64_t value) { m_message[node_id] = value; }

    inline void add_value(int32_t node_id, double value) { m_message[node_id] = value; }

    inline void add_value(int32_t node_id, std::string const& value) { m_message[node_id] = value; }

    inline void add_value(int32_t node_id, bool value) { m_message[node_id] = value; }

    /**
     * Clears the message
     */
    void clear() {
        m_schema_id = -1;
        m_message.clear();
    }

    /**
     * @return The content of the message
     */
    std::map<int32_t, std::variant<int64_t, double, std::string, bool>>& get_content() {
        return m_message;
    }

private:
    int32_t m_schema_id;
    std::map<int32_t, std::variant<int64_t, double, std::string, bool>> m_message;
};
}  // namespace clp_s

#endif  // CLP_S_PARSEDMESSAGE_HPP
