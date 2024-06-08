// Code from CLP

#ifndef CLP_S_SEARCH_CLP_SEARCH_QUERY_HPP
#define CLP_S_SEARCH_CLP_SEARCH_QUERY_HPP

#include <set>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "../../Defs.hpp"
#include "../../DictionaryEntry.hpp"
#include "../../Utils.hpp"

namespace clp_s::search::clp_search {
/**
 * Class representing a variable in a subquery. It can represent a precise encoded variable or
 * an imprecise dictionary variable (i.e., a set of possible encoded dictionary variable IDs)
 */
class QueryVar {
public:
    // Constructors
    explicit QueryVar(encoded_variable_t precise_non_dict_var);
    QueryVar(encoded_variable_t precise_dict_var, VariableDictionaryEntry const* var_dict_entry);
    QueryVar(
            std::unordered_set<encoded_variable_t> const& possible_dict_vars,
            std::unordered_set<VariableDictionaryEntry const*> const& possible_var_dict_entries
    );

    // Methods
    /**
     * Checks if the given encoded variable matches this QueryVar
     * @param var
     * @return true if matched, false otherwise
     */
    bool matches(encoded_variable_t var) const;

    bool is_precise_var() const { return m_is_precise_var; }

    bool is_dict_var() const { return m_is_dict_var; }

    VariableDictionaryEntry const* get_var_dict_entry() const { return m_var_dict_entry; }

    std::unordered_set<VariableDictionaryEntry const*> const& get_possible_var_dict_entries(
    ) const {
        return m_possible_var_dict_entries;
    }

private:
    // Variables
    bool m_is_precise_var;
    bool m_is_dict_var;

    encoded_variable_t m_precise_var;
    // Only used if the precise variable is a dictionary variable
    VariableDictionaryEntry const* m_var_dict_entry;

    // Only used if the variable is an imprecise dictionary variable
    std::unordered_set<encoded_variable_t> m_possible_dict_vars;
    std::unordered_set<VariableDictionaryEntry const*> m_possible_var_dict_entries;
};

/**
 * Class representing a subquery (or informally, an interpretation) of a user query. It contains
 * a series of possible logtypes, a set of QueryVars, and whether the query still requires
 * wildcard matching after it matches an encoded message.
 */
class SubQuery {
public:
    // Methods
    /**
     * Adds a precise non-dictionary variable to the subquery
     * @param precise_non_dict_var
     */
    void add_non_dict_var(encoded_variable_t precise_non_dict_var);
    /**
     * Adds a precise dictionary variable to the subquery
     * @param precise_dict_var
     * @param var_dict_entry
     */
    void add_dict_var(
            encoded_variable_t precise_dict_var,
            VariableDictionaryEntry const* var_dict_entry
    );
    /**
     * Adds an imprecise dictionary variable (i.e., a set of possible precise dictionary
     * variables) to the subquery
     * @param possible_dict_vars
     * @param possible_var_dict_entries
     */
    void add_imprecise_dict_var(
            std::unordered_set<encoded_variable_t> const& possible_dict_vars,
            std::unordered_set<VariableDictionaryEntry const*> const& possible_var_dict_entries
    );
    /**
     * Add a set of possible logtypes to the subquery
     * @param logtype_entries
     */
    void set_possible_logtypes(
            std::unordered_set<LogTypeDictionaryEntry const*> const& logtype_entries
    );
    void mark_wildcard_match_required();

    /**
     * Calculates the segment IDs that should contain a match for the subquery's current
     * logtypes and QueryVars
     */
    // void calculate_ids_of_matching_segments ();

    void clear();

    bool wildcard_match_required() const { return m_wildcard_match_required; }

    size_t get_num_possible_logtypes() const { return m_possible_logtype_ids.size(); }

    std::unordered_set<LogTypeDictionaryEntry const*> const& get_possible_logtype_entries() const {
        return m_possible_logtype_entries;
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
     * Whether the given variables contain the subquery's variables in order (but not
     * necessarily contiguously)
     * @param vars
     * @return true if matched, false otherwise
     */
    bool matches_vars(UnalignedMemSpan<int64_t> vars) const;

private:
    // Variables
    std::unordered_set<LogTypeDictionaryEntry const*> m_possible_logtype_entries;
    std::unordered_set<logtype_dictionary_id_t> m_possible_logtype_ids;
    std::set<segment_id_t> m_ids_of_matching_segments;
    std::vector<QueryVar> m_vars;
    bool m_wildcard_match_required;
};

/**
 * Class representing a user query with potentially multiple sub-queries.
 */
class Query {
public:
    // Constructors
    Query(bool ignore_case, std::string const& search_string, std::vector<SubQuery> sub_queries)
            : m_ignore_case(ignore_case),
              m_sub_queries(std::move(sub_queries)) {
        set_search_string(search_string);
    }

    void set_ignore_case(bool ignore_case) { m_ignore_case = ignore_case; }

    void set_search_string(std::string const& search_string);

    void add_sub_query(SubQuery const& sub_query);

    void clear_sub_queries();

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

private:
    // Variables
    bool m_ignore_case;
    std::string m_search_string;
    std::vector<SubQuery> m_sub_queries;
    std::vector<SubQuery const*> m_relevant_sub_queries;
    bool m_search_string_matches_all;
};
}  // namespace clp_s::search::clp_search

#endif  // CLP_S_SEARCH_CLP_SEARCH_QUERY_HPP
