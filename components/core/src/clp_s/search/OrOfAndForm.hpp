#ifndef CLP_S_SEARCH_OROFANDFORM_HPP
#define CLP_S_SEARCH_OROFANDFORM_HPP

#include <vector>

#include "AndExpr.hpp"
#include "OrExpr.hpp"
#include "Transformation.hpp"

namespace clp_s::search {
using ExpressionVector = std::vector<std::shared_ptr<Expression>>;
using ExpressionList = std::list<std::shared_ptr<Expression>>;

// TODO: handle degenerate forms like empty or/and expressions
class OrOfAndForm : public Transformation {
public:
    // Methods inherited from Transformation
    std::shared_ptr<Expression> run(std::shared_ptr<Expression>& expr) override;

private:
    /**
     * Use De Morgan's laws to convert the expression to a canonical form
     * @param expr
     */
    static void de_morgan(std::shared_ptr<Expression>& expr);

    /**
     * Simplify an expression
     * @param expr
     * @return The simplified expression
     */
    static std::shared_ptr<Expression> simplify(std::shared_ptr<Expression> const& expr);

    /**
     * Simplify an Or expression
     * @param expr
     * @return The simplified expression
     */
    static std::shared_ptr<Expression> simplify_or(std::shared_ptr<Expression> const& expr);

    /**
     * Simplify an And expression
     * @param expr
     * @return The simplified expression
     */
    static std::shared_ptr<Expression> simplify_and(std::shared_ptr<Expression> const& expr);

    /**
     * Insert all combinations of And expressions into an Or expression
     * @param new_or_expr
     * @param base_and_expr
     * @param cur
     * @param end
     * @param prefix
     */
    static void insert_all_combinations(
            std::shared_ptr<Expression> const& new_or_expr,
            std::shared_ptr<Expression> const& base_and_expr,
            ExpressionVector::iterator cur,
            ExpressionVector::iterator end,
            ExpressionList& prefix
    );
};
}  // namespace clp_s::search

#endif  // CLP_S_SEARCH_OROFANDFORM_HPP
