#ifndef CLP_S_SEARCH_OROFANDFORM_HPP
#define CLP_S_SEARCH_OROFANDFORM_HPP

#include <list>
#include <memory>
#include <vector>

#include "Expression.hpp"
#include "Transformation.hpp"

namespace clp_s::search::ast {
using ExpressionVector = std::vector<std::shared_ptr<Expression>>;
using ExpressionList = std::list<std::shared_ptr<Expression>>;

// TODO: handle degenerate forms like empty or/and expressions
class OrOfAndForm : public Transformation {
public:
    // Methods inherited from Transformation
    auto run(std::shared_ptr<Expression>& expr) -> std::shared_ptr<Expression> override;

private:
    /**
     * Use De Morgan's laws to convert the expression to a canonical form
     * @param expr
     */
    static auto de_morgan(std::shared_ptr<Expression>& expr) -> void;

    /**
     * Simplify an expression
     * @param expr
     * @return The simplified expression
     */
    static auto simplify(std::shared_ptr<Expression> const& expr) -> std::shared_ptr<Expression>;

    /**
     * Simplify an Or expression
     * @param expr
     * @return The simplified expression
     */
    static auto simplify_or(std::shared_ptr<Expression> const& expr) -> std::shared_ptr<Expression>;

    /**
     * Simplify an And expression
     * @param expr
     * @return The simplified expression
     */
    static auto simplify_and(std::shared_ptr<Expression> const& expr)
            -> std::shared_ptr<Expression>;

    /**
     * Insert all combinations of And expressions into an Or expression
     * @param new_or_expr
     * @param base_and_expr
     * @param cur
     * @param end
     * @param prefix
     */
    static auto insert_all_combinations(
            std::shared_ptr<Expression> const& new_or_expr,
            std::shared_ptr<Expression> const& base_and_expr,
            ExpressionVector::iterator cur,
            ExpressionVector::iterator end,
            ExpressionList& prefix
    ) -> void;
};
}  // namespace clp_s::search::ast

#endif  // CLP_S_SEARCH_OROFANDFORM_HPP
