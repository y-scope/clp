#ifndef TYPE_UTILS_HPP
#define TYPE_UTILS_HPP

// C++ standard libraries
#include <cstring>
#include <type_traits>

/**
 * Gets the underlying type of the given enum
 * @tparam T
 * @param enum_member
 * @return The underlying type of the given enum
 */
template <typename T>
constexpr typename std::underlying_type<T>::type enum_to_underlying_type(T enum_member) {
    return static_cast<typename std::underlying_type<T>::type>(enum_member);
}

/**
 * Cast between types by copying the exact bit representation. This avoids
 * issues with strict type aliasing. This method should be removed when we
 * switch to C++20.
 * @tparam Destination
 * @tparam Source
 * @param src
 * @return
 */
template <class Destination, class Source>
std::enable_if_t<sizeof(Destination) == sizeof(Source) &&
        std::is_trivially_copyable_v<Destination> &&
        std::is_trivially_copyable_v<Source> &&
        std::is_trivially_constructible_v<Destination>, Destination
> bit_cast (const Source& src) {
    Destination dst;
    std::memcpy(&dst, &src, sizeof(Destination));
    return dst;
}

/**
 * Helper for defining std::variant overloads inline, using lambdas
 * @tparam Ts The types of the variant that will be deduced using the deduction
 * guide below
 */
template <class... Ts>
struct overloaded : Ts ... {
    using Ts::operator()...;
};
/**
 * Explicit deduction guide for the types passed to the methods in the
 * overloaded helper
 */
template <class... Ts> overloaded (Ts...) -> overloaded<Ts...>;

#endif // TYPE_UTILS_HPP
