#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "../../BufferReader.hpp"
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
    // Load dict vars
    for (size_t i{0}; i < dict_var_bounds.size(); i += 2) {
        auto const begin{static_cast<size_t>(dict_var_bounds[i])};
        auto const length{static_cast<size_t>(dict_var_bounds[i + 1]) - begin};
        auto const dict_var{input.substr(begin, length)};
        BufferReader reader{dict_var.data(), dict_var.size()};
        REQUIRE_FALSE(string_blob.read_from(reader, length).has_value());
    }

    // Load logtype
    BufferReader reader{logtype.data(), logtype.size()};
    REQUIRE_FALSE(string_blob.read_from(reader, logtype.size()).has_value());

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
        std::string text;
        std::vector<std::string> const var_strs{
                "4938",
                std::to_string(INT32_MAX),
                std::to_string(INT64_MAX),
                "0.1",
                "-25.519686",
                "-25.5196868642755",
                "-00.00",
                "bin/python2.7.3",
                "abc123"
        };
        size_t var_ix{0};
        text = "here is a string with a small int " + var_strs[var_ix++];
        text += " and a medium int " + var_strs[var_ix++];
        text += " and a very large int " + var_strs[var_ix++];
        text += " and a small float " + var_strs[var_ix++];
        text += " and a medium float " + var_strs[var_ix++];
        text += " and a weird float " + var_strs[var_ix++];
        text += " and a string with numbers " + var_strs[var_ix++];
        text += " and another string with numbers " + var_strs[var_ix++];
        text += "\n";
        text += "Integer var placeholder: ";
        text += enum_to_underlying_type(VariablePlaceholder::Integer);
        text += "\n";
        text += "Float var placeholder: ";
        text += enum_to_underlying_type(VariablePlaceholder::Float);
        text += "\n";
        text += "Dict var placeholder: ";
        text += enum_to_underlying_type(VariablePlaceholder::Dictionary);

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
                     == EncodedTextAstErr{EncodedTextAstErrEnum::MissingLogtype})
            );
        }

        SECTION("Missing variables") {
            auto const [placeholder, expected_error_enum] = GENERATE(
                    std::make_pair(
                            enum_to_underlying_type(VariablePlaceholder::Integer),
                            EncodedTextAstErrEnum::MissingEncodedVar
                    ),
                    std::make_pair(
                            enum_to_underlying_type(VariablePlaceholder::Float),
                            EncodedTextAstErrEnum::MissingEncodedVar
                    ),
                    std::make_pair(
                            enum_to_underlying_type(VariablePlaceholder::Dictionary),
                            EncodedTextAstErrEnum::MissingDictVar
                    )
            );
            std::string const logtype_with_single_int_var{placeholder};
            StringBlob string_blob;
            BufferReader reader{
                    logtype_with_single_int_var.data(),
                    logtype_with_single_int_var.size()
            };
            REQUIRE_FALSE(
                    string_blob.read_from(reader, logtype_with_single_int_var.size()).has_value()
            );
            auto const encoded_text_ast_result{EncodedTextAst<TestType>::create(
                    std::vector<TestType>{},
                    std::move(string_blob)
            )};
            REQUIRE_FALSE(encoded_text_ast_result.has_error());
            auto const decoded_result{encoded_text_ast_result.value().to_string()};
            REQUIRE(decoded_result.has_error());
            REQUIRE((decoded_result.error() == EncodedTextAstErr{expected_error_enum}));
        }

        SECTION("Trailing escape") {
            std::string const logtype_with_trailing_escape{
                    "This is a string with a trailing escape "
                    + std::string(1, enum_to_underlying_type(VariablePlaceholder::Escape))
            };
            StringBlob string_blob;
            BufferReader reader{
                    logtype_with_trailing_escape.data(),
                    logtype_with_trailing_escape.size()
            };
            REQUIRE_FALSE(
                    string_blob.read_from(reader, logtype_with_trailing_escape.size()).has_value()
            );
            auto const encoded_text_ast_result{EncodedTextAst<TestType>::create(
                    std::vector<TestType>{},
                    std::move(string_blob)
            )};
            REQUIRE_FALSE(encoded_text_ast_result.has_error());
            auto const decoded_result{encoded_text_ast_result.value().to_string()};
            REQUIRE(decoded_result.has_error());
            REQUIRE(
                    (decoded_result.error()
                     == EncodedTextAstErr{EncodedTextAstErrEnum::UnexpectedTrailingEscapeCharacter})
            );
        }
    }
}
}  // namespace clp::ffi::test
