#include "Message.hpp"

namespace clp::streaming_archive::reader {
auto Message::get_log_event_ix() const -> size_t {
    return m_log_event_ix;
}

auto Message::get_ix_in_file_split() const -> size_t {
    return m_ix_in_file_split;
}

logtype_dictionary_id_t Message::get_logtype_id() const {
    return m_logtype_id;
}

std::vector<encoded_variable_t> const& Message::get_vars() const {
    return m_vars;
}

epochtime_t Message::get_ts_in_milli() const {
    return m_timestamp;
}

auto Message::set_msg_ix(uint64_t file_split_begin_msg_ix, uint64_t msg_ix_in_file_split) -> void {
    m_ix_in_file_split = msg_ix_in_file_split;
    m_log_event_ix = m_ix_in_file_split + file_split_begin_msg_ix;
}

void Message::set_logtype_id(logtype_dictionary_id_t logtype_id) {
    m_logtype_id = logtype_id;
}

void Message::add_var(encoded_variable_t var) {
    m_vars.push_back(var);
}

void Message::set_timestamp(epochtime_t timestamp) {
    m_timestamp = timestamp;
}

void Message::clear_vars() {
    m_vars.clear();
}
}  // namespace clp::streaming_archive::reader
