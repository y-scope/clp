#ifndef CLP_PARSEDMESSAGE_HPP
#define CLP_PARSEDMESSAGE_HPP

#include <string>

#include "TimestampPattern.hpp"

namespace clp {
/**
 * ParsedMessage represents a (potentially multiline) log message parsed into 3 primary fields:
 * timestamp, timestamp pattern, and content.
 */
class ParsedMessage {
public:
    // Constructors
    ParsedMessage()
            : m_ts_patt(nullptr),
              m_ts_patt_changed(false),
              m_ts(0),
              m_content({}),
              m_orig_num_bytes(0),
              m_is_set(false) {}

    // Disable copy and move constructor/assignment
    ParsedMessage(ParsedMessage const&) = delete;
    ParsedMessage& operator=(ParsedMessage const&) = delete;

    // Destructors
    ~ParsedMessage() = default;

    // Methods
    void clear();
    void clear_except_ts_patt();

    void set(
            TimestampPattern const* timestamp_pattern,
            epochtime_t timestamp,
            std::string const& line,
            size_t timestamp_begin_pos,
            size_t timestamp_end_pos
    );
    void append_line(std::string const& line);

    /**
     * Move all data from the given message into the current message while clearing the given
     * message
     * @param message
     */
    void consume(ParsedMessage& message);

    std::string const& get_content() const { return m_content; }

    size_t get_orig_num_bytes() const { return m_orig_num_bytes; }

    epochtime_t get_ts() const { return m_ts; }

    TimestampPattern const* get_ts_patt() const { return m_ts_patt; }

    bool has_ts_patt_changed() const { return m_ts_patt_changed; }

    bool is_empty() const { return false == m_is_set; }

private:
    // Variables
    TimestampPattern const* m_ts_patt;
    bool m_ts_patt_changed;
    epochtime_t m_ts;
    std::string m_content;
    size_t m_orig_num_bytes;
    bool m_is_set;
};
}  // namespace clp

#endif  // CLP_PARSEDMESSAGE_HPP
