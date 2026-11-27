#ifndef CLP_S_SEARCH_LITERAL_HPP
#define CLP_S_SEARCH_LITERAL_HPP

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <type_traits>

#include "FilterOperation.hpp"
#include "Value.hpp"

namespace clp_s::search::ast {
/**
 * An enum representing all of the Literal types that can show up in the AST.
 *
 * `StringT` is the single string type for all string flavours. A string literal is compatible with
 * every string occurrence: the occurrence's MPT `NodeType` (e.g. `NodeType::ClpString`,
 * `NodeType::VarString` or `NodeType::UnstructurizedText`) determines how the match is evaluated,
 * never the literal's type.
 */
enum LiteralType : uint32_t {
    TypesBegin = 1,
    IntegerT = 1,
    FloatT = 1 << 1,
    StringT = 1 << 2,
    BooleanT = 1 << 3,
    ArrayT = 1 << 4,
    NullT = 1 << 5,
    TimestampT = 1 << 6,
    TypesEnd = 1 << 7,
    UnknownT = static_cast<uint32_t>(1) << 31
};

using literal_type_bitmask_t = std::underlying_type_t<LiteralType>;

constexpr literal_type_bitmask_t cIntegralTypes = LiteralType::IntegerT | LiteralType::FloatT;
constexpr literal_type_bitmask_t cAllTypes = TypesEnd - 1;

/**
 * Whether the string a literal is compared as against a string occurrence contains a space.
 */
enum class StringSpaceClass : uint8_t {
    // A whole-value match against a string with no space.
    ContainsNoSpace,
    // The pattern has a literal space, so every match contains a space.
    ContainsSpace,
    // A wildcard without a literal space; `*` and `?` can match a space.
    MayContainSpace
};

/**
 * Parent class for all Literals in the AST.
 */
class Literal : public Value {
public:
    /**
     * Literals are considered to have 1 operand.
     * @return 1
     */
    [[nodiscard]] auto get_num_operands() const -> size_t override { return 1ULL; }

    /**
     * Strict checks for type matching against a given literal type.
     * @return true if the check succeeds
     */
    [[nodiscard]] virtual auto matches_type(LiteralType type) const -> bool = 0;

    [[nodiscard]] virtual auto matches_any(literal_type_bitmask_t mask) const -> bool = 0;

    [[nodiscard]] virtual auto matches_exactly(literal_type_bitmask_t mask) const -> bool = 0;

    /**
     * Convert LiteralType enum values to strings . Only used for printing.
     * @param type the enum value being turned in a string
     * @return A string representing the enum value
     */
    static auto type_to_string(LiteralType type) -> std::string {
        switch (type) {
            case LiteralType::IntegerT:
                return "int";
            case LiteralType::FloatT:
                return "float";
            case LiteralType::BooleanT:
                return "bool";
            case LiteralType::ArrayT:
                return "array";
            case LiteralType::NullT:
                return "null";
            case LiteralType::TimestampT:
                return "timestamp";
            case LiteralType::StringT:
                return "string";
            default:
                return "errtype";
        }
    }

    /**
     * Casts the literal to the string it is compared as against a string occurrence.
     *
     * @param ret Returns the casted value.
     * @param op The `FilterOperation` operating on the literal.
     * @return Whether the cast is successful.
     */
    virtual auto as_string(std::string& /*ret*/, FilterOperation /*op*/) -> bool { return false; }

    virtual auto as_float(double& /*ret*/, FilterOperation /*op*/) -> bool { return false; }

    virtual auto as_int(int64_t& /*ret*/, FilterOperation /*op*/) -> bool { return false; }

    virtual auto as_bool(bool& /*ret*/, FilterOperation /*op*/) -> bool { return false; }

    virtual auto as_null(FilterOperation /*op*/) -> bool { return false; }

    virtual auto as_timestamp() -> bool { return false; }

    virtual auto as_any(FilterOperation /*op*/) -> bool { return false; }

    /**
     * Checks compatibility with a value type and comparison operation without converting the
     * literal. Unlike `matches_type()`, which reports the interpretations the literal carries,
     * this also accounts for `op` and for wildcard patterns; wildcard compatibility may be
     * conservative and does not guarantee a match against any particular value.
     * @param type The single value type to test.
     * @param op The comparison operation; existence operations are handled separately by callers.
     * @return Whether the literal is compatible with `type` under `op`.
     */
    [[nodiscard]] virtual auto can_match_op(LiteralType type, FilterOperation op) const -> bool {
        return false;
    }

    [[nodiscard]] virtual auto has_wildcard() const -> bool { return false; }

    /**
     * @return The literal's string space class, or std::nullopt if it has no string
     * interpretation.
     */
    [[nodiscard]] virtual auto get_string_space_class() const -> std::optional<StringSpaceClass> {
        return std::nullopt;
    }
};
}  // namespace clp_s::search::ast

#endif  // CLP_S_SEARCH_LITERAL_HPP
