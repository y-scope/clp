#ifndef CLP_S_SEARCH_VALUELITERAL_HPP
#define CLP_S_SEARCH_VALUELITERAL_HPP

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include <clp_s/search/ast/FilterOperation.hpp>
#include <clp_s/search/ast/Literal.hpp>

namespace clp_s::search::ast {
/**
 * A polymorphic string, integer, float, boolean, or null literal with all valid interpretations
 * precomputed.
 */
class ValueLiteral : public Literal {
public:
    // Factory methods
    [[nodiscard]] static auto create_from_int(int64_t v) -> std::shared_ptr<Literal>;

    [[nodiscard]] static auto create_from_float(double v) -> std::shared_ptr<Literal>;

    [[nodiscard]] static auto create_from_bool(bool v) -> std::shared_ptr<Literal>;

    [[nodiscard]] static auto create_from_string(std::string_view v) -> std::shared_ptr<Literal>;

    // Delete copy constructor and assignment operator
    ValueLiteral(ValueLiteral const&) = delete;
    auto operator=(ValueLiteral const&) -> ValueLiteral& = delete;

    // Delete move constructor and assignment operator
    ValueLiteral(ValueLiteral&&) = delete;
    auto operator=(ValueLiteral&&) -> ValueLiteral& = delete;

    // Destructor
    ~ValueLiteral() override = default;

    // Methods implementing Value
    void print() const override;

    // Methods implementing Literal
    [[nodiscard]] auto matches_type(LiteralType type) const -> bool override {
        return 0 != (m_types & type);
    }

    [[nodiscard]] auto matches_any(literal_type_bitmask_t mask) const -> bool override {
        return 0 != (m_types & mask);
    }

    [[nodiscard]] auto matches_exactly(literal_type_bitmask_t mask) const -> bool override {
        return m_types == mask;
    }

    // Methods overriding Literal
    auto as_clp_string(std::string& ret, FilterOperation op) -> bool override;

    auto as_var_string(std::string& ret, FilterOperation op) -> bool override;

    auto as_float(double& ret, FilterOperation op) -> bool override;

    auto as_int(int64_t& ret, FilterOperation op) -> bool override;

    auto as_bool(bool& ret, FilterOperation op) -> bool override;

    auto as_null(FilterOperation op) -> bool override;

    auto as_timestamp() -> bool override { return 0 != (m_types & LiteralType::TimestampT); }

    auto as_any(FilterOperation op) -> bool override;

    /**
     * Checks compatibility with a value type and comparison operation without converting the
     * literal. Numeric compatibility includes exact numeric interpretations and, for EQ/NEQ,
     * wildcard patterns whose characters could appear in the requested numeric type's text.
     * Wildcard compatibility is conservative and does not guarantee a match against any particular
     * value.
     * @param type The single value type to test.
     * @param op The comparison operation; existence operations are handled separately by callers.
     * @return Whether the literal is compatible with `type` under `op`.
     */
    [[nodiscard]] auto can_match_op(LiteralType type, FilterOperation op) const -> bool override;

    /**
     * @return Whether the literal contains an unescaped wildcard.
     */
    [[nodiscard]] auto has_wildcard() const -> bool override { return m_wildcard.exists; }

private:
    // Types
    /**
     * Wildcard presence and compatibility with numeric text.
     */
    struct WildcardProperties {
        // Whether the literal contains an unescaped wildcard.
        bool exists{false};
        // Whether a wildcard is present and the pattern passes the integer-character heuristic.
        bool matches_int{false};
        // Whether a wildcard is present and the pattern passes the float-character heuristic.
        bool matches_float{false};
    };

    // Constructors
    /**
     * Creates a literal with the supplied interpretations and wildcard properties.
     * @param types The compatible literal types.
     * @param string_value The string interpretation.
     * @param float_option The floating-point interpretation, if available.
     * @param bool_option The boolean interpretation, if available.
     * @param int_bounds_option The integer floor and ceiling, if representable.
     * @param wildcard_properties The wildcard presence and numeric compatibility.
     */
    ValueLiteral(
            literal_type_bitmask_t types,
            std::string string_value,
            std::optional<double> float_option,
            std::optional<bool> bool_option,
            std::optional<std::pair<int64_t, int64_t>> int_bounds_option,
            WildcardProperties wildcard_properties
    )
            : m_types{types},
              m_string{std::move(string_value)},
              m_float_option{float_option},
              m_bool_option{bool_option},
              m_int_bounds_option{std::move(int_bounds_option)},
              m_wildcard{wildcard_properties} {}

    // Data members
    literal_type_bitmask_t m_types{};
    std::string m_string;
    std::optional<double> m_float_option;
    std::optional<bool> m_bool_option;
    std::optional<std::pair<int64_t, int64_t>> m_int_bounds_option;
    WildcardProperties m_wildcard;
};
}  // namespace clp_s::search::ast

#endif  // CLP_S_SEARCH_VALUELITERAL_HPP
