#include "Message.hpp"

namespace clp::streaming_archive::reader {
size_t Message::get_message_number() const {
    return m_message_number;
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

void Message::set_message_number(uint64_t message_number) {
    m_message_number = message_number;
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
