#ifndef CLP_MATH_UTILS_HPP
#define CLP_MATH_UTILS_HPP

#include <type_traits>

/**
 * @tparam unsigned_t An unsigned integer type
 * @param val
 * @param factor Factor for the multiple. Cannot be 0.
 * @return The given value rounded up to the nearest multiple of the given factor
 */
template <typename unsigned_t>
auto int_round_up_to_multiple(unsigned_t val, unsigned_t factor) -> unsigned_t {
    static_assert(std::is_unsigned_v<unsigned_t>);
    // NOTE: "val + multiple" could overflow, but the "- 1" will undo the overflow since overflow
    // semantics are well-defined for unsigned integers.
    return ((val + factor - 1) / factor) * factor;
}

#endif  // CLP_MATH_UTILS_HPP
