#ifndef CLP_S_NUMERICCOMPARE_HPP
#define CLP_S_NUMERICCOMPARE_HPP

#include <cmath>
#include <cstdint>

namespace clp_s {
// `2^63`: exactly representable as a double and one past `INT64_MAX`. Any double `>=` this exceeds
// every `int64_t`.
constexpr double cInt64UpperBound{9223372036854775808.0};
// `-2^63 == INT64_MIN`, exactly representable as a double.
constexpr double cInt64Min{-9223372036854775808.0};

[[nodiscard]] inline auto is_less(int64_t lhs, int64_t rhs) -> bool {
    return lhs < rhs;
}

[[nodiscard]] inline auto is_less(double lhs, double rhs) -> bool {
    return lhs < rhs;
}

/**
 * Determines whether an integer is strictly less than a double without precision loss. Casting
 * `lhs` to a double would be lossy above `2^53`, so we instead range-check `rhs` against the
 * `int64_t` bounds and then compare integer parts exactly, letting the fractional part of `rhs`
 * break ties.
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
    // `rhs` is now in `[INT64_MIN, INT64_MAX]`, so `trunc(rhs)` casts to `int64_t` exactly.
    auto const truncated{std::trunc(rhs)};
    auto const rhs_int{static_cast<int64_t>(truncated)};
    if (lhs != rhs_int) {
        return lhs < rhs_int;
    }
    // Integer parts are equal; `lhs < rhs` iff `rhs` has a positive fractional part.
    return rhs > truncated;
}

/**
 * Determines whether a double is strictly less than an integer without precision loss. See the
 * `int64_t`/`double` overload for the rationale.
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
    // Integer parts are equal; `lhs < rhs` iff `lhs` has a negative fractional part.
    return lhs < truncated;
}
}  // namespace clp_s
#endif  // CLP_S_NUMERICCOMPARE_HPP
