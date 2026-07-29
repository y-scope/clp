#ifndef CLP_S_INT_FLOAT_COMPARE_HPP
#define CLP_S_INT_FLOAT_COMPARE_HPP

#include <cmath>
#include <cstdint>

// Exact less-than comparisons between `int64_t` and `double` values.
//
// A double can represent every integer exactly only up to `2^53`. Beyond that the representable
// integers have gaps, so casting a larger `int64_t` to a double snaps it to the nearest value a
// double can hold (e.g. `2^53 + 1` becomes `2^53`). Comparing an `int64_t` and a double through
// that cast can therefore return the wrong result. The overloads below compare the two types
// exactly, without casting.
//
// Adapted from SQLite's `sqlite3IntFloatCompare`:
// https://github.com/sqlite/sqlite/blob/master/src/vdbeaux.c
namespace clp_s {
// Doubles span a far wider range than `int64_t`. These constants bracket the `int64_t` range,
// letting the comparisons below detect out-of-range doubles before converting them to `int64_t`
// (which would overflow).

// `2^63`, one greater than `INT64_MAX`: any double `>=` this is larger than every `int64_t`. We use
// `2^63` rather than `INT64_MAX` because `2^63` is exactly representable as a double and
// `INT64_MAX` is not.
constexpr double cInt64UpperBound{9223372036854775808.0};

// `-2^63`, which equals `INT64_MIN` and is exactly representable as a double: any double `<` this
// is smaller than every `int64_t`.
constexpr double cInt64Min{-9223372036854775808.0};

[[nodiscard]] inline auto is_less(int64_t lhs, int64_t rhs) -> bool {
    return lhs < rhs;
}

[[nodiscard]] inline auto is_less(double lhs, double rhs) -> bool {
    return lhs < rhs;
}

/**
 * Compares an integer and a double exactly, without a cast that would lose precision. If `rhs` is
 * greater than every `int64_t`, `lhs < rhs` is true; if `rhs` is less than every `int64_t`, it is
 * false. Otherwise `rhs` is truncated to a whole number and compared to `lhs`; on a tie, `lhs` is
 * smaller only when `rhs` has a positive fractional part.
 * @param lhs
 * @param rhs
 * @return Whether `lhs < rhs`.
 */
[[nodiscard]] inline auto is_less(int64_t lhs, double rhs) -> bool {
    if (std::isnan(rhs)) {
        return false;
    }
    if (rhs >= cInt64UpperBound) {
        return true;
    }
    if (rhs < cInt64Min) {
        return false;
    }
    auto const truncated{std::trunc(rhs)};
    auto const rhs_int{static_cast<int64_t>(truncated)};
    if (lhs != rhs_int) {
        return lhs < rhs_int;
    }
    return rhs > truncated;
}

/**
 * Compares a double and an integer exactly, without a cast that would lose precision. If `lhs` is
 * greater than every `int64_t`, `lhs < rhs` is false; if `lhs` is less than every `int64_t`, it is
 * true. Otherwise `lhs` is truncated to a whole number and compared to `rhs`; on a tie, `lhs` is
 * smaller only when it has a negative fractional part.
 * @param lhs
 * @param rhs
 * @return Whether `lhs < rhs`.
 */
[[nodiscard]] inline auto is_less(double lhs, int64_t rhs) -> bool {
    if (std::isnan(lhs)) {
        return false;
    }
    if (lhs >= cInt64UpperBound) {
        return false;
    }
    if (lhs < cInt64Min) {
        return true;
    }
    auto const truncated{std::trunc(lhs)};
    auto const lhs_int{static_cast<int64_t>(truncated)};
    if (lhs_int != rhs) {
        return lhs_int < rhs;
    }
    return lhs < truncated;
}
}  // namespace clp_s

#endif  // CLP_S_INT_FLOAT_COMPARE_HPP
