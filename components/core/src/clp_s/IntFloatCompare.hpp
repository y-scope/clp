#ifndef CLP_S_INTFLOATCOMPARE_HPP
#define CLP_S_INTFLOATCOMPARE_HPP

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
// `-2^63`, equal to `INT64_MIN`: any double `<` this is smaller than every `int64_t`. Both `-2^63`
// and `INT64_MIN` are the same value and are exactly representable as a double.
constexpr double cInt64Min{-9223372036854775808.0};

[[nodiscard]] inline auto is_less(int64_t lhs, int64_t rhs) -> bool {
    return lhs < rhs;
}

[[nodiscard]] inline auto is_less(double lhs, double rhs) -> bool {
    return lhs < rhs;
}

/**
 * Compares an integer against a double exactly. After ruling out doubles that fall outside the
 * `int64_t` range, `rhs` is split into its integer and fractional parts: the integer parts are
 * compared directly, and the fractional part of `rhs` breaks ties.
 * @param lhs
 * @param rhs
 * @return Whether `lhs < rhs`.
 */
[[nodiscard]] inline auto is_less(int64_t lhs, double rhs) -> bool {
    // NaN is unordered, so it is never less than anything.
    if (std::isnan(rhs)) {
        return false;
    }
    // `rhs` is larger than every `int64_t`, so `lhs` must be less than it.
    if (rhs >= cInt64UpperBound) {
        return true;
    }
    // `rhs` is smaller than every `int64_t`, so `lhs` cannot be less than it.
    if (rhs < cInt64Min) {
        return false;
    }
    // `rhs` is within the `int64_t` range, so truncating it toward zero and casting is exact.
    auto const truncated{std::trunc(rhs)};
    auto const rhs_int{static_cast<int64_t>(truncated)};
    if (lhs != rhs_int) {
        return lhs < rhs_int;
    }
    // Same integer part: `lhs < rhs` exactly when `rhs` has a positive fractional part.
    return rhs > truncated;
}

/**
 * Compares a double against an integer exactly.
 * @param lhs
 * @param rhs
 * @return Whether `lhs < rhs`.
 */
[[nodiscard]] inline auto is_less(double lhs, int64_t rhs) -> bool {
    // NaN is unordered, so it is never less than anything.
    if (std::isnan(lhs)) {
        return false;
    }
    // `lhs` is larger than every `int64_t`, so it cannot be less than `rhs`.
    if (lhs >= cInt64UpperBound) {
        return false;
    }
    // `lhs` is smaller than every `int64_t`, so it must be less than `rhs`.
    if (lhs < cInt64Min) {
        return true;
    }
    // `lhs` is within the `int64_t` range, so truncating it toward zero and casting is exact.
    auto const truncated{std::trunc(lhs)};
    auto const lhs_int{static_cast<int64_t>(truncated)};
    if (lhs_int != rhs) {
        return lhs_int < rhs;
    }
    // Same integer part: `lhs < rhs` exactly when `lhs` has a negative fractional part.
    return lhs < truncated;
}
}  // namespace clp_s

#endif  // CLP_S_INTFLOATCOMPARE_HPP
