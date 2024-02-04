#ifndef CLP_S_PARSEDMESSAGE_HPP
#define CLP_S_PARSEDMESSAGE_HPP

#include <map>
#include <string>
#include <utility>
#include <variant>

#include "Defs.hpp"

namespace clp_s {
class ParsedMessage {
public:
    // Types
    using variable_t
            = std::variant<int64_t, double, std::string, bool, std::pair<uint64_t, epochtime_t>>;

    // Constructor
    ParsedMessage() : m_schema_id(-1) {}

    // Destructor
    ~ParsedMessage() = default;

    void set_id(int32_t schema_id) { m_schema_id = schema_id; }

    /**
     * Adds an int64_t value to the message for a given MST node ID.
     * @param node_id
     * @param value
     */
    inline void add_value(int32_t node_id, int64_t value) { m_message.emplace(node_id, value); }

    /**
     * Adds a double value to the message for a given MST node ID.
     * @param node_id
     * @param value
     */
    inline void add_value(int32_t node_id, double value) { m_message.emplace(node_id, value); }

    /**
     * Adds a string value to the message for a given MST node ID.
     * @param node_id
     * @param value
     */
    inline void add_value(int32_t node_id, std::string const& value) {
        m_message.emplace(node_id, value);
    }

    /**
     * Adds a boolean value to the message for a given MST node ID.
     * @param node_id
     * @param value
     */
    inline void add_value(int32_t node_id, bool value) { m_message.emplace(node_id, value); }

    /**
     * Adds a timestamp value and its encoding to the message for a given MST node ID.
     * @param node_id
     * @param value
     */
    inline void add_value(int32_t node_id, uint64_t encoding_id, epochtime_t value) {
        m_message.emplace(node_id, std::make_pair(encoding_id, value));
    }

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
    std::map<int32_t, variable_t>& get_content() { return m_message; }

private:
    int32_t m_schema_id;
    std::map<int32_t, variable_t> m_message;
};
}  // namespace clp_s

#endif  // CLP_S_PARSEDMESSAGE_HPP
