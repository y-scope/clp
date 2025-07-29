#include "Query.hpp"

#include <functional>
#include <set>
#include <string>
#include <unordered_set>

#include "Defs.h"

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

namespace clp {
QueryVar::QueryVar(encoded_variable_t precise_non_dict_var) {
    m_precise_var = precise_non_dict_var;
    m_is_precise_var = true;
    m_is_dict_var = false;
}

QueryVar::QueryVar(encoded_variable_t precise_dict_var, variable_dictionary_id_t var_dict_entry) {
    m_precise_var = precise_dict_var;
    m_is_precise_var = true;
    m_is_dict_var = true;
    m_var_dict_entry = var_dict_entry;
}

QueryVar::QueryVar(
        unordered_set<encoded_variable_t> const& possible_dict_vars,
        unordered_set<variable_dictionary_id_t> const& possible_var_dict_entries
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
           || (!m_is_precise_var && m_possible_dict_vars.count(var) > 0);
}

void QueryVar::remove_segments_that_dont_contain_dict_var(
        set<segment_id_t>& segment_ids,
        std::function<std::set<segment_id_t> const&(variable_dictionary_id_t)>
                get_segments_containing_var_dict_id
) const {
    if (false == m_is_dict_var) {
        // Not a dictionary variable, so do nothing
        return;
    }

    if (m_is_precise_var) {
        auto& ids_of_segments_containing_query_var
                = get_segments_containing_var_dict_id(m_var_dict_entry);
        inplace_set_intersection(ids_of_segments_containing_query_var, segment_ids);
    } else {
        set<segment_id_t> ids_of_segments_containing_query_var;
        for (auto entry : m_possible_var_dict_entries) {
            auto& ids_of_segments_containing_var = get_segments_containing_var_dict_id(entry);
            ids_of_segments_containing_query_var.insert(
                    ids_of_segments_containing_var.cbegin(),
                    ids_of_segments_containing_var.cend()
            );
        }
        inplace_set_intersection(ids_of_segments_containing_query_var, segment_ids);
    }
}

void SubQuery::add_non_dict_var(encoded_variable_t precise_non_dict_var) {
    m_vars.emplace_back(precise_non_dict_var);
}

void SubQuery::add_dict_var(
        encoded_variable_t precise_dict_var,
        variable_dictionary_id_t var_dict_entry
) {
    m_vars.emplace_back(precise_dict_var, var_dict_entry);
}

void SubQuery::add_imprecise_dict_var(
        unordered_set<encoded_variable_t> const& possible_dict_vars,
        unordered_set<variable_dictionary_id_t> const& possible_var_dict_entries
) {
    m_vars.emplace_back(possible_dict_vars, possible_var_dict_entries);
}

void SubQuery::set_possible_logtypes(unordered_set<logtype_dictionary_id_t> const& logtype_ids) {
    m_possible_logtype_ids = logtype_ids;
}

void SubQuery::mark_wildcard_match_required() {
    m_wildcard_match_required = true;
}

void SubQuery::calculate_ids_of_matching_segments(
        std::function<std::set<segment_id_t> const&(logtype_dictionary_id_t)>
                get_segments_containing_log_dict_id,
        std::function<std::set<segment_id_t> const&(variable_dictionary_id_t)>
                get_segments_containing_var_dict_id
) {
    // Get IDs of segments containing logtypes
    m_ids_of_matching_segments.clear();
    for (auto logtype_id : m_possible_logtype_ids) {
        auto const& ids_of_segments_containing_logtype
                = get_segments_containing_log_dict_id(logtype_id);
        m_ids_of_matching_segments.insert(
                ids_of_segments_containing_logtype.cbegin(),
                ids_of_segments_containing_logtype.cend()
        );
    }

    // Intersect with IDs of segments containing variables
    for (auto& query_var : m_vars) {
        query_var.remove_segments_that_dont_contain_dict_var(
                m_ids_of_matching_segments,
                get_segments_containing_var_dict_id
        );
    }
}

void SubQuery::clear() {
    m_vars.clear();
    m_possible_logtype_ids.clear();
    m_wildcard_match_required = false;
}

bool SubQuery::matches_logtype(logtype_dictionary_id_t const logtype) const {
    return m_possible_logtype_ids.count(logtype) > 0;
}

Query::Query(
        epochtime_t search_begin_timestamp,
        epochtime_t search_end_timestamp,
        bool ignore_case,
        std::string search_string,
        std::vector<SubQuery> sub_queries
)
        : m_search_begin_timestamp{search_begin_timestamp},
          m_search_end_timestamp{search_end_timestamp},
          m_ignore_case{ignore_case},
          m_search_string{std::move(search_string)},
          m_sub_queries{std::move(sub_queries)} {
    m_search_string_matches_all = (m_search_string.empty() || "*" == m_search_string);
}

void Query::make_sub_queries_relevant_to_segment(segment_id_t segment_id) {
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

void Query::calculate_ids_of_matching_segments(
        std::function<std::set<segment_id_t> const&(logtype_dictionary_id_t)>
                get_segments_containing_log_dict_id,
        std::function<std::set<segment_id_t> const&(variable_dictionary_id_t)>
                get_segments_containing_var_dict_id
) {
    for (auto& sub_query : m_sub_queries) {
        sub_query.calculate_ids_of_matching_segments(
                get_segments_containing_log_dict_id,
                get_segments_containing_var_dict_id
        );
    }
}
}  // namespace clp
