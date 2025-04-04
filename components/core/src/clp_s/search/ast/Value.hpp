#ifndef CLP_S_SEARCH_VALUE_HPP
#define CLP_S_SEARCH_VALUE_HPP

#include <cstddef>
#include <iostream>

namespace clp_s::search::ast {
/**
 * Class representing a generic value in the AST. Values can be both Literals and Expressions and
 * can have some number of "operands", where each operand is a child node in the AST.
 *
 * All nodes in the AST are derived from Value.
 */
class Value {
public:
    // Default copy & move constructors and assignment operators and destructor
    Value() = default;
    Value(Value const&) = default;
    auto operator=(Value const&) -> Value& = default;
    Value(Value&&) = default;
    auto operator=(Value&&) -> Value& = default;
    virtual ~Value() = default;

    /**
     * @return The number of operands of this Value.
     */
    [[nodiscard]] virtual auto get_num_operands() const -> size_t = 0;

    /**
     * Prints a string representation of this Value to the output stream designated by
     * `get_print_stream`.
     *
     * This hook is meant to be used when debugging in gdb.
     */
    virtual void print() const = 0;

protected:
    /**
     * @return The output stream that should be used by the `print` function.
     */
    [[nodiscard]] static auto get_print_stream() -> std::ostream& { return std::cerr; }
};
}  // namespace clp_s::search::ast

#endif  // CLP_S_SEARCH_VALUE_HPP
