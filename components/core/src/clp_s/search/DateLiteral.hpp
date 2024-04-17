#ifndef CLP_S_SEARCH_DATELITERAL_HPP
#define CLP_S_SEARCH_DATELITERAL_HPP

#include <memory>

#include "../Defs.hpp"
#include "Integral.hpp"

namespace clp_s::search {
constexpr LiteralTypeBitmask cDateLiteralTypes = EpochDateT;

/**
 * Class for Date literal in the search AST. Represents time
 * in epoch time.
 */
class DateLiteral : public Integral {
public:
    // Deleted copy
    DateLiteral(DateLiteral const&) = delete;
    DateLiteral& operator=(DateLiteral const&) = delete;

    /**
     * Create a Date literal from an integral value
     * @param v the time as a double or epoch
     * @return A Date literal
     */
    static std::shared_ptr<Literal> create_from_float(double v);
    static std::shared_ptr<Literal> create_from_int(epochtime_t v);

    /**
     * Attempt to create a Date literal from string. Tries to parse the string using
     * TimestampPattern.
     * @return A Date Literal or nullptr if the string can not be parsed as date.
     */
    static std::shared_ptr<Literal> create_from_string(std::string const& v);

    // Methods inherited from Value
    void print() override;

    // Methods inherited from Literal
    bool matches_type(LiteralType type) override { return type & cDateLiteralTypes; }

    bool matches_any(LiteralTypeBitmask mask) override { return mask & cDateLiteralTypes; }

    bool matches_exactly(LiteralTypeBitmask mask) override { return mask == cDateLiteralTypes; }

    bool as_epoch_date() override { return true; }

    bool as_clp_string(std::string& ret, FilterOperation op) override;

    bool as_var_string(std::string& ret, FilterOperation op) override;

private:
    std::string m_epoch_str;

    // Constructors
    explicit DateLiteral(double v, std::string s);

    explicit DateLiteral(epochtime_t v, std::string s);
};
}  // namespace clp_s::search

#endif  // CLP_S_SEARCH_DATELITERAL_HPP
