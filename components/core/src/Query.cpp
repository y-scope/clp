#include "Query.hpp"

using std::set;
using std::string;
using std::unordered_set;

// Local function prototypes
/**
 * Performs a set intersection of a & b, storing the result in b
 * @tparam SetType
 * @param a
 * @param b
 */
template<typename SetType>
static void inplace_set_intersection (const SetType& a, SetType& b);

template<typename SetType>
static void inplace_set_intersection (const SetType& a, SetType& b) {
    for (auto ix = b.cbegin(); ix != b.cend();) {
        if (a.count(*ix) == 0) {
            ix = b.erase(ix);
        } else {
            ++ix;
        }
    }
}

QueryVar::QueryVar (encoded_variable_t precise_non_dict_var) {
    m_precise_var = precise_non_dict_var;
    m_is_precise_var = true;
    m_is_dict_var = false;
    m_var_dict_entry = nullptr;
}

QueryVar::QueryVar (encoded_variable_t precise_dict_var, const VariableDictionaryEntry* var_dict_entry) {
    m_precise_var = precise_dict_var;
    m_is_precise_var = true;
    m_is_dict_var = true;
    m_var_dict_entry = var_dict_entry;
}

QueryVar::QueryVar (const unordered_set<encoded_variable_t>& possible_dict_vars, const unordered_set<const VariableDictionaryEntry*>& possible_var_dict_entries)
{
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

bool QueryVar::matches (encoded_variable_t var) const {
    return (m_is_precise_var && m_precise_var == var) || (!m_is_precise_var && m_possible_dict_vars.count(var) > 0);
}

void QueryVar::remove_segments_that_dont_contain_dict_var (set<segment_id_t>& segment_ids) const {
    if (false == m_is_dict_var) {
        // Not a dictionary variable, so do nothing
        return;
    }

    if (m_is_precise_var) {
        auto& ids_of_segments_containing_query_var = m_var_dict_entry->get_ids_of_segments_containing_entry();
        inplace_set_intersection(ids_of_segments_containing_query_var, segment_ids);
    } else {
        set<segment_id_t> ids_of_segments_containing_query_var;
        for (auto entry : m_possible_var_dict_entries) {
            auto& ids_of_segments_containing_var = entry->get_ids_of_segments_containing_entry();
            ids_of_segments_containing_query_var.insert(ids_of_segments_containing_var.cbegin(), ids_of_segments_containing_var.cend());
        }
        inplace_set_intersection(ids_of_segments_containing_query_var, segment_ids);
    }
}

void SubQuery::add_non_dict_var (encoded_variable_t precise_non_dict_var) {
    m_vars.emplace_back(precise_non_dict_var);
}

void SubQuery::add_dict_var (encoded_variable_t precise_dict_var, const VariableDictionaryEntry* var_dict_entry) {
    m_vars.emplace_back(precise_dict_var, var_dict_entry);
}

void SubQuery::add_imprecise_dict_var (const unordered_set<encoded_variable_t>& possible_dict_vars,
                                       const unordered_set<const VariableDictionaryEntry*>& possible_var_dict_entries)
{
    m_vars.emplace_back(possible_dict_vars, possible_var_dict_entries);
}

void SubQuery::set_possible_logtypes (const unordered_set<const LogTypeDictionaryEntry*>& logtype_entries) {
    m_possible_logtype_ids.clear();
    for (auto entry : logtype_entries) {
        m_possible_logtype_ids.insert(entry->get_id());
    }
    m_possible_logtype_entries = logtype_entries;
}

void SubQuery::mark_wildcard_match_required () {
    m_wildcard_match_required = true;
}

void SubQuery::calculate_ids_of_matching_segments () {
    // Get IDs of segments containing logtypes
    m_ids_of_matching_segments.clear();
    for (auto entry : m_possible_logtype_entries) {
        auto& ids_of_segments_containing_logtype = entry->get_ids_of_segments_containing_entry();
        m_ids_of_matching_segments.insert(ids_of_segments_containing_logtype.cbegin(), ids_of_segments_containing_logtype.cend());
    }

    // Intersect with IDs of segments containing variables
    for (auto& query_var : m_vars) {
        query_var.remove_segments_that_dont_contain_dict_var(m_ids_of_matching_segments);
    }
}

void SubQuery::clear () {
    m_vars.clear();
    m_possible_logtype_ids.clear();
    m_wildcard_match_required = false;
}

bool SubQuery::matches_logtype (const logtype_dictionary_id_t logtype) const {
    return m_possible_logtype_ids.count(logtype) > 0;
}

bool SubQuery::matches_vars (const std::vector<encoded_variable_t>& vars) const {
    if (vars.size() < m_vars.size()) {
        // Not enough variables to satisfy query
        return false;
    }

    // Try to find m_vars in vars, in order, but not necessarily contiguously
    size_t possible_vars_ix = 0;
    const size_t num_possible_vars = m_vars.size();
    size_t vars_ix = 0;
    const size_t num_vars = vars.size();
    while (possible_vars_ix < num_possible_vars && vars_ix < num_vars) {
        const QueryVar& possible_var = m_vars[possible_vars_ix];

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

#include <iostream>
auto SubQuery::print () const -> void {
    std::cout << m_possible_logtype_entries.size() << std::endl;
    std::cout << m_possible_logtype_ids.size() << std::endl;
    std::cout << m_ids_of_matching_segments.size() << std::endl;
    std::cout << m_vars.size() << std::endl;
    std::cout << m_wildcard_match_required << std::endl;
    
    for (auto const& var : m_vars) {
        if(var.is_precise_var()) {
            std::cout << var.get_var_dict_entry()->get_value() << std::endl;
        } else {
            for(auto const& var_dict_entry : var.get_possible_var_dict_entries()) {
                std::cout << var_dict_entry->get_value() << std::endl;
            }
        }
    }
    
    for (auto const& logtype_entry : m_possible_logtype_entries) {
        std::cout << logtype_entry->get_value() << std::endl;
    }
    
    std::unordered_set<const LogTypeDictionaryEntry*> m_possible_logtype_entries;
    std::unordered_set<logtype_dictionary_id_t> m_possible_logtype_ids;
    std::set<segment_id_t> m_ids_of_matching_segments;
    std::vector<QueryVar> m_vars;
    bool m_wildcard_match_required;
}

void Query::set_search_string (const string& search_string) {
    m_search_string = search_string;
    m_search_string_matches_all = (m_search_string.empty() || "*" == m_search_string);
}

void Query::add_sub_query (const SubQuery& sub_query) {
    m_sub_queries.push_back(sub_query);

    // Add to relevant sub-queries if necessary
    if (sub_query.get_ids_of_matching_segments().count(m_prev_segment_id)) {
        m_relevant_sub_queries.push_back(&m_sub_queries.back());
    }
}

void Query::clear_sub_queries() {
    m_sub_queries.clear();
    m_relevant_sub_queries.clear();
}

void Query::make_sub_queries_relevant_to_segment (segment_id_t segment_id) {
    if (segment_id == m_prev_segment_id) {
        // Sub-queries already relevant to segment
        return;
    }

    // Make sub-queries relevant to segment
    m_relevant_sub_queries.clear();
    for (auto& sub_query : m_sub_queries) {
        if (sub_query.get_ids_of_matching_segments().count(segment_id)) {
            m_relevant_sub_queries.push_back(&sub_query);
        }
    }
    m_prev_segment_id = segment_id;
}
