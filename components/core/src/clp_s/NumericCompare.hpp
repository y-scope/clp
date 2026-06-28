#ifndef CLP_S_NUMERICCOMPARE_HPP
#define CLP_S_NUMERICCOMPARE_HPP

#include <cmath>
#include <cstdint>

// Exact less-than comparisons between `int64_t` and `double` values.
//
// Comparing the two types by casting the integer to a double is lossy: doubles can only represent
// integers exactly up to `2^53`, so larger `int64_t` values round and the comparison can give the
// wrong answer. The overloads below instead compare exactly across both types.
namespace clp_s {
// `2^63`: the smallest double too large to fit in an `int64_t` (exactly representable, one past
// `INT64_MAX`). Any double `>=` this is greater than every `int64_t`.
constexpr double cInt64UpperBound{9223372036854775808.0};
// `-2^63 == INT64_MIN` (exactly representable). Any double `<` this is less than every `int64_t`.
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
 *
 * Adapted from SQLite's `sqlite3IntFloatCompare`:
 * https://github.com/sqlite/sqlite/blob/master/src/vdbeaux.c
 * @param lhs
 * @param rhs
 * @return Whether `lhs < rhs`.
 */
[[nodiscard]] inline auto is_less(int64_t lhs, double rhs) -> bool {
    // NaN is unordered, so it is never less than anything.
    if (std::isnan(rhs)) {
        return false;
    }
    // `rhs` lies outside the `int64_t` range, so the result is determined by its sign.
    if (rhs >= cInt64UpperBound) {
        return true;
    }
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
 * Compares a double against an integer exactly. The mirror image of the `int64_t`/`double` overload;
 * see it for the rationale.
 * @param lhs
 * @param rhs
 * @return Whether `lhs < rhs`.
 */
[[nodiscard]] inline auto is_less(double lhs, int64_t rhs) -> bool {
    // NaN is unordered, so it is never less than anything.
    if (std::isnan(lhs)) {
        return false;
    }
    // `lhs` lies outside the `int64_t` range, so the result is determined by its sign.
    if (lhs >= cInt64UpperBound) {
        return false;
    }
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
#endif  // CLP_S_NUMERICCOMPARE_HPP
