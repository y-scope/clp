#ifndef CLP_S_SEARCH_VALUE_HPP
#define CLP_S_SEARCH_VALUE_HPP

#include <iostream>

namespace clp_s::search {
/**
 * Class representing a generic value in the AST. Key subclasses are Literal and Expression.
 */
class Value {
public:
    /**
     * @return The number of operands this value has
     */
    virtual unsigned get_num_operands() = 0;

    /**
     * Print a string representation of the value to standard error.
     * Useful for debugging in gdb.
     */
    virtual void print() = 0;

    virtual ~Value() = default;

protected:
    /**
     * @return The stream to print to
     */
    static std::ostream& get_print_stream() { return std::cerr; }
};
}  // namespace clp_s::search

#endif  // CLP_S_SEARCH_VALUE_HPP
