#ifndef CLP_S_SEARCH_NULLLITERAL_HPP
#define CLP_S_SEARCH_NULLLITERAL_HPP

#include <memory>
#include <string>
#include <variant>

#include "Literal.hpp"

namespace clp_s::search {
/**
 * Class for Null literals in the search AST
 */
class NullLiteral : public Literal {
public:
    // Deleted copy
    NullLiteral(NullLiteral const&) = delete;

    NullLiteral& operator=(NullLiteral const&) = delete;

    /**
     * Explicit create a null literal
     * @return A newly created null literal
     */
    static std::shared_ptr<Literal> create();

    /**
     * Try to create a null literal from a string
     * @param v the string we are attempting to convert to Null
     * @return A null literal, or nullptr if the string does not represent "null"
     */
    static std::shared_ptr<Literal> create_from_string(std::string const& v);

    // Methods inherited from Value
    void print() override;

    // Methods inherited from Literal
    bool matches_type(LiteralType type) override { return type & LiteralType::NullT; }

    bool matches_any(LiteralTypeBitmask mask) override { return mask & LiteralType::NullT; }

    bool matches_exactly(LiteralTypeBitmask mask) override { return mask == LiteralType::NullT; }

    bool as_var_string(std::string& ret, FilterOperation op) override;

    bool as_null(FilterOperation op) override;

private:
    // Constructor
    NullLiteral() = default;
};
}  // namespace clp_s::search

#endif  // CLP_S_SEARCH_NULLLITERAL_HPP
