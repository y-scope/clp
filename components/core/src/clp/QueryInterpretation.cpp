#include "QueryInterpretation.hpp"

#include <utility>

#include <log_surgeon/Constants.hpp>

#include "LogSurgeonReader.hpp"
#include "Utils.hpp"

namespace clp {

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
            os << std::get<StaticQueryToken>(query_token).get_query_stubstring();
        } else {
            auto const& variable_token = std::get<VariableQueryToken>(query_token);
            os << "<" << variable_token.get_variable_type() << ">("
               << variable_token.get_query_stubstring() << ")";
        }
    }
    os << "\"";
    os << "(";
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
    os << ")";
    os << "(";
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
    os << ")";
    return os;
}
}  // namespace clp
