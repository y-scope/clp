#ifndef CLP_S_SEARCH_LITERAL_HPP
#define CLP_S_SEARCH_LITERAL_HPP

#include <string>

#include "FilterOperation.hpp"
#include "Value.hpp"

namespace clp_s::search {
/**
 * An enum representing all of the Literal types that can show up in the AST.
 */
enum LiteralType : uint32_t {
    TypesBegin = 1,
    IntegerT = 1,
    FloatT = 1 << 1,
    ClpStringT = 1 << 2,
    VarStringT = 1 << 3,
    BooleanT = 1 << 4,
    ArrayT = 1 << 5,
    NullT = 1 << 6,
    EpochDateT = 1 << 7,
    TypesEnd = 1 << 8,
    UnknownT = ((uint32_t)1) << 31
};

using LiteralTypeBitmask = uint32_t;

constexpr LiteralTypeBitmask cIntegralTypes = LiteralType::IntegerT | LiteralType::FloatT;
constexpr LiteralTypeBitmask cAllTypes = TypesEnd - 1;

/**
 * Parent class for all Literals in the AST.
 */
class Literal : public Value {
public:
    /**
     * Literals are considered to have 1 operand.
     * @return 1
     */
    unsigned get_num_operands() override { return 1; }

    /**
     * Strict checks for type matching against a given literal type.
     * @return true if the check succeeds
     */
    virtual bool matches_type(LiteralType type) = 0;

    virtual bool matches_any(LiteralTypeBitmask mask) = 0;

    virtual bool matches_exactly(LiteralTypeBitmask mask) = 0;

    /**
     * Convert LiteralType enum values to strings . Only used for printing.
     * @param type the enum value being turned in a string
     * @return A string representing the enum value
     */
    static std::string type_to_string(LiteralType type) {
        switch (type) {
            case LiteralType::IntegerT:
                return "int";
            case LiteralType::FloatT:
                return "float";
            case LiteralType::ClpStringT:
                return "clpstring";
            case LiteralType::VarStringT:
                return "varstring";
            case LiteralType::BooleanT:
                return "bool";
            case LiteralType::ArrayT:
                return "array";
            case LiteralType::NullT:
                return "null";
            case LiteralType::EpochDateT:
                return "epochdate";
            default:
                return "errtype";
        }
    }

    /**
     * Functions to check type conversion and cast when possible under a given filter operation.
     * By default all casts fail until overriden by the derived literal types.
     * @param ret the casted value
     * @param op the FilterOperation operating on the Literal
     * @return true if cast is successful
     */
    virtual bool as_clp_string(std::string& ret, FilterOperation op) { return false; }

    virtual bool as_var_string(std::string& ret, FilterOperation op) { return false; }

    virtual bool as_float(double& ret, FilterOperation op) { return false; }

    virtual bool as_int(int64_t& ret, FilterOperation op) { return false; }

    virtual bool as_bool(bool& ret, FilterOperation op) { return false; }

    virtual bool as_null(FilterOperation op) { return false; }

    inline bool as_array(std::string& ret, FilterOperation op) {
        return as_var_string(ret, op) || as_clp_string(ret, op);
    }

    virtual bool as_epoch_date() { return false; }

    virtual bool as_any(FilterOperation op) { return false; }
};
}  // namespace clp_s::search

#endif  // CLP_S_SEARCH_LITERAL_HPP
