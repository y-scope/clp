#ifndef CLP_S_SEARCH_NARROWTYPES_HPP
#define CLP_S_SEARCH_NARROWTYPES_HPP

#include "Transformation.hpp"

namespace clp_s::search {
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
    static std::shared_ptr<Expression> narrow(std::shared_ptr<Expression> cur);
};
}  // namespace clp_s::search

#endif  // CLP_S_SEARCH_NARROWTYPES_HPP
