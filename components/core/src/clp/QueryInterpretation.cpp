#include "QueryInterpretation.hpp"

#include <log_surgeon/Constants.hpp>

#include "LogSurgeonReader.hpp"
#include "Utils.hpp"

namespace clp {

bool QueryInterpretation::operator<(QueryInterpretation const& rhs) const {
    if (m_logtype.size() < rhs.m_logtype.size()) {
        return true;
    } else if (m_logtype.size() > rhs.m_logtype.size()) {
        return false;
    }
    for (uint32_t i = 0; i < m_logtype.size(); i++) {
        if (m_logtype[i] < rhs.m_logtype[i]) {
            return true;
        } else if (m_logtype[i] > rhs.m_logtype[i]) {
            return false;
        }
    }
    for (uint32_t i = 0; i < m_query.size(); i++) {
        if (m_query[i] < rhs.m_query[i]) {
            return true;
        } else if (m_query[i] > rhs.m_query[i]) {
            return false;
        }
    }
    for (uint32_t i = 0; i < m_is_encoded_with_wildcard.size(); i++) {
        if (m_is_encoded_with_wildcard[i] < rhs.m_is_encoded_with_wildcard[i]) {
            return true;
        } else if (m_is_encoded_with_wildcard[i] > rhs.m_is_encoded_with_wildcard[i]) {
            return false;
        }
    }
    return false;
}

void QueryInterpretation::append_logtype(QueryInterpretation& suffix) {
    m_logtype.insert(m_logtype.end(), suffix.m_logtype.begin(), suffix.m_logtype.end());
    m_query.insert(m_query.end(), suffix.m_query.begin(), suffix.m_query.end());
    m_is_encoded_with_wildcard.insert(
            m_is_encoded_with_wildcard.end(),
            suffix.m_is_encoded_with_wildcard.begin(),
            suffix.m_is_encoded_with_wildcard.end()
    );
    m_var_has_wildcard.insert(
            m_var_has_wildcard.end(),
            suffix.m_var_has_wildcard.begin(),
            suffix.m_var_has_wildcard.end()
    );
}

void QueryInterpretation::append_value(
        std::variant<char, int> const& val,
        std::string const& string,
        bool var_contains_wildcard,
        bool is_encoded_with_wildcard
) {
    m_var_has_wildcard.push_back(var_contains_wildcard);
    m_logtype.push_back(val);
    m_query.push_back(string);
    m_is_encoded_with_wildcard.push_back(is_encoded_with_wildcard);
}

std::ostream& operator<<(std::ostream& os, QueryInterpretation const& query_logtype) {
    os << "\"";
    for (uint32_t idx = 0; idx < query_logtype.get_logtype_size(); idx++) {
        if (std::holds_alternative<char>(query_logtype.get_logtype_value(idx))) {
            os << std::get<char>(query_logtype.get_logtype_value(idx));
        } else {
            os << "<" << std::get<int>(query_logtype.get_logtype_value(idx)) << ">("
               << query_logtype.get_query_string(idx) << ")";
        }
    }
    os << "\"";
    os << "(";
    for (uint32_t idx = 0; idx < query_logtype.get_logtype_size(); idx++) {
        os << query_logtype.get_var_has_wildcard(idx);
    }
    os << ")";
    os << "(";
    for (uint32_t idx = 0; idx < query_logtype.get_logtype_size(); idx++) {
        os << query_logtype.get_is_encoded_with_wildcard(idx);
    }
    os << ")";
    return os;
}
}  // namespace clp
