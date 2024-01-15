#ifndef GLT_FFI_SEARCH_QUERY_METHODS_HPP
#define GLT_FFI_SEARCH_QUERY_METHODS_HPP

#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "CompositeWildcardToken.hpp"
#include "ExactVariableToken.hpp"
#include "Subquery.hpp"
#include "WildcardToken.hpp"

namespace glt::ffi::search {
template <typename encoded_variable_t>
void generate_subqueries(
        std::string_view wildcard_query,
        std::vector<Subquery<encoded_variable_t>>& sub_queries
);
}  // namespace glt::ffi::search

#endif  // GLT_FFI_SEARCH_QUERY_METHODS_HPP
