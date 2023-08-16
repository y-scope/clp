#include "ParsedIrMessage.hpp"

#include <spdlog/spdlog.h>

#include "LogTypeDictionaryEntry.hpp"

using std::string;

auto ParsedIrMessage::set_ts(epochtime_t ts) -> void {
    m_ts = ts;
    if (ts != 0) {
        m_orig_num_bytes += m_ts_bytes;
    }
}

auto ParsedIrMessage::set_ts_pattern(TimestampPattern const* timestamp_pattern) -> void {
    if (m_ts_patt != nullptr) {
        SPDLOG_ERROR("Can not set different timestamp for an IR file");
        throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
    }
    m_ts_patt = timestamp_pattern;
    // get a rough estimation of ts string size
    string empty_msg;
    m_ts_patt->insert_formatted_timestamp(0, empty_msg);
    m_ts_bytes = empty_msg.length();
}

auto ParsedIrMessage::append_to_logtype(string const& value, size_t begin_pos, size_t length)
        -> void {
    m_logtype_entry.add_constant(value, begin_pos, length);
    m_orig_num_bytes += length;
}

auto ParsedIrMessage::clear() -> void {
    m_ts_patt = nullptr;
    m_ts_bytes = 0;
    clear_except_ts_patt();
}

auto ParsedIrMessage::clear_except_ts_patt() -> void {
    m_variables.clear();
    m_orig_num_bytes = 0;
    m_ts = 0;
    m_logtype_entry.clear();
}

auto ParsedIrMessage::add_dictionary_var(string const& dictionary_var) -> void {
    m_variables.emplace_back(dictionary_var);
    m_logtype_entry.add_dictionary_var();
    m_orig_num_bytes += dictionary_var.size();
}

auto ParsedIrMessage::add_encoded_integer(encoded_variable_t var, size_t orginal_size_in_bytes)
        -> void {
    m_variables.emplace_back(var);
    m_logtype_entry.add_int_var();
    m_orig_num_bytes += orginal_size_in_bytes;
}

auto ParsedIrMessage::add_encoded_float(encoded_variable_t var, size_t orginal_size_in_bytes)
        -> void {
    m_variables.emplace_back(var);
    m_logtype_entry.add_float_var();
    m_orig_num_bytes += orginal_size_in_bytes;
}
