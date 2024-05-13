// Code from CLP

#include "Query.hpp"

using std::set;
using std::string;
using std::unordered_set;

namespace clp_s::search::clp_search {
// Local function prototypes
/**
 * Performs a set intersection of a & b, storing the result in b
 * @tparam SetType
 * @param a
 * @param b
 */
template <typename SetType>
static void inplace_set_intersection(SetType const& a, SetType& b);

template <typename SetType>
static void inplace_set_intersection(SetType const& a, SetType& b) {
    for (auto ix = b.cbegin(); ix != b.cend();) {
        if (a.count(*ix) == 0) {
            ix = b.erase(ix);
        } else {
            ++ix;
        }
    }
}

QueryVar::QueryVar(encoded_variable_t precise_non_dict_var) {
    m_precise_var = precise_non_dict_var;
    m_is_precise_var = true;
    m_is_dict_var = false;
    m_var_dict_entry = nullptr;
}

QueryVar::QueryVar(
        encoded_variable_t precise_dict_var,
        VariableDictionaryEntry const* var_dict_entry
) {
    m_precise_var = precise_dict_var;
    m_is_precise_var = true;
    m_is_dict_var = true;
    m_var_dict_entry = var_dict_entry;
}

QueryVar::QueryVar(
        unordered_set<encoded_variable_t> const& possible_dict_vars,
        unordered_set<VariableDictionaryEntry const*> const& possible_var_dict_entries
) {
    m_is_dict_var = true;
    if (possible_dict_vars.size() == 1) {
        // A single possible variable is the same as a precise variable
        m_precise_var = *possible_dict_vars.cbegin();
        m_is_precise_var = true;
        m_var_dict_entry = *possible_var_dict_entries.cbegin();
    } else {
        m_possible_dict_vars = possible_dict_vars;
        m_is_precise_var = false;
        m_possible_var_dict_entries = possible_var_dict_entries;
    }
}

bool QueryVar::matches(encoded_variable_t var) const {
    return (m_is_precise_var && m_precise_var == var)
           || (false == m_is_precise_var && m_possible_dict_vars.count(var) > 0);
}

void SubQuery::add_non_dict_var(encoded_variable_t precise_non_dict_var) {
    m_vars.emplace_back(precise_non_dict_var);
}

void SubQuery::add_dict_var(
        encoded_variable_t precise_dict_var,
        VariableDictionaryEntry const* var_dict_entry
) {
    m_vars.emplace_back(precise_dict_var, var_dict_entry);
}

void SubQuery::add_imprecise_dict_var(
        unordered_set<encoded_variable_t> const& possible_dict_vars,
        unordered_set<VariableDictionaryEntry const*> const& possible_var_dict_entries
) {
    m_vars.emplace_back(possible_dict_vars, possible_var_dict_entries);
}

void SubQuery::set_possible_logtypes(
        unordered_set<LogTypeDictionaryEntry const*> const& logtype_entries
) {
    m_possible_logtype_ids.clear();

    for (auto const* entry : logtype_entries) {
        m_possible_logtype_ids.insert(entry->get_id());
    }
    m_possible_logtype_entries = logtype_entries;
}

void SubQuery::mark_wildcard_match_required() {
    m_wildcard_match_required = true;
}

void SubQuery::clear() {
    m_vars.clear();
    m_possible_logtype_ids.clear();
    m_wildcard_match_required = false;
}

bool SubQuery::matches_logtype(logtype_dictionary_id_t const logtype) const {
    return m_possible_logtype_ids.count(logtype) > 0;
}

bool SubQuery::matches_vars(UnalignedMemSpan<int64_t> vars) const {
    if (vars.size() < m_vars.size()) {
        // Not enough variables to satisfy query
        return false;
    }

    // Try to find m_vars in vars, in order, but not necessarily contiguously
    size_t possible_vars_ix = 0;
    size_t const num_possible_vars = m_vars.size();
    size_t vars_ix = 0;
    size_t const num_vars = vars.size();
    while (possible_vars_ix < num_possible_vars && vars_ix < num_vars) {
        QueryVar const& possible_var = m_vars[possible_vars_ix];

        if (possible_var.matches(vars[vars_ix])) {
            // Matched
            ++possible_vars_ix;
            ++vars_ix;
        } else {
            ++vars_ix;
        }
    }
    return (num_possible_vars == possible_vars_ix);
}

void Query::set_search_string(string const& search_string) {
    m_search_string = search_string;
    m_search_string_matches_all = (m_search_string.empty() || "*" == m_search_string);
}

void Query::add_sub_query(SubQuery const& sub_query) {
    m_sub_queries.push_back(sub_query);
}

void Query::clear_sub_queries() {
    m_sub_queries.clear();
}
}  // namespace clp_s::search::clp_search
