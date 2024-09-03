#include "QueryInterpretation.hpp"

#include <algorithm>
#include <cstdint>
#include <ostream>
#include <string>
#include <utility>
#include <variant>

#include "Defs.h"
#include "EncodedVariableInterpreter.hpp"
#include "log_surgeon/Lexer.hpp"
#include "LogTypeDictionaryEntry.hpp"
#include "string_utils/string_utils.hpp"

using clp::string_utils::clean_up_wildcard_search_string;
using log_surgeon::lexers::ByteLexer;

namespace clp {
WildcardExpression::WildcardExpression(std::string processed_search_string)
        : m_processed_search_string(std::move(processed_search_string)) {
    // TODO: remove this when subqueries can handle '?' wildcards
    // Replace '?' wildcards with '*' wildcards since we currently have no support for
    // generating sub-queries with '?' wildcards. The final wildcard match on the decompressed
    // message uses the original wildcards, so correctness will be maintained.
    std::replace(m_processed_search_string.begin(), m_processed_search_string.end(), '?', '*');

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

auto WildcardExpressionView::extend_to_adjacent_greedy_wildcards() const -> WildcardExpressionView {
    auto extended_view = *this;
    bool const prev_char_is_greedy_wildcard
            = m_begin_idx > 0 && m_search_string_ptr->get_value_is_greedy_wildcard(m_begin_idx - 1);
    if (prev_char_is_greedy_wildcard) {
        extended_view.m_begin_idx--;
    }
    bool const next_char_is_greedy_wildcard
            = m_end_idx < m_search_string_ptr->length()
              && m_search_string_ptr->get_value_is_greedy_wildcard(m_end_idx);
    if (next_char_is_greedy_wildcard) {
        ++extended_view.m_end_idx;
    }
    return extended_view;
}

auto WildcardExpressionView::surrounded_by_delims_or_wildcards(ByteLexer const& lexer
) const -> bool {
    bool has_preceding_delim{};
    if (0 == m_begin_idx) {
        has_preceding_delim = true;
    } else {
        bool const preceded_by_greedy_wildcard
                = m_search_string_ptr->get_value_is_greedy_wildcard(m_begin_idx - 1);
        bool const preceded_by_non_greedy_wildcard
                = m_search_string_ptr->get_value_is_non_greedy_wildcard(m_begin_idx - 1);
        bool const preceded_by_delimiter
                = lexer.is_delimiter(m_search_string_ptr->get_value(m_begin_idx - 1));
        has_preceding_delim = preceded_by_greedy_wildcard || preceded_by_non_greedy_wildcard
                              || preceded_by_delimiter;
    }

    bool has_succeeding_delim{};
    if (m_search_string_ptr->length() == m_end_idx) {
        has_succeeding_delim = true;
    } else {
        bool const succeeded_by_greedy_wildcard
                = m_search_string_ptr->get_value_is_greedy_wildcard(m_end_idx);
        bool const succeeded_by_non_greedy_wildcard
                = m_search_string_ptr->get_value_is_non_greedy_wildcard(m_end_idx);
        // E.g. "foo:", where ':' is a delimiter
        bool const succeeded_by_unescaped_delim
                = false == m_search_string_ptr->get_value_is_escape(m_end_idx)
                  && lexer.is_delimiter(m_search_string_ptr->get_value(m_end_idx));
        // E.g. "foo\\", where '\' is a delimiter
        bool const succeeded_by_escaped_delim
                = m_search_string_ptr->get_value_is_escape(m_end_idx)
                  && lexer.is_delimiter(m_search_string_ptr->get_value(m_end_idx + 1));
        has_succeeding_delim = succeeded_by_greedy_wildcard || succeeded_by_non_greedy_wildcard
                               || succeeded_by_unescaped_delim || succeeded_by_escaped_delim;
    }

    return has_preceding_delim && has_succeeding_delim;
}

[[nodiscard]] auto WildcardExpression::create_view(
        uint32_t const start_idx,
        uint32_t const end_idx
) const -> WildcardExpressionView {
    return WildcardExpressionView{this, start_idx, end_idx};
}

auto VariableQueryToken::operator<(VariableQueryToken const& rhs) const -> bool {
    if (m_variable_type < rhs.m_variable_type) {
        return true;
    }
    if (m_variable_type > rhs.m_variable_type) {
        return false;
    }
    if (m_query_substring < rhs.m_query_substring) {
        return true;
    }
    if (m_query_substring > rhs.m_query_substring) {
        return false;
    }
    if (m_has_wildcard != rhs.m_has_wildcard) {
        return rhs.m_has_wildcard;
    }
    if (m_is_encoded != rhs.m_is_encoded) {
        return rhs.m_is_encoded;
    }
    return false;
}

auto VariableQueryToken::operator>(VariableQueryToken const& rhs) const -> bool {
    if (m_variable_type > rhs.m_variable_type) {
        return true;
    }
    if (m_variable_type < rhs.m_variable_type) {
        return false;
    }
    if (m_query_substring > rhs.m_query_substring) {
        return true;
    }
    if (m_query_substring < rhs.m_query_substring) {
        return false;
    }
    if (m_has_wildcard != rhs.m_has_wildcard) {
        return m_has_wildcard;
    }
    if (m_is_encoded != rhs.m_is_encoded) {
        return m_is_encoded;
    }
    return false;
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

    // Reserve size for m_logtype_string
    uint32_t logtype_string_size = 0;
    for (uint32_t i = 0; i < get_logtype_size(); i++) {
        if (auto const& logtype_token = get_logtype_token(i);
            std::holds_alternative<StaticQueryToken>(logtype_token))
        {
            logtype_string_size
                    += std::get<StaticQueryToken>(logtype_token).get_query_substring().size();
        } else {
            logtype_string_size++;
        }
    }
    m_logtype_string.reserve(logtype_string_size);

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
            encoded_variable_t encoded_var = 0;
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

auto QueryInterpretation::operator<(QueryInterpretation const& rhs) const -> bool {
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

auto operator<<(std::ostream& os, QueryInterpretation const& query_logtype) -> std::ostream& {
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
