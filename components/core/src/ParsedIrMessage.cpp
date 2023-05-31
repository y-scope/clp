#include "ParsedIrMessage.hpp"

// C standard libraries

// C++ standard libraries

// Project headers
#include "type_utils.hpp"
#include "LogTypeDictionaryEntry.hpp"

// spdlog
#include "spdlog/spdlog.h"

void ParsedIrMessage::set_ts (epochtime_t ts) {
    m_ts = ts;
}

void ParsedIrMessage::set_ts_pattern (const TimestampPattern* timestamp_pattern) {
    if (m_ts_patt != nullptr) {
        SPDLOG_ERROR("Can not set different timestamp for an IR file");
        throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
    }
    m_ts_patt = timestamp_pattern;
}

void ParsedIrMessage::append_to_logtype (const char* begin, size_t length) {
    m_logtype.append(begin, length);
    m_orig_num_bytes += length;
}

void ParsedIrMessage::clear () {
    m_ts_patt = nullptr;
    clear_except_ts_patt();
}

void ParsedIrMessage::clear_except_ts_patt () {
    m_variables.clear();
    m_var_positions.clear();
    m_orig_num_bytes = 0;
    m_logtype.clear();
}

void ParsedIrMessage::add_dictionary_var (std::string_view dictionary_var) {
    m_variables.emplace_back(dictionary_var);
    m_var_positions.push_back(m_logtype.length());
    m_logtype += enum_to_underlying_type(LogTypeDictionaryEntry::VarDelim::Dictionary);
    m_orig_num_bytes += dictionary_var.size();
}

void ParsedIrMessage::add_encoded_integer (encoded_variable_t var, size_t orginal_size_in_bytes) {
    m_variables.emplace_back(var);
    m_var_positions.push_back(m_logtype.length());
    m_logtype += enum_to_underlying_type(LogTypeDictionaryEntry::VarDelim::Integer);
    m_orig_num_bytes += orginal_size_in_bytes;
}

void ParsedIrMessage::add_encoded_float (encoded_variable_t var, size_t orginal_size_in_bytes) {
    m_variables.emplace_back(var);
    m_var_positions.push_back(m_logtype.length());
    m_logtype += enum_to_underlying_type(LogTypeDictionaryEntry::VarDelim::Float);
    m_orig_num_bytes += orginal_size_in_bytes;
}
