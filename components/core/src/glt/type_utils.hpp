#ifndef GLT_TYPE_UTILS_HPP
#define GLT_TYPE_UTILS_HPP

#include <cstring>
#include <type_traits>

namespace glt {
/**
 * An empty type which can be used to declare variables conditionally based on template parameters
 */
struct EmptyType {};

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
 * Cast between types by copying the exact bit representation. This avoids issues with strict type
 * aliasing. This method should be removed when we switch to C++20.
 * @tparam Destination
 * @tparam Source
 * @param src
 * @return
 */
template <class Destination, class Source>
std::enable_if_t<
        sizeof(Destination) == sizeof(Source)
                && std::is_trivially_copyable_v<Destination> && std::is_trivially_copyable_v<Source>
                && std::is_trivially_constructible_v<Destination>,
        Destination>
bit_cast(Source const& src) {
    Destination dst;
    std::memcpy(&dst, &src, sizeof(Destination));
    return dst;
}

/**
 * Helper for defining std::variant overloads inline, using lambdas
 * @tparam Ts The types of the variant that will be deduced using the deduction guide below
 */
template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
/**
 * Explicit deduction guide for the types passed to the methods in the overloaded helper
 */
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

/**
 * Cast between pointers after ensuring the source and destination types are the same size
 * @tparam Destination The destination type
 * @tparam Source The source type
 * @param src The source pointer
 * @return The casted pointer
 */
template <typename Destination, class Source>
std::enable_if_t<sizeof(Destination) == sizeof(Source), Destination*>
size_checked_pointer_cast(Source* src) {
    return reinterpret_cast<Destination*>(src);
}
}  // namespace glt

#endif  // GLT_TYPE_UTILS_HPP
