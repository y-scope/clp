#ifndef CLP_S_SEARCH_INTEGRAL_HPP
#define CLP_S_SEARCH_INTEGRAL_HPP

#include <memory>
#include <string>
#include <variant>

#include "Literal.hpp"

namespace clp_s::search {
using Integral64 = std::variant<int64_t, double>;

// FIXME: figure out why String types are part of this bitmask
constexpr LiteralTypeBitmask cIntegralLiteralTypes = cIntegralTypes | VarStringT;

/**
 * Class for Integral values (float/int) in the search AST
 */
class Integral : public Literal {
public:
    // Deleted copy
    Integral(Integral const&) = delete;

    Integral& operator=(Integral const&) = delete;

    /**
     * Create an Integral literal from an double value
     * @param v the value
     * @return an Integral literal
     */
    static std::shared_ptr<Literal> create_from_float(double v);

    /**
     * Create an Integral literal from an integral value
     * @param v the value
     * @return an Integral literal
     */
    static std::shared_ptr<Literal> create_from_int(int64_t v);

    /**
     * Try to create an integral literal from a string
     * @param v the string we are attempting to convert to Integral
     * @return an Integral literal, or nullptr if the string does not represent an integral
     */
    static std::shared_ptr<Literal> create_from_string(std::string const& v);

    /**
     * Return the underlying integral value
     * @return the underlying integral value
     */
    Integral64 get();

    // Methods inherited from Value
    void print() override;

    // Methods inherited from Literal
    bool matches_type(LiteralType type) override { return type & cIntegralLiteralTypes; }

    bool matches_any(LiteralTypeBitmask mask) override { return mask & cIntegralLiteralTypes; }

    bool matches_exactly(LiteralTypeBitmask mask) override { return mask == cIntegralLiteralTypes; }

    bool as_epoch_date() override { return true; }

    bool as_var_string(std::string& ret, FilterOperation op) override;

    bool as_float(double& ret, FilterOperation op) override;

    bool as_int(int64_t& ret, FilterOperation op) override;

protected:
    Integral64 m_v;
    std::string m_vstr;  // original string representation if created from string

    // Constructors
    explicit Integral(double v);

    explicit Integral(int64_t v);
};
}  // namespace clp_s::search

#endif  // CLP_S_SEARCH_INTEGRAL_HPP
