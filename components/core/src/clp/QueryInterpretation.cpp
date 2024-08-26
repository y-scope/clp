#include "QueryInterpretation.hpp"

#include <utility>

#include "EncodedVariableInterpreter.hpp"
#include "LogTypeDictionaryEntry.hpp"
#include "Utils.hpp"

using clp::string_utils::clean_up_wildcard_search_string;
using log_surgeon::lexers::ByteLexer;

namespace clp {

SearchString::SearchString(std::string processed_search_string)
        : m_processed_search_string(std::move(processed_search_string)) {
    // TODO: remove this when subqueries can handle '?' wildcards
    // Replace '?' wildcards with '*' wildcards since we currently have no support for
    // generating sub-queries with '?' wildcards. The final wildcard match on the decompressed
    // message uses the original wildcards, so correctness will be maintained.
    std::ranges::replace(m_processed_search_string, '?', '*');
    // Clean-up in case any instances of "?*" or "*?" were changed into "**"
    m_processed_search_string = clean_up_wildcard_search_string(m_processed_search_string);
    m_is_greedy_wildcard.reserve(m_processed_search_string.size());
    m_is_non_greedy_wildcard.reserve(m_processed_search_string.size());
    m_is_escape.reserve(m_processed_search_string.size());
    bool is_escaped = false;
    for (auto const& c : m_processed_search_string) {
        if (is_escaped) {
            m_is_greedy_wildcard.push_back(false);
            m_is_non_greedy_wildcard.push_back(false);
            m_is_escape.push_back(false);
            is_escaped = false;
        } else {
            if ('\\' == c) {
                m_is_greedy_wildcard.push_back(false);
                m_is_non_greedy_wildcard.push_back(false);
                m_is_escape.push_back(true);
                is_escaped = true;
            } else if ('*' == c) {
                m_is_greedy_wildcard.push_back(true);
                m_is_non_greedy_wildcard.push_back(false);
                m_is_escape.push_back(false);
            } else if ('?' == c) {
                m_is_greedy_wildcard.push_back(false);
                m_is_non_greedy_wildcard.push_back(true);
                m_is_escape.push_back(false);
            } else {
                m_is_greedy_wildcard.push_back(false);
                m_is_non_greedy_wildcard.push_back(false);
                m_is_escape.push_back(false);
            }
        }
    }
}

void SearchStringView::extend_to_adjacent_wildcards() {
    bool const prev_char_is_star = m_begin_idx > 0 && m_is_greedy_wildcard[m_begin_idx - 1];
    bool const next_char_is_greedy_wildcard
            = m_end_idx < m_processed_search_string.length() && m_is_greedy_wildcard[m_end_idx];
    if (prev_char_is_star) {
        m_begin_idx--;
    }
    if (next_char_is_greedy_wildcard) {
        m_end_idx++;
    }
}

bool SearchStringView::surrounded_by_delims(log_surgeon::lexers::ByteLexer const& lexer) const {
    // Preceding delimiter counts the start of log, a wildcard, or an actual delimiter.
    bool const has_preceding_delimiter
            = m_begin_idx == 0 || m_is_greedy_wildcard[m_begin_idx - 1]
              || m_is_non_greedy_wildcard[m_begin_idx - 1]
              || lexer.is_delimiter(m_processed_search_string[m_begin_idx - 1]);

    // Proceeding delimiter counts the end of log, a wildcard, or an actual delimiter. However,
    // we have to be careful about a proceeding escape character. First, if '\' is a delimiter,
    // we avoid counting the escape character. Second, if a literal '*' or '?' is a delimiter,
    // then it will appear after the escape character.
    bool const has_proceeding_delimiter
            = m_processed_search_string.size() == m_end_idx || m_is_greedy_wildcard[m_end_idx]
              || m_is_non_greedy_wildcard[m_end_idx]
              || (false == m_is_escape[m_end_idx]
                  && lexer.is_delimiter(m_processed_search_string[m_end_idx]))
              || (m_is_escape[m_end_idx]
                  && lexer.is_delimiter(m_processed_search_string[m_end_idx + 1]));
    return has_preceding_delimiter && has_proceeding_delimiter;
}

void StaticQueryToken::append(StaticQueryToken const& rhs) {
    m_query_substring += rhs.get_query_substring();
}

void QueryInterpretation::append_logtype(QueryInterpretation& suffix) {
    auto const& first_new_token = suffix.m_logtype[0];
    if (auto& prev_token = m_logtype.back();
        false == m_logtype.empty() && std::holds_alternative<StaticQueryToken>(prev_token)
        && false == suffix.m_logtype.empty()
        && std::holds_alternative<StaticQueryToken>(first_new_token))
    {
        std::get<StaticQueryToken>(prev_token).append(std::get<StaticQueryToken>(first_new_token));
        m_logtype.insert(m_logtype.end(), suffix.m_logtype.begin() + 1, suffix.m_logtype.end());
    } else {
        m_logtype.insert(m_logtype.end(), suffix.m_logtype.begin(), suffix.m_logtype.end());
    }
}

void QueryInterpretation::generate_logtype_string(ByteLexer& lexer) {
    // Convert each query logtype into a set of logtype strings. Logtype strings are used in the
    // sub query as they have the correct format for comparing against the archive. Also, a
    // single query logtype might represent multiple logtype strings. While static text converts
    // one-to-one, wildcard variables that may be encoded have different logtype strings when
    // comparing against the dictionary than they do when comparing against the segment.
    // TODO: Can m_logtype_string be reserved?
    for (uint32_t i = 0; i < get_logtype_size(); i++) {
        if (auto const& logtype_token = get_logtype_token(i);
            std::holds_alternative<StaticQueryToken>(logtype_token))
        {
            m_logtype_string += std::get<StaticQueryToken>(logtype_token).get_query_substring();
        } else {
            auto const& variable_token = std::get<VariableQueryToken>(logtype_token);
            auto const variable_type = variable_token.get_variable_type();
            auto const& raw_string = variable_token.get_query_substring();
            auto const is_encoded_with_wildcard = variable_token.get_is_encoded_with_wildcard();
            auto const var_has_wildcard = variable_token.get_has_wildcard();
            auto& schema_type = lexer.m_id_symbol[variable_type];
            encoded_variable_t encoded_var;
            if (is_encoded_with_wildcard) {
                if (cIntVarName == schema_type) {
                    LogTypeDictionaryEntry::add_int_var(m_logtype_string);
                } else if (cFloatVarName == schema_type) {
                    LogTypeDictionaryEntry::add_float_var(m_logtype_string);
                }
            } else if (false == var_has_wildcard && cIntVarName == schema_type
                       && EncodedVariableInterpreter::convert_string_to_representable_integer_var(
                               raw_string,
                               encoded_var
                       ))
            {
                LogTypeDictionaryEntry::add_int_var(m_logtype_string);
            } else if (false == var_has_wildcard && cFloatVarName == schema_type
                       && EncodedVariableInterpreter::convert_string_to_representable_float_var(
                               raw_string,
                               encoded_var
                       ))
            {
                LogTypeDictionaryEntry::add_float_var(m_logtype_string);
            } else {
                LogTypeDictionaryEntry::add_dict_var(m_logtype_string);
            }
        }
    }
}

bool QueryInterpretation::operator<(QueryInterpretation const& rhs) const {
    if (m_logtype.size() < rhs.m_logtype.size()) {
        return true;
    }
    if (m_logtype.size() > rhs.m_logtype.size()) {
        return false;
    }
    for (uint32_t i = 0; i < m_logtype.size(); i++) {
        if (m_logtype[i] < rhs.m_logtype[i]) {
            return true;
        }
        if (m_logtype[i] > rhs.m_logtype[i]) {
            return false;
        }
    }
    return false;
}

std::ostream& operator<<(std::ostream& os, QueryInterpretation const& query_logtype) {
    os << "\"";
    for (uint32_t idx = 0; idx < query_logtype.get_logtype_size(); idx++) {
        if (auto const& query_token = query_logtype.get_logtype_token(idx);
            std::holds_alternative<StaticQueryToken>(query_token))
        {
            os << std::get<StaticQueryToken>(query_token).get_query_substring();
        } else {
            auto const& variable_token = std::get<VariableQueryToken>(query_token);
            os << "<" << variable_token.get_variable_type() << ">("
               << variable_token.get_query_substring() << ")";
        }
    }
    os << "\"(";
    for (uint32_t idx = 0; idx < query_logtype.get_logtype_size(); idx++) {
        if (auto const& query_token = query_logtype.get_logtype_token(idx);
            std::holds_alternative<StaticQueryToken>(query_token))
        {
            os << 0;
        } else {
            auto const& variable_token = std::get<VariableQueryToken>(query_token);
            os << variable_token.get_has_wildcard();
        }
    }
    os << ")(";
    for (uint32_t idx = 0; idx < query_logtype.get_logtype_size(); idx++) {
        if (auto const& query_token = query_logtype.get_logtype_token(idx);
            std::holds_alternative<StaticQueryToken>(query_token))
        {
            os << 0;
        } else {
            auto const& variable_token = std::get<VariableQueryToken>(query_token);
            os << variable_token.get_is_encoded_with_wildcard();
        }
    }
    os << ")(" << query_logtype.get_logtype_string() << ")";
    return os;
}
}  // namespace clp
