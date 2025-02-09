#ifndef CLP_S_SEARCH_BOOLEANLITERAL_HPP
#define CLP_S_SEARCH_BOOLEANLITERAL_HPP

#include <memory>
#include <string>
#include <variant>

#include "Literal.hpp"

namespace clp_s::search {
/**
 * Class representing a Boolean literal in the search AST
 */
class BooleanLiteral : public Literal {
public:
    // Deleted copy
    BooleanLiteral(BooleanLiteral const&) = delete;
    BooleanLiteral& operator=(BooleanLiteral const&) = delete;

    /**
     * Create a bool literal
     * @param v the value of the boolean
     * @return A Boolean literal
     */
    static std::shared_ptr<Literal> create_from_bool(bool v);

    /**
     * Attempt to create a bool literal from a string
     * @param v the string we are attempting to convert to bool
     * @return A Boolean literal, or nullptr if the string does not represent a bool
     */
    static std::shared_ptr<Literal> create_from_string(std::string const& v);

    // Methods inherited from Value
    void print() override;

    // Methods inherited from Literal
    bool matches_type(LiteralType type) override { return type & LiteralType::BooleanT; }

    bool matches_any(LiteralTypeBitmask mask) override { return mask & LiteralType::BooleanT; }

    bool matches_exactly(LiteralTypeBitmask mask) override { return mask == LiteralType::BooleanT; }

    bool as_var_string(std::string& ret, FilterOperation op) override;

    bool as_bool(bool& ret, FilterOperation op) override;

private:
    bool m_v;

    // Constructors
    BooleanLiteral() = default;

    explicit BooleanLiteral(bool v) : m_v(v) {}
};
}  // namespace clp_s::search

#endif  // CLP_S_SEARCH_BOOLEANLITERAL_HPP
