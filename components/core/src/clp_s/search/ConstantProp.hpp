#ifndef CLP_S_SEARCH_CONSTANTPROP_HPP
#define CLP_S_SEARCH_CONSTANTPROP_HPP

#include "Transformation.hpp"

namespace clp_s::search {
// Constant propagate empty expressions keeping all remaining data IN PLACE
class ConstantProp : public Transformation {
public:
    // Methods inherited from Transformation
    std::shared_ptr<Expression> run(std::shared_ptr<Expression>& expr) override;

private:
    /**
     * Propagate empty expressions through the expression tree
     * @param cur
     * @return A new expression with empty expressions propagated
     */
    static std::shared_ptr<Expression> propagate_empty(std::shared_ptr<Expression> cur);
};
}  // namespace clp_s::search

#endif  // CLP_S_SEARCH_CONSTANTPROP_HPP
