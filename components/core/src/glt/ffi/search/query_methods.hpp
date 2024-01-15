#ifndef CLP_FFI_SEARCH_QUERY_METHODS_HPP
#define CLP_FFI_SEARCH_QUERY_METHODS_HPP

#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "CompositeWildcardToken.hpp"
#include "ExactVariableToken.hpp"
#include "Subquery.hpp"
#include "WildcardToken.hpp"

namespace clp::ffi::search {
template <typename encoded_variable_t>
void generate_subqueries(
        std::string_view wildcard_query,
        std::vector<Subquery<encoded_variable_t>>& sub_queries
);
}  // namespace clp::ffi::search

#endif  // CLP_FFI_SEARCH_QUERY_METHODS_HPP
