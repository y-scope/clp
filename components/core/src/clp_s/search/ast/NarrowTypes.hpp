#ifndef CLP_S_SEARCH_NARROWTYPES_HPP
#define CLP_S_SEARCH_NARROWTYPES_HPP

#include <memory>
#include <vector>

#include <clp_s/search/ast/ColumnDescriptor.hpp>
#include <clp_s/search/ast/Expression.hpp>
#include <clp_s/search/ast/Transformation.hpp>

namespace clp_s::search::ast {
class NarrowTypes : public Transformation {
public:
    // Methods inherited from Transformation
    auto run(std::shared_ptr<Expression>& expr) -> std::shared_ptr<Expression> override;

private:
    /**
     * Narrow the type of an expression
     * @param cur the expression to narrow
     * @return the narrowed expression
     */
    auto narrow(std::shared_ptr<Expression> cur) -> std::shared_ptr<Expression>;

    bool m_should_renormalize{false};
    std::vector<std::shared_ptr<ColumnDescriptor>> m_local_exists_descriptors;
};
}  // namespace clp_s::search::ast

#endif  // CLP_S_SEARCH_NARROWTYPES_HPP
