#ifndef CLP_S_SEARCH_TRANSFORMATION_HPP
#define CLP_S_SEARCH_TRANSFORMATION_HPP

#include "Expression.hpp"

namespace clp_s::search {
/**
 * Generic class representing a transformation on some expression.
 */
class Transformation {
public:
    /**
     * Runs the pass. The expression passed as input may be mutated by the pass.
     * @param expr the expression that the pass will run on
     * @return a new expression; may be the same as the input expression or different
     */
    virtual std::shared_ptr<Expression> run(std::shared_ptr<Expression>& expr) = 0;
};
}  // namespace clp_s::search

#endif  // CLP_S_SEARCH_TRANSFORMATION_HPP
