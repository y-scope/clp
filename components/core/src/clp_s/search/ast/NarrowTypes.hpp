#ifndef CLP_S_SEARCH_NARROWTYPES_HPP
#define CLP_S_SEARCH_NARROWTYPES_HPP

#include <memory>
#include <vector>

#include <clp_s/search/ast/ColumnDescriptor.hpp>
#include <clp_s/search/ast/FilterExpr.hpp>
#include <clp_s/search/ast/Transformation.hpp>

namespace clp_s::search::ast {
class NarrowTypes : public Transformation {
public:
    // Methods inherited from Transformation
    std::shared_ptr<Expression> run(std::shared_ptr<Expression>& expr) override;

private:
    /**
     * Narrow the type of an expression
     * @param cur the expression to narrow
     * @return the narrowed expression
     */
    std::shared_ptr<Expression> narrow(std::shared_ptr<Expression> cur);

    /**
     * Rewrites a negated equality (`NOT EQ`, or bare `NEQ`) whose column started with both string
     * types and was narrowed to just one, into an OR of the filter and an EXISTS for the other
     * string type (fix for y-scope/clp#254): values of the other string type can never equal the
     * operand, since a variable string must not contain a space and a clp string must. The rewrite
     * doesn't apply to the pure wildcard column `*` (it resolves to every node in a schema, so the
     * existence of one node of the other string type says nothing about the values of the others)
     * or to shape() columns (an EXISTS pinned to one string type can never resolve to the
     * StructurizedText or ParentRule node a shape() column requires). The EXISTS expression is
     * deduplicated per sibling group via `m_local_exists_descriptors`: an AND over N independent
     * rewrites would otherwise be expanded by OrOfAndForm into 2^N AND clauses.
     * @param filter The filter to consider rewriting, after its column was narrowed.
     * @param matches_var_and_clp_string Whether the filter's column matched both string types
     * before narrowing.
     * @param narrowed_clp_string Whether the filter's column was narrowed out of ClpStringT.
     * @param narrowed_var_string Whether the filter's column was narrowed out of VarStringT.
     * @return The rewritten expression, or nullptr if the rewrite doesn't apply.
     */
    auto rewrite_negated_string_equality(
            std::shared_ptr<FilterExpr> const& filter,
            bool matches_var_and_clp_string,
            bool narrowed_clp_string,
            bool narrowed_var_string
    ) -> std::shared_ptr<Expression>;

    bool m_should_renormalize{false};
    std::vector<std::shared_ptr<ColumnDescriptor>> m_local_exists_descriptors;
};
}  // namespace clp_s::search::ast

#endif  // CLP_S_SEARCH_NARROWTYPES_HPP
