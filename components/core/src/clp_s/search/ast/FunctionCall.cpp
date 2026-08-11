#include "FunctionCall.hpp"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <clp_s/search/ast/ColumnDescriptor.hpp>
#include <clp_s/search/ast/Expression.hpp>
#include <clp_s/search/ast/Value.hpp>

namespace clp_s::search::ast {
auto FunctionCall::create(
        std::string function_name,
        std::shared_ptr<ColumnDescriptor> column_arg,
        bool inverted,
        Expression* parent
) -> std::shared_ptr<Expression> {
    std::vector<std::shared_ptr<Value>> args;
    args.push_back(std::move(column_arg));
    return std::shared_ptr<Expression>{
            new FunctionCall{std::move(function_name), std::move(args), inverted, parent}
    };
}

auto FunctionCall::create(
        std::string function_name,
        std::vector<std::shared_ptr<Value>> args,
        bool inverted,
        Expression* parent
) -> std::shared_ptr<Expression> {
    return std::shared_ptr<Expression>{
            new FunctionCall{std::move(function_name), std::move(args), inverted, parent}
    };
}

FunctionCall::FunctionCall(
        std::string function_name,
        std::vector<std::shared_ptr<Value>> args,
        bool inverted,
        Expression* parent
)
        : Expression{inverted, parent},
          m_function_name{std::move(function_name)} {
    for (auto& arg : args) {
        if (auto expr = std::dynamic_pointer_cast<Expression>(arg)) {
            expr->set_parent(this);
        }
        m_operands.push_back(std::move(arg));
    }
}

auto FunctionCall::print() const -> void {
    auto& out = get_print_stream();
    out << "FunctionCall(" << m_function_name << ", [";
    bool first{true};
    for (auto it = op_begin(); it != op_end(); ++it) {
        if (!first) {
            out << ", ";
        }
        first = false;
        (*it)->print();
    }
    out << "])";
}

// Shallow-copy via the defaulted copy ctor, then deep-copy ColumnDescriptor operands (mirrors
// FilterExpr::copy). Other operands are shared Literals.
auto FunctionCall::copy() const -> std::shared_ptr<Expression> {
    auto new_expr = std::shared_ptr<Expression>{new FunctionCall{*this}};
    for (auto it = new_expr->op_begin(); it != new_expr->op_end(); ++it) {
        if (auto descriptor = std::dynamic_pointer_cast<ColumnDescriptor>(*it)) {
            *it = descriptor->copy();
        }
    }
    return new_expr;
}
}  // namespace clp_s::search::ast
