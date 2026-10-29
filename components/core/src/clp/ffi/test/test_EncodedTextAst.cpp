#include <cstddef>
#include <cstdint>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <fmt/format.h>
#include <fmt/ranges.h>

#include "../../ir/types.hpp"
#include "../../type_utils.hpp"
#include "../EncodedTextAst.hpp"
#include "../EncodedTextAstError.hpp"
#include "../encoding_methods.hpp"
#include "../StringBlob.hpp"

namespace clp::ffi::test {
namespace {
using ir::eight_byte_encoded_variable_t;
using ir::EncodedVariableTypeReq;
using ir::four_byte_encoded_variable_t;
using ir::VariablePlaceholder;

/**
 * @tparam encoded_variable_t
 * @param input
 * @return An encoded text AST constructed by encoding the given input string.
 */
template <EncodedVariableTypeReq encoded_variable_t>
[[nodiscard]] auto create_encoded_text_ast_from_string(std::string_view input)
        -> EncodedTextAst<encoded_variable_t>;

template <EncodedVariableTypeReq encoded_variable_t>
auto create_encoded_text_ast_from_string(std::string_view input)
        -> EncodedTextAst<encoded_variable_t> {
    std::string logtype;
    std::vector<encoded_variable_t> encoded_vars;
    std::vector<int32_t> dict_var_bounds;
    REQUIRE(encode_message(input, logtype, encoded_vars, dict_var_bounds));

    StringBlob string_blob;
    for (size_t i{0}; i < dict_var_bounds.size(); i += 2) {
        auto const begin{static_cast<size_t>(dict_var_bounds[i])};
        auto const length{static_cast<size_t>(dict_var_bounds[i + 1]) - begin};
        auto const dict_var{input.substr(begin, length)};
        string_blob.append(dict_var);
    }

    string_blob.append(logtype);

    auto encoded_text_ast_result{EncodedTextAst<encoded_variable_t>::create(
            std::move(encoded_vars),
            std::move(string_blob)
    )};
    REQUIRE_FALSE(encoded_text_ast_result.has_error());
    return std::move(encoded_text_ast_result.value());
}
}  // namespace

TEMPLATE_TEST_CASE(
        "EncodedTextAst Decoding",
        "[ffi][EncodedTextAst]",
        eight_byte_encoded_variable_t,
        four_byte_encoded_variable_t
) {
    SECTION("Text with variables") {
        std::vector<std::pair<std::string, std::string>> const test_str_components{
                {"Here is a string with a small int ", "2887"},
                {"and a medium int ", std::to_string(INT32_MAX)},
                {"and a very large int ", std::to_string(INT64_MAX)},
                {"and a small float ", "0.1"},
                {"and a medium float ", "-25.519686"},
                {"and a long float ", "-25.5196868642755"},
                {"and a weird float ", "-00.00"},
                {"and a string with numbers ", "bin/python3.14.0"},
                {"and another string with numbers ", "abc123"},
                {"and a dict var=", "IamString"},
                {"and another dict var=", "DictVarWith\\escape"},
                {"and an int var placeholder: ",
                 std::string(1, enum_to_underlying_type(VariablePlaceholder::Integer))},
                {"and a float var placeholder: ",
                 std::string(1, enum_to_underlying_type(VariablePlaceholder::Float))},
                {"and a dict var placeholder: ",
                 std::string(1, enum_to_underlying_type(VariablePlaceholder::Dictionary))},
                {"and a valid trailing escape: ",
                 std::string(2, enum_to_underlying_type(VariablePlaceholder::Escape))},
        };
        auto const text = fmt::format(
                "{}",
                fmt::join(
                        test_str_components | std::views::transform([](auto const& pair) {
                            return pair.first + pair.second;
                        }),
                        " "
                )
        );

        auto const encoded_text_ast{create_encoded_text_ast_from_string<TestType>(text)};
        auto const decoded_text_result{encoded_text_ast.to_string()};
        REQUIRE_FALSE(decoded_text_result.has_error());
        REQUIRE((decoded_text_result.value() == text));
    }

    SECTION("Text without variables") {
        constexpr std::string_view cText{"This is a static message."};
        auto const encoded_text_ast{create_encoded_text_ast_from_string<TestType>(cText)};
        REQUIRE((encoded_text_ast.get_logtype() == cText));
        auto const decoded_text_result{encoded_text_ast.to_string()};
        REQUIRE_FALSE((decoded_text_result.has_error()));
        REQUIRE((decoded_text_result.value() == cText));
    }

    SECTION("Decoding errors") {
        SECTION("Missing logtype") {
            auto const encoded_text_ast_result{
                    EncodedTextAst<TestType>::create(std::vector<TestType>{}, StringBlob{})
            };
            REQUIRE(encoded_text_ast_result.has_error());
            REQUIRE(
                    (encoded_text_ast_result.error()
                     == EncodedTextAstError{EncodedTextAstErrorEnum::MissingLogtype})
            );
        }

        SECTION("Missing variables") {
            auto const [placeholder, expected_error_enum] = GENERATE(
                    std::make_pair(
                            enum_to_underlying_type(VariablePlaceholder::Integer),
                            EncodedTextAstErrorEnum::MissingEncodedVar
                    ),
                    std::make_pair(
                            enum_to_underlying_type(VariablePlaceholder::Float),
                            EncodedTextAstErrorEnum::MissingEncodedVar
                    ),
                    std::make_pair(
                            enum_to_underlying_type(VariablePlaceholder::Dictionary),
                            EncodedTextAstErrorEnum::MissingDictVar
                    )
            );
            std::string const logtype_with_single_int_var{placeholder};
            StringBlob string_blob;
            string_blob.append(logtype_with_single_int_var);
            auto const encoded_text_ast_result{EncodedTextAst<TestType>::create(
                    std::vector<TestType>{},
                    std::move(string_blob)
            )};
            REQUIRE_FALSE(encoded_text_ast_result.has_error());
            auto const decoded_result{encoded_text_ast_result.value().to_string()};
            REQUIRE(decoded_result.has_error());
            REQUIRE((decoded_result.error() == EncodedTextAstError{expected_error_enum}));
        }

        SECTION("Trailing escape") {
            std::string const logtype_with_trailing_escape{
                    "This is a string with a trailing escape "
                    + std::string(1, enum_to_underlying_type(VariablePlaceholder::Escape))
            };
            StringBlob string_blob;
            string_blob.append(logtype_with_trailing_escape);
            auto const encoded_text_ast_result{EncodedTextAst<TestType>::create(
                    std::vector<TestType>{},
                    std::move(string_blob)
            )};
            REQUIRE_FALSE(encoded_text_ast_result.has_error());
            auto const decoded_result{encoded_text_ast_result.value().to_string()};
            REQUIRE(decoded_result.has_error());
            REQUIRE(
                    (decoded_result.error()
                     == EncodedTextAstError{
                             EncodedTextAstErrorEnum::UnexpectedTrailingEscapeCharacter
                     })
            );
        }
    }
}
}  // namespace clp::ffi::test
