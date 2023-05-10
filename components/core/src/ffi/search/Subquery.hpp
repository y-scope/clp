#ifndef FFI_SEARCH_SUBQUERY_HPP
#define FFI_SEARCH_SUBQUERY_HPP

// C++ standard libraries
#include <string>
#include <variant>
#include <vector>

// Project headers
#include "ExactVariableToken.hpp"
#include "WildcardToken.hpp"

namespace ffi::search {
    /**
     * A class representing a subquery. Each subquery encompasses a single
     * logtype query and zero or more variable queries. Both the logtype and
     * variables may contain wildcards.
     * @tparam encoded_variable_t The type of encoded variables
     */
    template<typename encoded_variable_t>
    class Subquery {
    public:
        using QueryVariables = std::vector<
                std::variant<
                        ExactVariableToken<encoded_variable_t>,
                        WildcardToken<encoded_variable_t>
                >
        >;

        // Constructors
        Subquery (std::string logtype_query, QueryVariables variables);

        // Methods
        [[nodiscard]] const std::string& get_logtype_query () const { return m_logtype_query; }
        [[nodiscard]] bool logtype_query_contains_wildcards () const {
            return m_logtype_query_contains_wildcards;
        }
        [[nodiscard]] const QueryVariables& get_query_vars () const {
            return m_query_vars;
        }

        /**
         * @param logtype_query
         * @param variables
         * @return Whether the given logtype query and query variables match this
         * subquery.
         */
        bool equals (const std::string& logtype_query,
                     const Subquery::QueryVariables& variables) const
        {
            return logtype_query == m_logtype_query && variables == m_query_vars;
        }

    private:
        // Variables
        std::string m_logtype_query;
        bool m_logtype_query_contains_wildcards;
        QueryVariables m_query_vars;
    };
}

#endif // FFI_SEARCH_SUBQUERY_HPP
