#ifndef CLP_S_SEARCH_CONVERTTOEXISTS_HPP
#define CLP_S_SEARCH_CONVERTTOEXISTS_HPP

#include <memory>

#include <clp_s/search/ast/Expression.hpp>

#include "Transformation.hpp"

namespace clp_s::search::ast {
// Must run after NarrowTypes pass
class ConvertToExists : public Transformation {
public:
    // Methods inherited from Transformation
    auto run(std::shared_ptr<Expression>& expr) -> std::shared_ptr<Expression> override;

private:
    /**
     * Convert an expression to exists form
     * @param cur the expression to convert
     * @return A new expression in exists form
     */
    auto convert(std::shared_ptr<Expression> cur) -> std::shared_ptr<Expression>;

    // Data members
    bool m_needs_constant_prop{false};
    bool m_needs_standard_form{false};
};
}  // namespace clp_s::search::ast

#endif  // CLP_S_SEARCH_CONVERTTOEXISTS_HPP
