#ifndef CLP_FFI_VALUE_HPP
#define CLP_FFI_VALUE_HPP

#include <cstdint>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

#include "../ErrorCode.hpp"
#include "../ir/EncodedTextAst.hpp"
#include "../TraceableException.hpp"

namespace clp::ffi {
using value_int_t = int64_t;
using value_float_t = double;
using value_bool_t = bool;

/**
 * Tuple of all the valid primitive value types.
 */
using PrimitiveValueTypeTuple = std::tuple<
        value_int_t,
        value_float_t,
        value_bool_t,
        std::string,
        clp::ir::EightByteEncodedTextAst,
        clp::ir::FourByteEncodedTextAst>;

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
 * Concept that defines primitive value types that are C++ fundamental types.
 */
template <typename Type>
concept FundamentalValueType = cIsValidPrimitiveValueType<Type> && std::is_fundamental_v<Type>;

/**
 * Concept that defines move-constructable primitive value types.
 */
template <typename Type>
concept MoveConstructableValueType
        = cIsValidPrimitiveValueType<Type> && std::is_move_constructible_v<Type>
          && (false == std::is_fundamental_v<Type>);

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
 * Specializes `std::string`'s immutable view type to `std::string_view`.
 */
template <>
struct ImmutableViewTypeConverter<std::string> {
    using type = std::string_view;
};

/**
 * Specializes `clp::ir::EightByteEncodedTextAst`'s immutable view type its const reference.
 */
template <>
struct ImmutableViewTypeConverter<clp::ir::EightByteEncodedTextAst> {
    using type = clp::ir::EightByteEncodedTextAst const&;
};

/**
 * Specializes `clp::ir::FourByteEncodedTextAst`'s immutable view type its const reference.
 */
template <>
struct ImmutableViewTypeConverter<clp::ir::FourByteEncodedTextAst> {
    using type = clp::ir::FourByteEncodedTextAst const&;
};

/**
 * Template alias to the underlying typename of `ImmutableViewTypeConverter`.
 * @tparam Type
 */
template <typename Type>
using ImmutableViewType = typename ImmutableViewTypeConverter<Type>::type;

class Value {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        OperationFailed(
                ErrorCode error_code,
                char const* const filename,
                int line_number,
                std::string message
        )
                : TraceableException{error_code, filename, line_number},
                  m_message{std::move(message)} {}

        [[nodiscard]] auto what() const noexcept -> char const* override {
            return m_message.c_str();
        }

    private:
        std::string m_message;
    };

    // Constructors
    Value() = default;

    /**
     * Move constructs from the given rvalue.
     * @tparam PrimitiveValue The type of the rvalue, which must be a move constructable
     * non-reference type.
     * @value
     */
    template <MoveConstructableValueType PrimitiveValue>
    requires(false == std::is_reference_v<PrimitiveValue>)
    Value(PrimitiveValue&& value) : m_value{std::forward<PrimitiveValue>(value)} {}

    /**
     * Constructs from the given fundamental-type value.
     * @tparam PrimitiveValue The type of the given value, which must be a C++ fundamental type.
     * @value
     */
    template <FundamentalValueType PrimitiveValue>
    Value(PrimitiveValue value) : m_value{value} {}

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
     * @throw `OperationFailed` if the given type doesn't match the underlying value's type.
     */
    template <PrimitiveValueType PrimitiveValue>
    [[nodiscard]] auto get_immutable_view() const -> ImmutableViewType<PrimitiveValue> {
        if (false == is<PrimitiveValue>()) {
            throw OperationFailed(
                    clp::ErrorCode_BadParam,
                    __FILE__,
                    __LINE__,
                    "The underlying value does not match the query type."
            );
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
