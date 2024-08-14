#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

#include <Catch2/single_include/catch2/catch.hpp>

#include "../src/clp/ffi/encoding_methods.hpp"
#include "../src/clp/ffi/Value.hpp"
#include "../src/clp/ir/EncodedTextAst.hpp"
#include "../src/clp/ir/types.hpp"

using clp::ffi::Value;
using clp::ffi::value_bool_t;
using clp::ffi::value_float_t;
using clp::ffi::value_int_t;
using clp::ir::eight_byte_encoded_variable_t;
using clp::ir::EightByteEncodedTextAst;
using clp::ir::four_byte_encoded_variable_t;
using clp::ir::FourByteEncodedTextAst;
using std::string;
using std::vector;

namespace {
/**
 * Parses and encodes the given string as an instance of `EncodedTextAst`.
 * @tparam encoded_variable_t
 * @param text
 * @return The encoded result.
 */
template <typename encoded_variable_t>
requires(std::is_same_v<encoded_variable_t, eight_byte_encoded_variable_t>
         || std::is_same_v<encoded_variable_t, four_byte_encoded_variable_t>)
[[nodiscard]] auto get_encoded_text_ast(std::string_view text
) -> clp::ir::EncodedTextAst<encoded_variable_t>;

/**
 * Tests that `Value::is` returns true for the given type and false for all others.
 * @tparam Type The type to query.
 * @param value The value to test against.
 */
template <typename Type>
auto test_value_is(Value const& value) -> void;

/**
 * Tests `Value::get_immutable_view` either:
 * 1. returns the expected value with the expected type for the given type and value;
 * 2. throws for any other type.
 * @tparam Type The type to query.
 * @param value The value to test against.
 * @param typed_value The typed value to compare with.
 */
template <typename Type>
auto test_value_get_immutable_view(Value const& value, Type const& typed_value) -> void;

// Implementation

template <typename encoded_variable_t>
requires(std::is_same_v<encoded_variable_t, eight_byte_encoded_variable_t>
         || std::is_same_v<encoded_variable_t, four_byte_encoded_variable_t>)
auto get_encoded_text_ast(std::string_view text) -> clp::ir::EncodedTextAst<encoded_variable_t> {
    string logtype;
    vector<encoded_variable_t> encoded_vars;
    vector<int32_t> dict_var_bounds;
    REQUIRE(clp::ffi::encode_message(text, logtype, encoded_vars, dict_var_bounds));
    REQUIRE(((dict_var_bounds.size() % 2) == 0));

    vector<string> dict_vars;
    for (size_t i{0}; i < dict_var_bounds.size(); i += 2) {
        auto const begin_pos{static_cast<size_t>(dict_var_bounds[i])};
        auto const end_pos{static_cast<size_t>(dict_var_bounds[i + 1])};
        dict_vars.emplace_back(text.cbegin() + begin_pos, text.cbegin() + end_pos);
    }

    return clp::ir::EncodedTextAst<encoded_variable_t>{logtype, dict_vars, encoded_vars};
}

template <typename Type>
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
auto test_value_is(Value const& value) -> void {
    REQUIRE((std::is_same_v<std::monostate, Type> == value.is_null()));
    REQUIRE((std::is_same_v<value_int_t, Type> == value.is<value_int_t>()));
    REQUIRE((std::is_same_v<value_float_t, Type> == value.is<value_float_t>()));
    REQUIRE((std::is_same_v<value_bool_t, Type> == value.is<value_bool_t>()));
    REQUIRE((std::is_same_v<string, Type> == value.is<string>()));
    REQUIRE((std::is_same_v<EightByteEncodedTextAst, Type> == value.is<EightByteEncodedTextAst>()));
    REQUIRE((std::is_same_v<FourByteEncodedTextAst, Type> == value.is<FourByteEncodedTextAst>()));
}

template <typename Type>
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
auto test_value_get_immutable_view(Value const& value, Type const& typed_value) -> void {
    if constexpr (std::is_same_v<value_int_t, Type>) {
        REQUIRE((value.get_immutable_view<Type>() == typed_value));
        REQUIRE((std::is_same_v<Type, decltype(value.get_immutable_view<Type>())>));
    } else {
        REQUIRE_THROWS(value.get_immutable_view<value_int_t>());
    }

    if constexpr (std::is_same_v<value_float_t, Type>) {
        REQUIRE((value.get_immutable_view<Type>() == typed_value));
        REQUIRE((std::is_same_v<Type, decltype(value.get_immutable_view<Type>())>));
    } else {
        REQUIRE_THROWS(value.get_immutable_view<value_float_t>());
    }

    if constexpr (std::is_same_v<value_bool_t, Type>) {
        REQUIRE((value.get_immutable_view<Type>() == typed_value));
        REQUIRE((std::is_same_v<Type, decltype(value.get_immutable_view<Type>())>));
    } else {
        REQUIRE_THROWS(value.get_immutable_view<value_bool_t>());
    }

    if constexpr (std::is_same_v<string, Type>) {
        REQUIRE((value.get_immutable_view<Type>() == typed_value));
        REQUIRE((std::is_same_v<std::string_view, decltype(value.get_immutable_view<Type>())>));
    } else {
        REQUIRE_THROWS(value.get_immutable_view<string>());
    }

    if constexpr (std::is_same_v<EightByteEncodedTextAst, Type>) {
        REQUIRE((value.get_immutable_view<Type>() == typed_value));
        REQUIRE((std::is_same_v<
                 EightByteEncodedTextAst const&,
                 decltype(value.get_immutable_view<Type>())>));
    } else {
        REQUIRE_THROWS(value.get_immutable_view<EightByteEncodedTextAst>());
    }

    if constexpr (std::is_same_v<FourByteEncodedTextAst, Type>) {
        REQUIRE((value.get_immutable_view<Type>() == typed_value));
        REQUIRE((std::is_same_v<
                 FourByteEncodedTextAst const&,
                 decltype(value.get_immutable_view<Type>())>));
    } else {
        REQUIRE_THROWS(value.get_immutable_view<FourByteEncodedTextAst>());
    }
}
}  // namespace

TEST_CASE("ffi_Value_basic", "[ffi][Value]") {
    Value const null_value;
    test_value_is<std::monostate>(null_value);
    test_value_get_immutable_view<std::monostate>(null_value, std::monostate{});

    constexpr value_int_t cIntVal{1000};
    Value const int_value{cIntVal};
    test_value_is<value_int_t>(int_value);
    test_value_get_immutable_view<value_int_t>(int_value, cIntVal);

    constexpr value_float_t cFloatValue{1000.0001};
    Value const float_value{cFloatValue};
    test_value_is<value_float_t>(float_value);
    test_value_get_immutable_view<value_float_t>(float_value, cFloatValue);

    constexpr value_bool_t cBoolVal{false};
    Value const bool_value{cBoolVal};
    test_value_is<value_bool_t>(bool_value);
    test_value_get_immutable_view<value_bool_t>(bool_value, cBoolVal);

    constexpr std::string_view cStringVal{"This is a test string message"};
    Value const string_value{string{cStringVal}};
    test_value_is<string>(string_value);
    test_value_get_immutable_view<string>(string_value, string{cStringVal});

    constexpr std::string_view cStringToEncode{"uid=0, CPU usage: 99.99%, \"user_name\"=YScope"};
    Value const eight_byte_encoded_text_ast_value{
            get_encoded_text_ast<eight_byte_encoded_variable_t>(cStringToEncode)
    };
    test_value_is<EightByteEncodedTextAst>(eight_byte_encoded_text_ast_value);
    test_value_get_immutable_view<EightByteEncodedTextAst>(
            eight_byte_encoded_text_ast_value,
            get_encoded_text_ast<eight_byte_encoded_variable_t>(cStringToEncode)
    );

    Value const four_byte_encoded_text_ast_value{
            get_encoded_text_ast<four_byte_encoded_variable_t>(cStringToEncode)
    };
    test_value_is<FourByteEncodedTextAst>(four_byte_encoded_text_ast_value);
    test_value_get_immutable_view<FourByteEncodedTextAst>(
            four_byte_encoded_text_ast_value,
            get_encoded_text_ast<four_byte_encoded_variable_t>(cStringToEncode)
    );
}
