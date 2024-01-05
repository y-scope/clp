#ifndef CLP_S_SEARCH_FILTER_OPERATION_H
#define CLP_S_SEARCH_FILTER_OPERATION_H

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

#endif  // CLP_S_SEARCH_FILTER_OPERATION_H
