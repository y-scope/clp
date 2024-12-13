#ifndef CLP_S_SEARCH_STRINGLITERAL_HPP
#define CLP_S_SEARCH_STRINGLITERAL_HPP

#include <memory>
#include <string>

#include "../Utils.hpp"
#include "Literal.hpp"

namespace clp_s::search {
/**
 * Class for String literals in the search AST
 *
 * StringLiteral will automatically classify itself as possibly matching
 * a clp style (containing spaces) and/or variable style (not containing spaces)
 * string at creation time.
 */
class StringLiteral : public Literal {
public:
    // Deleted copy
    StringLiteral(StringLiteral const&) = delete;
    StringLiteral& operator=(StringLiteral const&) = delete;

    /**
     * Create a StringLiteral from a string
     * @param v
     * @return A new StringLiteral
     */
    static std::shared_ptr<Literal> create(std::string const& v);

    /**
     * @return Reference to underlying string
     */
    std::string& get();

    // Methods inherited from Value
    void print() override;

    // Methods inherited from Literal
    bool matches_type(LiteralType type) override { return type & m_string_type; }

    bool matches_any(LiteralTypeBitmask mask) override { return mask & m_string_type; }

    bool matches_exactly(LiteralTypeBitmask mask) override { return mask == m_string_type; }

    bool as_clp_string(std::string& ret, FilterOperation op) override;

    bool as_var_string(std::string& ret, FilterOperation op) override;

    bool as_float(double& ret, FilterOperation op) override;

    bool as_int(int64_t& ret, FilterOperation op) override;

    bool as_bool(bool& ret, FilterOperation op) override;

    bool as_null(FilterOperation op) override;

    bool as_any(FilterOperation op) override;

private:
    std::string m_v;
    LiteralTypeBitmask m_string_type;

    // Constructor
    explicit StringLiteral(std::string v) : m_v(std::move(v)), m_string_type(0) {
        if (m_v.find(' ') != std::string::npos) {
            m_string_type = LiteralType::ClpStringT;
        } else {
            m_string_type = LiteralType::VarStringT;
        }

        if (StringUtils::has_unescaped_wildcards(m_v)) {
            m_string_type |= LiteralType::ClpStringT;
        }
    }
};
}  // namespace clp_s::search

#endif  // CLP_S_SEARCH_STRINGLITERAL_HPP
