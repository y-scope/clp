#include "ParsedMessage.hpp"

using std::string;

namespace glt {
void ParsedMessage::clear() {
    m_ts_patt = nullptr;
    clear_except_ts_patt();
}

void ParsedMessage::clear_except_ts_patt() {
    m_ts_patt_changed = false;
    m_ts = 0;
    m_content.clear();
    m_orig_num_bytes = 0;
    m_is_set = false;
}

void ParsedMessage::set(
        TimestampPattern const* timestamp_pattern,
        epochtime_t const timestamp,
        string const& line,
        size_t timestamp_begin_pos,
        size_t timestamp_end_pos
) {
    if (timestamp_pattern != m_ts_patt) {
        m_ts_patt = timestamp_pattern;
        m_ts_patt_changed = true;
    }
    m_ts = timestamp;
    if (timestamp_begin_pos == timestamp_end_pos) {
        m_content.assign(line);
    } else {
        m_content.assign(line, 0, timestamp_begin_pos);
        m_content.append(line, timestamp_end_pos, string::npos);
    }
    m_orig_num_bytes = line.length();
    m_is_set = true;
}

void ParsedMessage::append_line(string const& line) {
    m_content += line;
    m_orig_num_bytes += line.length();
}

void ParsedMessage::consume(ParsedMessage& message) {
    if (message.m_ts_patt != m_ts_patt) {
        m_ts_patt = message.m_ts_patt;
        m_ts_patt_changed = true;
    }
    m_ts = message.m_ts;
    m_content.swap(message.m_content);
    m_orig_num_bytes = message.m_orig_num_bytes;
    m_is_set = true;

    message.clear();
}
}  // namespace glt
