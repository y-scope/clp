#ifndef CLP_S_SEARCH_FUNCTIONCALL_HPP
#define CLP_S_SEARCH_FUNCTIONCALL_HPP

#include <memory>
#include <string>
#include <vector>

#include <clp_s/search/ast/ColumnDescriptor.hpp>
#include <clp_s/search/ast/Expression.hpp>
#include <clp_s/search/ast/Value.hpp>

namespace clp_s::search::ast {
/**
 * Class representing a function call in the Search AST. Enables UDFs and function syntax for
 * filters and projections.
 */
class FunctionCall : public Expression {
public:
    // Factory methods
    /**
     * Create a FunctionCall with a name and a single column argument.
     * @param function_name the name of the function.
     * @param column_arg the column descriptor argument.
     * @param inverted expression is inverted when true.
     * @param parent parent this expression is attached to.
     * @return Newly created FunctionCall expression.
     */
    static auto create(
            std::string function_name,
            std::shared_ptr<ColumnDescriptor> column_arg,
            bool inverted = false,
            Expression* parent = nullptr
    ) -> std::shared_ptr<Expression>;

    /**
     * Create a FunctionCall with a name and multiple arguments.
     * @param function_name the name of the function.
     * @param args the argument expressions.
     * @param inverted expression is inverted when true.
     * @param parent parent this expression is attached to.
     * @return Newly created FunctionCall expression.
     */
    static auto create(
            std::string function_name,
            std::vector<std::shared_ptr<Value>> args,
            bool inverted = false,
            Expression* parent = nullptr
    ) -> std::shared_ptr<Expression>;

    // Delete copy assignment operator
    auto operator=(FunctionCall const&) -> FunctionCall& = delete;

    // Delete move constructors and assignment operators
    FunctionCall(FunctionCall&&) = delete;
    auto operator=(FunctionCall&&) -> FunctionCall& = delete;

    // Destructor
    ~FunctionCall() override = default;

    // Methods implementing Value
    auto print() const -> void override;

    // Methods implementing Expression
    auto has_only_expression_operands() -> bool override { return false; }

    [[nodiscard]] auto copy() const -> std::shared_ptr<Expression> override;

    // Methods
    [[nodiscard]] auto get_function_name() const -> std::string const& { return m_function_name; }

    [[nodiscard]] auto get_args() const -> std::vector<std::shared_ptr<Value>> {
        return {op_begin(), op_end()};
    }

private:
    // Constructor
    explicit FunctionCall(
            std::string function_name,
            std::vector<std::shared_ptr<Value>> args,
            bool inverted = false,
            Expression* parent = nullptr
    );

    // Default copy constructor
    FunctionCall(FunctionCall const&) = default;

    // Data members
    std::string m_function_name;
};
}  // namespace clp_s::search::ast

#endif  // CLP_S_SEARCH_FUNCTIONCALL_HPP
