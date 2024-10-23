#ifndef CLP_TYPE_UTILS_HPP
#define CLP_TYPE_UTILS_HPP

#include <cstring>
#include <tuple>
#include <type_traits>
#include <variant>

namespace clp {
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

/**
 * Template that converts a tuple of types into a variant.
 * @tparam Tuple A tuple of types
 */
template <typename Tuple>
struct tuple_to_variant;

template <typename... Types>
struct tuple_to_variant<std::tuple<Types...>> {
    using Type = std::variant<std::monostate, Types...>;
};

/**
 * Template to validate if the given type is in the given tuple of types.
 * @tparam Type
 * @tparam Tuple
 */
template <typename Type, typename Tuple>
struct is_in_type_tuple;

template <typename Type, typename... Types>
struct is_in_type_tuple<Type, std::tuple<Types...>>
        : std::disjunction<std::is_same<Type, Types>...> {};

/**
 * Concept for integer types.
 */
template <typename integer_t>
concept IntegerType = std::is_integral_v<integer_t> && false == std::is_same_v<integer_t, bool>;

/**
 * Concept for signed integer types.
 */
template <typename integer_t>
concept SignedIntegerType = IntegerType<integer_t> && std::is_signed_v<integer_t>;
}  // namespace clp

#endif  // CLP_TYPE_UTILS_HPP
