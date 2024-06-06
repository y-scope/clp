// Code from CLP

#ifndef CLP_S_SEARCH_CLP_SEARCH_GREP_HPP
#define CLP_S_SEARCH_CLP_SEARCH_GREP_HPP

#include <memory>
#include <optional>
#include <string>

#include "../../Defs.hpp"
#include "../../DictionaryReader.hpp"
#include "Query.hpp"

namespace clp_s::search::clp_search {
class Grep {
public:
    // Methods
    /**
     * Processes a raw user query into a Query
     * @param log_dict
     * @param var_dict
     * @param search_string
     * @param ignore_case
     * @param add_wildcards
     * @return Query if it may match a message, std::nullopt otherwise
     */
    static std::optional<Query> process_raw_query(
            std::shared_ptr<LogTypeDictionaryReader> log_dict,
            std::shared_ptr<VariableDictionaryReader> var_dict,
            std::string const& search_string,
            bool ignore_case,
            bool add_wildcards = true
    );

    /**
     * Returns bounds of next potential variable (either a definite variable or a token with
     * wildcards)
     * @param value String containing token
     * @param begin_pos Begin position of last token, changes to begin position of next token
     * @param end_pos End position of last token, changes to end position of next token
     * @param is_var Whether the token is definitely a variable
     * @return true if another potential variable was found, false otherwise
     */
    static bool get_bounds_of_next_potential_var(
            std::string const& value,
            size_t& begin_pos,
            size_t& end_pos,
            bool& is_var
    );
};
}  // namespace clp_s::search::clp_search

#endif  // CLP_S_SEARCH_CLP_SEARCH_GREP_HPP
