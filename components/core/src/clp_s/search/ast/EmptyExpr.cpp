#include "EmptyExpr.hpp"

#include <memory>
#include <ostream>

#include <clp_s/search/ast/Expression.hpp>

namespace clp_s::search::ast {
EmptyExpr::EmptyExpr(Expression* parent) : Expression(false, parent) {}

EmptyExpr::EmptyExpr(EmptyExpr const& expr) : Expression(expr) {}

auto EmptyExpr::create(Expression* parent) -> std::shared_ptr<Expression> {
    return std::shared_ptr<Expression>(static_cast<Expression*>(new EmptyExpr(parent)));
}

auto EmptyExpr::print() const -> void {
    auto& os = get_print_stream();
    os << "EmptyExpr()";

    if (get_parent() == nullptr) {
        os << '\n';
    }
    os << std::flush;
}

auto EmptyExpr::copy() const -> std::shared_ptr<Expression> {
    // Copy on EmptyExpr can use default shallow copy
    return std::shared_ptr<Expression>(static_cast<Expression*>(new EmptyExpr(*this)));
}
}  // namespace clp_s::search::ast
