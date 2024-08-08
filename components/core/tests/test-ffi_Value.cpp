#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>
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
}  // namespace

TEST_CASE("ffi_Value_basic", "[ffi][Value]") {
    Value const null_value;
    REQUIRE(null_value.is_null());
    REQUIRE_FALSE(null_value.is<value_int_t>());
    REQUIRE_FALSE(null_value.is<value_float_t>());
    REQUIRE_FALSE(null_value.is<value_bool_t>());
    REQUIRE_FALSE(null_value.is<string>());
    REQUIRE_FALSE(null_value.is<EightByteEncodedTextAst>());
    REQUIRE_FALSE(null_value.is<FourByteEncodedTextAst>());
    REQUIRE_THROWS(null_value.get_immutable_view<value_int_t>());
    REQUIRE_THROWS(null_value.get_immutable_view<value_float_t>());
    REQUIRE_THROWS(null_value.get_immutable_view<value_bool_t>());
    REQUIRE_THROWS(null_value.get_immutable_view<string>());
    REQUIRE_THROWS(null_value.get_immutable_view<EightByteEncodedTextAst>());
    REQUIRE_THROWS(null_value.get_immutable_view<FourByteEncodedTextAst>());

    constexpr value_int_t cIntVal{1000};
    Value const int_value{cIntVal};
    REQUIRE(int_value.is<value_int_t>());
    REQUIRE((int_value.get_immutable_view<value_int_t>() == cIntVal));
    REQUIRE_FALSE(int_value.is_null());
    REQUIRE_FALSE(int_value.is<value_float_t>());
    REQUIRE_FALSE(int_value.is<value_bool_t>());
    REQUIRE_FALSE(int_value.is<string>());
    REQUIRE_FALSE(int_value.is<EightByteEncodedTextAst>());
    REQUIRE_FALSE(int_value.is<FourByteEncodedTextAst>());
    REQUIRE_THROWS(int_value.get_immutable_view<value_float_t>());
    REQUIRE_THROWS(int_value.get_immutable_view<value_bool_t>());
    REQUIRE_THROWS(int_value.get_immutable_view<string>());
    REQUIRE_THROWS(int_value.get_immutable_view<EightByteEncodedTextAst>());
    REQUIRE_THROWS(int_value.get_immutable_view<FourByteEncodedTextAst>());

    constexpr value_float_t cFloatValue{1000.0001};
    Value const float_value{cFloatValue};
    REQUIRE(float_value.is<value_float_t>());
    REQUIRE((float_value.get_immutable_view<value_float_t>() == cFloatValue));
    REQUIRE_FALSE(float_value.is_null());
    REQUIRE_FALSE(float_value.is<value_int_t>());
    REQUIRE_FALSE(float_value.is<value_bool_t>());
    REQUIRE_FALSE(float_value.is<string>());
    REQUIRE_FALSE(float_value.is<EightByteEncodedTextAst>());
    REQUIRE_FALSE(float_value.is<FourByteEncodedTextAst>());
    REQUIRE_THROWS(float_value.get_immutable_view<value_int_t>());
    REQUIRE_THROWS(float_value.get_immutable_view<value_bool_t>());
    REQUIRE_THROWS(float_value.get_immutable_view<string>());
    REQUIRE_THROWS(float_value.get_immutable_view<EightByteEncodedTextAst>());
    REQUIRE_THROWS(float_value.get_immutable_view<FourByteEncodedTextAst>());

    constexpr value_bool_t cBoolVal{false};
    Value const bool_value{cBoolVal};
    REQUIRE(bool_value.is<value_bool_t>());
    REQUIRE((bool_value.get_immutable_view<value_bool_t>() == cBoolVal));
    REQUIRE_FALSE(bool_value.is_null());
    REQUIRE_FALSE(bool_value.is<value_int_t>());
    REQUIRE_FALSE(bool_value.is<value_float_t>());
    REQUIRE_FALSE(bool_value.is<string>());
    REQUIRE_FALSE(bool_value.is<EightByteEncodedTextAst>());
    REQUIRE_FALSE(bool_value.is<FourByteEncodedTextAst>());
    REQUIRE_THROWS(bool_value.get_immutable_view<value_int_t>());
    REQUIRE_THROWS(bool_value.get_immutable_view<value_float_t>());
    REQUIRE_THROWS(bool_value.get_immutable_view<string>());
    REQUIRE_THROWS(bool_value.get_immutable_view<EightByteEncodedTextAst>());
    REQUIRE_THROWS(bool_value.get_immutable_view<FourByteEncodedTextAst>());

    constexpr std::string_view cStringVal{"This is a test string message"};
    Value const string_value{string{cStringVal}};
    REQUIRE(string_value.is<string>());
    REQUIRE((string_value.get_immutable_view<string>() == cStringVal));
    REQUIRE_FALSE(string_value.is_null());
    REQUIRE_FALSE(string_value.is<value_int_t>());
    REQUIRE_FALSE(string_value.is<value_float_t>());
    REQUIRE_FALSE(string_value.is<value_bool_t>());
    REQUIRE_FALSE(string_value.is<EightByteEncodedTextAst>());
    REQUIRE_FALSE(string_value.is<FourByteEncodedTextAst>());
    REQUIRE_THROWS(string_value.get_immutable_view<value_int_t>());
    REQUIRE_THROWS(string_value.get_immutable_view<value_float_t>());
    REQUIRE_THROWS(string_value.get_immutable_view<value_bool_t>());
    REQUIRE_THROWS(string_value.get_immutable_view<EightByteEncodedTextAst>());
    REQUIRE_THROWS(string_value.get_immutable_view<FourByteEncodedTextAst>());

    constexpr std::string_view cStringToEncode{"uid=0, CPU usage: 99.99%, \"user_name\"=YScope"};
    Value const eight_byte_encoded_text_ast_value{
            get_encoded_text_ast<eight_byte_encoded_variable_t>(cStringToEncode)
    };
    REQUIRE(eight_byte_encoded_text_ast_value.is<EightByteEncodedTextAst>());
    REQUIRE_FALSE(eight_byte_encoded_text_ast_value.is_null());
    REQUIRE_FALSE(eight_byte_encoded_text_ast_value.is<value_int_t>());
    REQUIRE_FALSE(eight_byte_encoded_text_ast_value.is<value_float_t>());
    REQUIRE_FALSE(eight_byte_encoded_text_ast_value.is<value_bool_t>());
    REQUIRE_FALSE(eight_byte_encoded_text_ast_value.is<string>());
    REQUIRE_FALSE(eight_byte_encoded_text_ast_value.is<FourByteEncodedTextAst>());
    REQUIRE_THROWS(eight_byte_encoded_text_ast_value.get_immutable_view<value_int_t>());
    REQUIRE_THROWS(eight_byte_encoded_text_ast_value.get_immutable_view<value_float_t>());
    REQUIRE_THROWS(eight_byte_encoded_text_ast_value.get_immutable_view<value_bool_t>());
    REQUIRE_THROWS(eight_byte_encoded_text_ast_value.get_immutable_view<string>());
    REQUIRE_THROWS(eight_byte_encoded_text_ast_value.get_immutable_view<FourByteEncodedTextAst>());

    Value const four_byte_encoded_text_ast_value{
            get_encoded_text_ast<four_byte_encoded_variable_t>(cStringToEncode)
    };
    REQUIRE(four_byte_encoded_text_ast_value.is<FourByteEncodedTextAst>());
    REQUIRE_FALSE(four_byte_encoded_text_ast_value.is_null());
    REQUIRE_FALSE(four_byte_encoded_text_ast_value.is<value_int_t>());
    REQUIRE_FALSE(four_byte_encoded_text_ast_value.is<value_float_t>());
    REQUIRE_FALSE(four_byte_encoded_text_ast_value.is<value_bool_t>());
    REQUIRE_FALSE(four_byte_encoded_text_ast_value.is<string>());
    REQUIRE_FALSE(four_byte_encoded_text_ast_value.is<EightByteEncodedTextAst>());
    REQUIRE_THROWS(four_byte_encoded_text_ast_value.get_immutable_view<value_int_t>());
    REQUIRE_THROWS(four_byte_encoded_text_ast_value.get_immutable_view<value_float_t>());
    REQUIRE_THROWS(four_byte_encoded_text_ast_value.get_immutable_view<value_bool_t>());
    REQUIRE_THROWS(four_byte_encoded_text_ast_value.get_immutable_view<string>());
    REQUIRE_THROWS(four_byte_encoded_text_ast_value.get_immutable_view<EightByteEncodedTextAst>());
}
