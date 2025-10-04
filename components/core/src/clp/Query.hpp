#ifndef CLP_QUERY_HPP
#define CLP_QUERY_HPP

#include <functional>
#include <set>
#include <string>
#include <unordered_set>
#include <vector>

#include "Defs.h"

namespace clp {
/**
 * Class representing a variable in a subquery. It can represent a precise encoded variable or an
 * imprecise dictionary variable (i.e., a set of possible encoded dictionary variable IDs)
 */
class QueryVar {
public:
    // Constructors
    explicit QueryVar(encoded_variable_t precise_non_dict_var);
    QueryVar(encoded_variable_t precise_dict_var, variable_dictionary_id_t var_dict_id);
    QueryVar(
            std::unordered_set<encoded_variable_t> const& possible_dict_vars,
            std::unordered_set<variable_dictionary_id_t> const& possible_var_dict_ids
    );

    // Methods
    auto operator==(QueryVar const& rhs) const -> bool = default;

    /**
     * Checks if the given encoded variable matches this QueryVar
     * @param var
     * @return true if matched, false otherwise
     */
    bool matches(encoded_variable_t var) const;

    /**
     * Removes segments from the given set that don't contain the given variable
     * @param segment_ids
     * @param get_segments_containing_var_dict_id
     */
    void remove_segments_that_dont_contain_dict_var(
            std::set<segment_id_t>& segment_ids,
            std::function<std::set<segment_id_t> const&(variable_dictionary_id_t)> const&
                    get_segments_containing_var_dict_id
    ) const;

    bool is_precise_var() const { return m_is_precise_var; }

    bool is_dict_var() const { return m_is_dict_var; }

    variable_dictionary_id_t get_var_dict_id() const { return m_var_dict_id; }

    std::unordered_set<variable_dictionary_id_t> const& get_possible_var_dict_ids() const {
        return m_possible_var_dict_ids;
    }

private:
    // Variables
    bool m_is_precise_var{};
    bool m_is_dict_var{};

    encoded_variable_t m_precise_var{};
    // Only used if the precise variable is a dictionary variable
    variable_dictionary_id_t m_var_dict_id{};

    // Only used if the variable is an imprecise dictionary variable
    std::unordered_set<encoded_variable_t> m_possible_dict_vars;
    std::unordered_set<variable_dictionary_id_t> m_possible_var_dict_ids;
};

/**
 * Class representing a subquery (or informally, an interpretation) of a user query. It contains a
 * series of possible logtypes, a set of QueryVars, and whether the query still requires wildcard
 * matching after it matches an encoded message.
 */
class SubQuery {
public:
    // Methods
    auto operator==(SubQuery const& rhs) const -> bool = default;

    /**
     * Adds a precise non-dictionary variable to the subquery
     * @param precise_non_dict_var
     */
    void add_non_dict_var(encoded_variable_t precise_non_dict_var);
    /**
     * Adds a precise dictionary variable to the subquery
     * @param precise_dict_var
     * @param var_dict_id
     */
    void add_dict_var(encoded_variable_t precise_dict_var, variable_dictionary_id_t var_dict_id);
    /**
     * Adds an imprecise dictionary variable (i.e., a set of possible precise dictionary variables)
     * to the subquery
     * @param possible_dict_vars
     * @param possible_var_dict_ids
     */
    void add_imprecise_dict_var(
            std::unordered_set<encoded_variable_t> const& possible_dict_vars,
            std::unordered_set<variable_dictionary_id_t> const& possible_var_dict_ids
    );
    /**
     * Add a set of possible logtypes to the subquery
     * @param logtype_ids
     */
    void set_possible_logtypes(std::unordered_set<logtype_dictionary_id_t> const& logtype_ids);
    void mark_wildcard_match_required();

    /**
     * Calculates the segment IDs that should contain a match for the subquery's current logtypes
     * and QueryVars.
     * @param get_segments_containing_logtype_dict_id
     * @param get_segments_containing_var_dict_id
     */
    void calculate_ids_of_matching_segments(
            std::function<std::set<segment_id_t> const&(logtype_dictionary_id_t)> const&
                    get_segments_containing_logtype_dict_id,
            std::function<std::set<segment_id_t> const&(variable_dictionary_id_t)> const&
                    get_segments_containing_var_dict_id
    );

    void clear();

    bool wildcard_match_required() const { return m_wildcard_match_required; }

    size_t get_num_possible_logtypes() const { return m_possible_logtypes.size(); }

    std::unordered_set<logtype_dictionary_id_t> const& get_possible_logtypes() const {
        return m_possible_logtypes;
    }

    size_t get_num_possible_vars() const { return m_vars.size(); }

    std::vector<QueryVar> const& get_vars() const { return m_vars; }

    std::set<segment_id_t> const& get_ids_of_matching_segments() const {
        return m_ids_of_matching_segments;
    }

    /**
     * Whether the given logtype ID matches one of the possible logtypes in this subquery
     * @param logtype
     * @return true if matched, false otherwise
     */
    bool matches_logtype(logtype_dictionary_id_t logtype) const;
    /**
     * Whether the given variables contain the subquery's variables in order (but not necessarily
     * contiguously)
     * @tparam EncodedVariableContainerType A random access list of `clp::encoded_variable_t`.
     * @param vars
     * @return true if matched, false otherwise
     */
    template <typename EncodedVariableContainerType>
    bool matches_vars(EncodedVariableContainerType const& vars) const;

private:
    // Variables
    std::unordered_set<logtype_dictionary_id_t> m_possible_logtypes;
    std::set<segment_id_t> m_ids_of_matching_segments;
    std::vector<QueryVar> m_vars;
    bool m_wildcard_match_required{false};
};

/**
 * Class representing a user query with potentially multiple sub-queries.
 */
class Query {
public:
    // Constructors
    Query(epochtime_t search_begin_timestamp,
          epochtime_t search_end_timestamp,
          bool ignore_case,
          std::string search_string,
          std::vector<SubQuery> sub_queries);

    // Methods
    /**
     * Populates the set of relevant sub-queries with only those that match the given segment
     * @param segment_id
     */
    void make_sub_queries_relevant_to_segment(segment_id_t segment_id);

    epochtime_t get_search_begin_timestamp() const { return m_search_begin_timestamp; }

    epochtime_t get_search_end_timestamp() const { return m_search_end_timestamp; }

    /**
     * Checks if the given timestamp is in the search time range (begin and end inclusive)
     * @param timestamp
     * @return true if the timestamp is in the search time range
     * @return false otherwise
     */
    bool timestamp_is_in_search_time_range(epochtime_t timestamp) const {
        return (m_search_begin_timestamp <= timestamp && timestamp <= m_search_end_timestamp);
    }

    bool get_ignore_case() const { return m_ignore_case; }

    std::string const& get_search_string() const { return m_search_string; }

    /**
     * Checks if the search string will match all messages (i.e., it's "" or "*")
     * @return true if the search string will match all messages
     * @return false otherwise
     */
    bool search_string_matches_all() const { return m_search_string_matches_all; }

    std::vector<SubQuery> const& get_sub_queries() const { return m_sub_queries; }

    bool contains_sub_queries() const { return m_sub_queries.empty() == false; }

    std::vector<SubQuery const*> const& get_relevant_sub_queries() const {
        return m_relevant_sub_queries;
    }

    /**
     * Calculates the segment IDs that should contain a match for each subquery's logtypes and
     * QueryVars.
     * @param get_segments_containing_logtype_dict_id
     * @param get_segments_containing_var_dict_id
     */
    void calculate_ids_of_matching_segments(
            std::function<std::set<segment_id_t> const&(logtype_dictionary_id_t)> const&
                    get_segments_containing_logtype_dict_id,
            std::function<std::set<segment_id_t> const&(variable_dictionary_id_t)> const&
                    get_segments_containing_var_dict_id
    );

private:
    // Variables
    // Start of search time range (inclusive)
    epochtime_t m_search_begin_timestamp{cEpochTimeMin};
    // End of search time range (inclusive)
    epochtime_t m_search_end_timestamp{cEpochTimeMax};
    bool m_ignore_case{false};
    std::string m_search_string;
    bool m_search_string_matches_all{true};
    std::vector<SubQuery> m_sub_queries;
    std::vector<SubQuery const*> m_relevant_sub_queries;
    segment_id_t m_prev_segment_id{cInvalidSegmentId};
};

template <typename EncodedVariableContainerType>
bool SubQuery::matches_vars(EncodedVariableContainerType const& vars) const {
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
}  // namespace clp

#endif  // CLP_QUERY_HPP
