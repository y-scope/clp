#ifndef CLP_S_SEARCH_TRANSFORMATION_HPP
#define CLP_S_SEARCH_TRANSFORMATION_HPP

#include <memory>

#include <clp_s/search/ast/Expression.hpp>

namespace clp_s::search::ast {
/**
 * Generic class representing a transformation on some expression.
 */
class Transformation {
public:
    // Delete copy constructor and assignment operator
    Transformation(Transformation const&) = delete;
    auto operator=(Transformation const&) -> Transformation& = delete;

    // Delete move constructor and assignment operator
    Transformation(Transformation&&) = delete;
    auto operator=(Transformation&&) -> Transformation& = delete;

    // Destructor
    virtual ~Transformation() = default;

    // Methods
    /**
     * Runs the pass. The expression passed as input may be mutated by the pass.
     * @param expr the expression that the pass will run on
     * @return a new expression; may be the same as the input expression or different
     */
    virtual auto run(std::shared_ptr<Expression>& expr) -> std::shared_ptr<Expression> = 0;

protected:
    // Constructors
    Transformation() = default;
};
}  // namespace clp_s::search::ast

#endif  // CLP_S_SEARCH_TRANSFORMATION_HPP
