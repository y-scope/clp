#ifndef CLP_S_SEARCH_CONVERTTOEXISTS_HPP
#define CLP_S_SEARCH_CONVERTTOEXISTS_HPP

#include "Transformation.hpp"

namespace clp_s::search {
// Must run after NarrowTypes pass
class ConvertToExists : public Transformation {
public:
    // Constructors
    ConvertToExists() : m_needs_constant_prop(false), m_needs_standard_form(false) {}

    // Methods inherited from Transformation
    std::shared_ptr<Expression> run(std::shared_ptr<Expression>& expr) override;

private:
    bool m_needs_constant_prop;
    bool m_needs_standard_form;

    /**
     * Convert an expression to exists form
     * @param cur the expression to convert
     * @return A new expression in exists form
     */
    std::shared_ptr<Expression> convert(std::shared_ptr<Expression> cur);
};
}  // namespace clp_s::search

#endif  // CLP_S_SEARCH_CONVERTTOEXISTS_HPP
