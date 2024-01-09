#ifndef CLP_S_SEARCH_FILTEROPERATION_HPP
#define CLP_S_SEARCH_FILTEROPERATION_HPP

namespace clp_s::search {
/**
 * Enum describing all supported filtering operations in the search AST
 */
enum FilterOperation {
    EXISTS,
    NEXISTS,
    EQ,
    NEQ,
    LT,
    GT,
    LTE,
    GTE
};
}  // namespace clp_s::search

#endif  // CLP_S_SEARCH_FILTEROPERATION_HPP
