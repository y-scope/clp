#ifndef CLP_FFI_VALUE_HPP
#define CLP_FFI_VALUE_HPP

#include <cstdint>
#include <string>
#include <string_view>

#include "../ir/ClpString.hpp"

namespace clp::ffi {
using ValueInt = int64_t;
using ValueFloat = double;
using ValueBool = bool;
using ValueString = std::string;
using ValueEightByteEncodingClpString = clp::ir::EightByteEncodingClpString;
using ValueFourByteEncodingClpString = clp::ir::FourByteEncodingClpString;

/**
 * Template struct that converts a given type into an immutable view type. By default, the immutable
 * view type is the given type itself, meaning that the immutable view is a copy. Specialization is
 * needed when the immutable view type is a const reference or some other types.
 * @tparam ValueType
 */
template <typename ValueType>
struct ImmutableViewTypeConverter {
    using type = ValueType;
};

/**
 * Template alias to the underlying typename of `ImmutableViewTypeConverter`.
 * @tparam ValueType
 */
template <typename ValueType>
using ImmutableViewType = typename ImmutableViewTypeConverter<ValueType>::type;

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
}  // namespace clp::ffi

#endif  // CLP_FFI_VALUE_HPP
