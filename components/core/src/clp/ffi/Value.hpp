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
#include "../type_utils.hpp"

// NOTE: In this file, "primitive" doesn't refer to a C++ fundamental type (e.g. int) but instead
// refers to a value in a kv-pair that has no children (i.e. not an object/array).

namespace clp::ffi {
using value_int_t = int64_t;
using value_float_t = double;
using value_bool_t = bool;

/**
 * Tuple of all primitive value types.
 */
using PrimitiveValueTypes = std::tuple<
        value_int_t,
        value_float_t,
        value_bool_t,
        std::string,
        clp::ir::EightByteEncodedTextAst,
        clp::ir::FourByteEncodedTextAst>;

/**
 * Variant for all primitive value types.
 */
using PrimitiveValueVariant = tuple_to_variant<PrimitiveValueTypes>::Type;

/**
 * Template to validate whether the given type is a primitive value type.
 * @tparam T
 */
template <typename T>
constexpr bool cIsPrimitiveValueType = is_in_type_tuple<T, PrimitiveValueTypes>::value;

/**
 * Concept that defines primitive value types.
 */
template <typename T>
concept PrimitiveValueType = cIsPrimitiveValueType<T>;

/**
 * Concept that defines primitive value types that are also C++ fundamental types.
 */
template <typename T>
concept FundamentalPrimitiveValueType = cIsPrimitiveValueType<T> && std::is_fundamental_v<T>;

/**
 * Concept that defines move-constructable primitive value types.
 */
template <typename T>
concept MoveConstructablePrimitiveValueType
        = cIsPrimitiveValueType<T> && std::is_move_constructible_v<T>
          && (false == std::is_fundamental_v<T>);

/**
 * Template struct that converts a given type into an immutable view type. By default, the immutable
 * view type is the given type itself, meaning that the immutable view is a copy. Specialization is
 * needed when the immutable view type is a const reference or some other types.
 * @tparam T
 */
template <typename T>
struct ImmutableViewTypeConverter {
    using Type = T;
};

/**
 * Specializes `std::string`'s immutable view type as a `std::string_view`.
 */
template <>
struct ImmutableViewTypeConverter<std::string> {
    using Type = std::string_view;
};

/**
 * Specializes `clp::ir::EightByteEncodedTextAst`'s immutable view type as its const reference.
 */
template <>
struct ImmutableViewTypeConverter<clp::ir::EightByteEncodedTextAst> {
    using Type = clp::ir::EightByteEncodedTextAst const&;
};

/**
 * Specializes `clp::ir::FourByteEncodedTextAst`'s immutable view type as its const reference.
 */
template <>
struct ImmutableViewTypeConverter<clp::ir::FourByteEncodedTextAst> {
    using Type = clp::ir::FourByteEncodedTextAst const&;
};

/**
 * Template alias to the underlying typename of `ImmutableViewTypeConverter`.
 * @tparam T
 */
template <typename T>
using ImmutableViewType = typename ImmutableViewTypeConverter<T>::Type;

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
     * Constructs a `Value` by moving the given primitive value.
     * @tparam T The type of the value, which must be an rvalue to a move-constructable primitive
     * value type.
     * @param value
     */
    template <MoveConstructablePrimitiveValueType T>
    explicit Value(T&& value) : m_value{std::forward<T>(value)} {
        static_assert(std::is_rvalue_reference_v<decltype(value)>);
    }

    /**
     * @tparam T The type of the given value, which must be a fundamental primitive value type.
     * @param value
     */
    template <FundamentalPrimitiveValueType T>
    explicit Value(T value) : m_value{value} {}

    // Methods
    /**
     * @tparam T
     * @return Whether the underlying value is the given type.
     */
    template <PrimitiveValueType T>
    [[nodiscard]] auto is() const -> bool {
        return std::holds_alternative<T>(m_value);
    }

    /**
     * @tparam T
     * @return An immutable view of the underlying value if its type matches the given type.
     * @throw OperationFailed if the given type doesn't match the underlying value's type.
     */
    template <PrimitiveValueType T>
    [[nodiscard]] auto get_immutable_view() const -> ImmutableViewType<T> {
        if (false == is<T>()) {
            throw OperationFailed(
                    clp::ErrorCode_BadParam,
                    __FILE__,
                    __LINE__,
                    "The underlying value does not match the given type."
            );
        }
        return std::get<T>(m_value);
    }

    /**
     * @return Whether the underlying value is null.
     */
    [[nodiscard]] auto is_null() const -> bool {
        return std::holds_alternative<std::monostate>(m_value);
    }

private:
    PrimitiveValueVariant m_value{std::monostate{}};
};
}  // namespace clp::ffi

#endif  // CLP_FFI_VALUE_HPP
