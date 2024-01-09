#include "EmptyExpr.hpp"

namespace clp_s::search {
EmptyExpr::EmptyExpr(Expression* parent) : Expression(false, parent) {}

EmptyExpr::EmptyExpr(EmptyExpr const& expr) : Expression(expr) {}

std::shared_ptr<Expression> EmptyExpr::create(Expression* parent) {
    return std::shared_ptr<Expression>(static_cast<Expression*>(new EmptyExpr(parent)));
}

void EmptyExpr::print() {
    auto& os = get_print_stream();
    os << "EmptyExpr()";

    if (get_parent() == nullptr) {
        os << std::endl;
    } else {
        os << std::flush;
    }
}

std::shared_ptr<Expression> EmptyExpr::copy() const {
    // Copy on EmptyExpr can use default shallow copy
    return std::shared_ptr<Expression>(static_cast<Expression*>(new EmptyExpr(*this)));
}
}  // namespace clp_s::search
