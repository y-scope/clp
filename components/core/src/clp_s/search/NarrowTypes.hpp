#ifndef CLP_S_SEARCH_NARROWTYPES_HPP
#define CLP_S_SEARCH_NARROWTYPES_HPP

#include <vector>

#include "ColumnDescriptor.hpp"
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
    std::shared_ptr<Expression> narrow(std::shared_ptr<Expression> cur);

    bool m_should_renormalize{false};
    std::vector<std::shared_ptr<ColumnDescriptor>> m_local_exists_descriptors;
};
}  // namespace clp_s::search

#endif  // CLP_S_SEARCH_NARROWTYPES_HPP
