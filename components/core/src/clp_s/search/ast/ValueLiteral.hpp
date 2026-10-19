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
    auto matches_type(LiteralType type) -> bool override { return 0 != (m_types & type); }

    auto matches_any(literal_type_bitmask_t mask) -> bool override { return 0 != (m_types & mask); }

    auto matches_exactly(literal_type_bitmask_t mask) -> bool override { return m_types == mask; }

    // Methods overriding Literal
    auto as_clp_string(std::string& ret, FilterOperation op) -> bool override;

    auto as_var_string(std::string& ret, FilterOperation op) -> bool override;

    auto as_float(double& ret, FilterOperation op) -> bool override;

    auto as_int(int64_t& ret, FilterOperation op) -> bool override;

    auto as_bool(bool& ret, FilterOperation op) -> bool override;

    auto as_null(FilterOperation op) -> bool override;

    auto as_timestamp() -> bool override { return 0 != (m_types & LiteralType::TimestampT); }

    auto as_any(FilterOperation op) -> bool override;

private:
    // Constructor
    ValueLiteral(
            literal_type_bitmask_t types,
            std::string string_value,
            std::optional<double> float_option,
            std::optional<bool> bool_option,
            std::optional<std::pair<int64_t, int64_t>> int_bounds_option
    )
            : m_types{types},
              m_string{std::move(string_value)},
              m_float_option{float_option},
              m_bool_option{bool_option},
              m_int_bounds_option{int_bounds_option} {}

    // Data members
    literal_type_bitmask_t m_types{};
    std::string m_string;
    std::optional<double> m_float_option{};
    std::optional<bool> m_bool_option{};
    std::optional<std::pair<int64_t, int64_t>> m_int_bounds_option{};
};
}  // namespace clp_s::search::ast

#endif  // CLP_S_SEARCH_VALUELITERAL_HPP
