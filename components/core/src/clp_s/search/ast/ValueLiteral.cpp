#include "ValueLiteral.hpp"

#include <cmath>
#include <cstdint>
#include <ios>
#include <limits>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>

#include <clp_s/search/ast/FilterOperation.hpp>
#include <clp_s/search/ast/Literal.hpp>
#include <clp_s/search/ast/SearchUtils.hpp>

namespace clp_s::search::ast {
namespace {
constexpr std::string_view cTrueString{"true"};
constexpr std::string_view cFalseString{"false"};
constexpr std::string_view cNullString{"null"};
constexpr std::string_view cMatchAnyString{"*"};

// -2^63, which is exactly representable as a double (unlike the `int64_t` max, 2^63 - 1, which has
// too many significant bits to be exactly represented as a double). The floor and ceiling of any
// double in [-2^63, 2^63) are representable as an `int64_t`.
constexpr double cInt64Min{static_cast<double>(std::numeric_limits<int64_t>::min())};

/**
 * Parses the entirety of a string as a number of type `T`.
 * @tparam T The numeric type to parse.
 * @param str
 * @return The parsed number, or `std::nullopt` if `str` isn't exactly a valid `T`.
 */
template <typename T>
auto parse_number(std::string const& str) -> std::optional<T>;

/**
 * @param op
 * @return Whether `op` is `FilterOperation::EQ` or `FilterOperation::NEQ`.
 */
auto is_equality_op(FilterOperation op) -> bool;

/**
 * Determines which string types a string can be interpreted as. A string containing spaces or
 * unescaped wildcards can match a CLP string, and a string without spaces can match a variable
 * string.
 * @param str
 * @return A bitmask of the matching string types.
 */
auto get_string_types(std::string_view str) -> literal_type_bitmask_t;

/**
 * @param v
 * @return An option containing a pair holding the floor and ceiling of `v`, or an empty option if
 * either the floor or ceiling of `v` is outside the range of `int64_t`.
 */
auto get_int_bounds(double v) -> std::optional<std::pair<int64_t, int64_t>>;

template <typename T>
auto parse_number(std::string const& str) -> std::optional<T> {
    T value{};
    std::istringstream ss{str};
    ss >> std::noskipws >> value;
    if (ss.fail() || false == ss.eof()) {
        return std::nullopt;
    }
    return value;
}

auto is_equality_op(FilterOperation op) -> bool {
    return FilterOperation::EQ == op || FilterOperation::NEQ == op;
}

auto get_string_types(std::string_view str) -> literal_type_bitmask_t {
    literal_type_bitmask_t types{};
    if (std::string_view::npos != str.find(' ')) {
        types |= LiteralType::ClpStringT;
    } else {
        types |= LiteralType::VarStringT;
    }
    if (has_unescaped_wildcards(str)) {
        types |= LiteralType::ClpStringT;
    }
    return types;
}

auto get_int_bounds(double v) -> std::optional<std::pair<int64_t, int64_t>> {
    if (v >= cInt64Min && v < -cInt64Min) {
        return std::pair{static_cast<int64_t>(std::floor(v)), static_cast<int64_t>(std::ceil(v))};
    }
    return std::nullopt;
}
}  // namespace

auto ValueLiteral::create_from_int(int64_t v) -> std::shared_ptr<Literal> {
    auto str{std::to_string(v)};
    auto const types{cIntegralTypes | LiteralType::TimestampT | get_string_types(str)};
    return std::shared_ptr<Literal>{new ValueLiteral{
            types,
            std::move(str),
            static_cast<double>(v),
            std::nullopt,
            std::pair{v, v}
    }};
}

auto ValueLiteral::create_from_float(double v) -> std::shared_ptr<Literal> {
    std::ostringstream ss;
    ss << v;
    auto str{ss.str()};
    auto const int_bounds_option{get_int_bounds(v)};
    literal_type_bitmask_t types{
            LiteralType::FloatT | LiteralType::TimestampT | get_string_types(str)
    };
    if (int_bounds_option.has_value()) {
        types |= LiteralType::IntegerT;
    }
    return std::shared_ptr<Literal>{
            new ValueLiteral{types, std::move(str), v, std::nullopt, int_bounds_option}
    };
}

auto ValueLiteral::create_from_bool(bool v) -> std::shared_ptr<Literal> {
    std::string str{v ? cTrueString : cFalseString};
    auto const types{LiteralType::BooleanT | get_string_types(str)};
    return std::shared_ptr<Literal>{
            new ValueLiteral{types, std::move(str), std::nullopt, v, std::nullopt}
    };
}

auto ValueLiteral::create_from_string(std::string_view v) -> std::shared_ptr<Literal> {
    std::string str{v};
    literal_type_bitmask_t types{get_string_types(str)};
    std::optional<double> float_option;
    std::optional<bool> bool_option;
    std::optional<std::pair<int64_t, int64_t>> int_bounds_option;
    std::optional<int64_t> int_upper_bound_option;
    if (auto const parsed_int_option{parse_number<int64_t>(str)}; parsed_int_option.has_value()) {
        types |= cIntegralTypes | LiteralType::TimestampT;
        float_option = static_cast<double>(parsed_int_option.value());
        int_bounds_option = {parsed_int_option.value(), parsed_int_option.value()};
    } else if (auto const parsed_float_option{parse_number<double>(str)};
               parsed_float_option.has_value())
    {
        types |= LiteralType::FloatT | LiteralType::TimestampT;
        float_option = parsed_float_option;
        int_bounds_option = get_int_bounds(parsed_float_option.value());
        if (int_bounds_option.has_value()) {
            types |= LiteralType::IntegerT;
        }
    }
    if (cTrueString == str || cFalseString == str) {
        types |= LiteralType::BooleanT;
        bool_option = cTrueString == str;
    }
    if (cNullString == str) {
        types |= LiteralType::NullT;
    }
    return std::shared_ptr<Literal>{
            new ValueLiteral{types, std::move(str), float_option, bool_option, int_bounds_option}
    };
}

void ValueLiteral::print() const {
    auto& os{get_print_stream()};
    if (0 != (m_types & (cIntegralTypes | LiteralType::BooleanT | LiteralType::NullT))) {
        os << m_string;
    } else {
        os << "\"" << m_string << "\"";
    }
}

auto ValueLiteral::as_clp_string(std::string& ret, FilterOperation op) -> bool {
    if (false == is_equality_op(op) || 0 == (m_types & LiteralType::ClpStringT)) {
        return false;
    }
    ret = m_string;
    return true;
}

auto ValueLiteral::as_var_string(std::string& ret, FilterOperation op) -> bool {
    if (false == is_equality_op(op) || 0 == (m_types & LiteralType::VarStringT)) {
        return false;
    }
    ret = m_string;
    return true;
}

auto ValueLiteral::as_float(double& ret, FilterOperation /*op*/) -> bool {
    if (false == m_float_option.has_value()) {
        return false;
    }
    ret = m_float_option.value();
    return true;
}

auto ValueLiteral::as_int(int64_t& ret, FilterOperation op) -> bool {
    if (false == m_int_bounds_option.has_value()) {
        return false;
    }
    switch (op) {
        case FilterOperation::LT:
        case FilterOperation::GTE:
            ret = m_int_bounds_option->second;
            return true;
        case FilterOperation::GT:
        case FilterOperation::LTE:
            ret = m_int_bounds_option->first;
            return true;
        default:
            if (m_int_bounds_option->first != m_int_bounds_option->second) {
                return false;
            }
            ret = m_int_bounds_option->first;
            return true;
    }
}

auto ValueLiteral::as_bool(bool& ret, FilterOperation op) -> bool {
    if (false == is_equality_op(op) || false == m_bool_option.has_value()) {
        return false;
    }
    ret = m_bool_option.value();
    return true;
}

auto ValueLiteral::as_null(FilterOperation op) -> bool {
    return is_equality_op(op) && 0 != (m_types & LiteralType::NullT);
}

auto ValueLiteral::as_any(FilterOperation op) -> bool {
    return is_equality_op(op) && cMatchAnyString == m_string;
}
}  // namespace clp_s::search::ast
