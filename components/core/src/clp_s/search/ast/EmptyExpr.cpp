#include "EmptyExpr.hpp"

#include <memory>
#include <optional>
#include <ostream>

#include <clp_s/search/ast/Expression.hpp>

namespace clp_s::search::ast {
EmptyExpr::EmptyExpr(Expression* parent) : Expression(false, parent) {}

auto EmptyExpr::create(Expression* parent) -> std::shared_ptr<Expression> {
    return std::shared_ptr<Expression>(static_cast<Expression*>(new EmptyExpr(parent)));
}

auto EmptyExpr::create_constant(bool value, Expression* parent) -> std::shared_ptr<Expression> {
    auto constant{create(parent)};
    if (value) {
        constant->invert();
    }
    return constant;
}

auto EmptyExpr::as_constant(Expression const& expr) -> std::optional<bool> {
    if (nullptr == dynamic_cast<EmptyExpr const*>(&expr)) {
        return std::nullopt;
    }
    return expr.is_inverted();
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
    return std::shared_ptr<Expression>(static_cast<Expression*>(new EmptyExpr(*this)));
}
}  // namespace clp_s::search::ast
