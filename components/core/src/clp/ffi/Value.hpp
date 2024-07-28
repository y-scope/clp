#ifndef CLP_FFI_VALUE_HPP
#define CLP_FFI_VALUE_HPP

#include <cstdint>
#include <string>
#include <string_view>
#include <system_error>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

#include <outcome/single-header/outcome.hpp>

#include "../ir/ClpString.hpp"

namespace clp::ffi {
using ValueInt = int64_t;
using ValueFloat = double;
using ValueBool = bool;
using ValueString = std::string;
using ValueEightByteEncodingClpString = clp::ir::EightByteEncodingClpString;
using ValueFourByteEncodingClpString = clp::ir::FourByteEncodingClpString;

/**
 * Tuple of all the valid primitive value types.
 */
using PrimitiveValueTypeTuple = std::tuple<
        ValueInt,
        ValueFloat,
        ValueBool,
        ValueString,
        ValueEightByteEncodingClpString,
        ValueFourByteEncodingClpString>;

/**
 * Template that converts a tuple of types into a variant.
 * @tparam Tuple A tuple of types
 */
template <typename Tuple>
struct tuple_to_variant;

template <typename... Types>
struct tuple_to_variant<std::tuple<Types...>> {
    using type = std::variant<std::monostate, Types...>;
};

/**
 * Variant type defined by the tuple of all primitive value types.
 */
using PrimitiveValueTypeVariant = typename tuple_to_variant<PrimitiveValueTypeTuple>::type;

/**
 * Template to validate if the given type is in the type tuple.
 * @tparam Type
 * @tparam Tuple
 */
template <typename Type, typename Tuple>
struct is_valid_type;

template <typename Type, typename... Types>
struct is_valid_type<Type, std::tuple<Types...>> : std::disjunction<std::is_same<Type, Types>...> {
};

/**
 * Template to validate whether the given type is a valid primitive value type.
 * @tparam Type
 */
template <typename Type>
constexpr bool cIsValidPrimitiveValueType = is_valid_type<Type, PrimitiveValueTypeTuple>::value;

/**
 * Concept that defines primitive value types.
 */
template <typename Type>
concept PrimitiveValueType = cIsValidPrimitiveValueType<Type>;

/**
 * Concept that defines copy-constructable primitive value types.
 */
template <typename Type>
concept CopyConstructablePrimitiveValueType
        = cIsValidPrimitiveValueType<Type> && std::is_copy_constructible_v<Type>;

/**
 * Concept that defines move-constructable primitive value types.
 */
template <typename Type>
concept MoveConstructablePrimitiveValueType
        = cIsValidPrimitiveValueType<Type> && std::is_move_constructible_v<Type>;

/**
 * Template struct that converts a given type into an immutable view type. By default, the immutable
 * view type is the given type itself, meaning that the immutable view is a copy. Specialization is
 * needed when the immutable view type is a const reference or some other types.
 * @tparam Type
 */
template <typename Type>
struct ImmutableViewTypeConverter {
    using type = Type;
};

/**
 * Template alias to the underlying typename of `ImmutableViewTypeConverter`.
 * @tparam Type
 */
template <typename Type>
using ImmutableViewType = typename ImmutableViewTypeConverter<Type>::type;

/**
 * Specializes `ValueString`'s immutable view type to `std::string_view`.
 */
template <>
struct ImmutableViewTypeConverter<ValueString> {
    using type = std::string_view;
};

/**
 * Specializes `ValueEightByteEncodingClpString`'s immutable view type its const reference.
 */
template <>
struct ImmutableViewTypeConverter<ValueEightByteEncodingClpString> {
    using type = ValueEightByteEncodingClpString const&;
};

/**
 * Specializes `ValueFourByteEncodingClpString`'s immutable view type its const reference.
 */
template <>
struct ImmutableViewTypeConverter<ValueFourByteEncodingClpString> {
    using type = ValueFourByteEncodingClpString const&;
};

class Value {
public:
    // Constructors
    Value() = default;

    template <MoveConstructablePrimitiveValueType PrimitiveValue>
    Value(PrimitiveValue&& value) : m_value{std::forward(value)} {}

    template <CopyConstructablePrimitiveValueType PrimitiveValue>
    Value(PrimitiveValue const& value) : m_value{value} {}

    // Disable copy constructor and assignment operator
    Value(Value const&) = delete;
    auto operator=(Value const&) -> Value& = delete;

    // Default move constructor and assignment operator
    Value(Value&&) = default;
    auto operator=(Value&&) -> Value& = default;

    // Destructor
    ~Value() = default;

    // Methods
    /**
     * @tparam PrimitiveValue
     * @return Whether the underlying value is the given `PrimitiveValue` type.
     */
    template <PrimitiveValueType PrimitiveValue>
    [[nodiscard]] auto is() const -> bool {
        return std::holds_alternative<PrimitiveValue>(m_value);
    }

    /**
     * @tparam PrimitiveValue
     * @return An immutable view of the underlying value if its type matches `PrimitiveValue` type.
     * @return `std::errc::wrong_protocol_type` if the type doesn't match.
     */
    template <PrimitiveValueType PrimitiveValue>
    [[nodiscard]] auto get_immutable_view(
    ) const -> OUTCOME_V2_NAMESPACE::std_result<ImmutableViewType<PrimitiveValue>> {
        if (false == is<PrimitiveValue>()) {
            return std::errc::wrong_protocol_type;
        }
        return std::get<PrimitiveValue>(m_value);
    }

    /**
     * @return Whether the underlying value is null.
     */
    [[nodiscard]] auto is_null() const -> bool {
        return std::holds_alternative<std::monostate>(m_value);
    }

private:
    PrimitiveValueTypeVariant m_value{std::monostate{}};
};
}  // namespace clp::ffi

#endif  // CLP_FFI_VALUE_HPP
