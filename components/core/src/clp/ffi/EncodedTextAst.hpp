#ifndef CLP_FFI_ENCODEDTEXTAST_HPP
#define CLP_FFI_ENCODEDTEXTAST_HPP

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <ystdlib/error_handling/Result.hpp>

#include "../ir/types.hpp"
#include "../type_utils.hpp"
#include "EncodedTextAstError.hpp"
#include "encoding_methods.hpp"
#include "StringBlob.hpp"
#include "type_utils.hpp"

namespace clp::ffi {
/**
 * Method signature requirements for handling constant text segments in an encoded text AST.
 * @tparam EncodedTextAstConstantHandlerType
 */
template <typename EncodedTextAstConstantHandlerType>
concept EncodedTextAstConstantHandlerReq
        = requires(EncodedTextAstConstantHandlerType handler, std::string_view constant) {
              { handler(constant) } -> std::same_as<void>;
          };

/**
 * Method signature requirements for handling int variables in an encoded text AST.
 * @tparam EncodedTextAstIntVarHandlerType
 * @tparam encoded_variable_t
 */
template <typename EncodedTextAstIntVarHandlerType, typename encoded_variable_t>
concept EncodedTextAstIntVarHandlerReq
        = requires(EncodedTextAstIntVarHandlerType handler, encoded_variable_t var) {
              { handler(var) } -> std::same_as<void>;
          };

/**
 * Method signature requirements for handling float variables in an encoded text AST.
 * @tparam EncodedTextAstFloatVarHandlerType
 * @tparam encoded_variable_t
 */
template <typename EncodedTextAstFloatVarHandlerType, typename encoded_variable_t>
concept EncodedTextAstFloatVarHandlerReq
        = requires(EncodedTextAstFloatVarHandlerType handler, encoded_variable_t var) {
              { handler(var) } -> std::same_as<void>;
          };

/**
 * Method signature requirements for handling dictionary variables in an encoded text AST.
 * @tparam EncodedTextAstDictVarHandlerType
 */
template <typename EncodedTextAstDictVarHandlerType>
concept EncodedTextAstDictVarHandlerReq
        = requires(EncodedTextAstDictVarHandlerType handler, std::string_view var) {
              { handler(var) } -> std::same_as<void>;
          };

/**
 * A parsed and encoded unstructured text string.
 * @tparam encoded_variable_t The type of encoded variables in the string.
 */
template <ir::EncodedVariableTypeReq encoded_variable_t>
class EncodedTextAst {
public:
    // Factory function
    /**
     * @param encoded_vars
     * @param string_blob A string blob containing a list of dictionary variables followed by a
     * logtype.
     * @return A result containing the newly created `EncodedTextAst` instance on success, or an
     * error code indicating the failure:
     * - EncodedTextAstErrorEnum::MissingLogtype: if `string_blob` contains no strings.
     */
    [[nodiscard]] static auto
    create(std::vector<encoded_variable_t> encoded_vars, StringBlob string_blob)
            -> ystdlib::error_handling::Result<EncodedTextAst> {
        if (string_blob.get_num_strings() < 1) {
            return EncodedTextAstError{EncodedTextAstErrorEnum::MissingLogtype};
        }
        return EncodedTextAst{std::move(encoded_vars), std::move(string_blob)};
    }

    /**
     * Parses and encodes the given text into an `EncodedTextAst` instance.
     * @param text
     * @return The encoded text AST.
     */
    [[nodiscard]] static auto parse_and_encode_from(std::string_view text) -> EncodedTextAst {
        std::string logtype;
        std::vector<encoded_variable_t> encoded_vars;
        std::vector<int32_t> dict_var_bounds;
        ffi::encode_message(text, logtype, encoded_vars, dict_var_bounds);

        StringBlob string_blob;
        for (size_t i{0}; i < dict_var_bounds.size(); i += 2) {
            auto const begin_pos{static_cast<size_t>(dict_var_bounds[i])};
            auto const length{static_cast<size_t>(dict_var_bounds[i + 1]) - begin_pos};
            string_blob.append(text.substr(begin_pos, length));
        }

        string_blob.append(logtype);

        return EncodedTextAst{std::move(encoded_vars), std::move(string_blob)};
    }

    // Default copy & move constructors and assignment operators
    EncodedTextAst(EncodedTextAst const&) = default;
    EncodedTextAst(EncodedTextAst&&) noexcept = default;
    auto operator=(EncodedTextAst const&) -> EncodedTextAst& = default;
    auto operator=(EncodedTextAst&&) noexcept -> EncodedTextAst& = default;

    // Destructor
    ~EncodedTextAst() = default;

    auto operator==(EncodedTextAst const& other) const -> bool = default;

    // Methods
    [[nodiscard]] auto get_logtype() const -> std::string_view {
        return m_string_blob.get_string(m_num_dict_vars).value();
    }

    /**
     * Decodes the encoded text AST into its string form by calling the given handlers for each
     * component of the message.
     * @tparam unescape_logtype Whether to remove the escape characters from the logtype before
     * calling `constant_handler`.
     * @param constant_handler
     * @param int_var_handler
     * @param float_var_handler
     * @param dict_var_handler
     * @return A void result on success, or an error code indicating the failure:
     * - EncodedTextAstErrorEnum::MissingEncodedVar if an encoded variable is missing.
     * - EncodedTextAstErrorEnum::MissingDictVar if a dictionary variable is missing.
     * - EncodedTextAstErrorEnum::UnexpectedTrailingEscapeCharacter if the logtype ends with an
     *   unexpected escape character.
     */
    template <bool unescape_logtype>
    [[nodiscard]] auto decode(
            EncodedTextAstConstantHandlerReq auto constant_handler,
            EncodedTextAstIntVarHandlerReq<encoded_variable_t> auto int_var_handler,
            EncodedTextAstFloatVarHandlerReq<encoded_variable_t> auto float_var_handler,
            EncodedTextAstDictVarHandlerReq auto dict_var_handler
    ) const -> ystdlib::error_handling::Result<void>;

    /**
     * Decodes and un-parses the encoded text AST into its string form.
     * @return A result containing the decoded string on success, or an error code indicating the
     * failure:
     * - Forwards `decode`'s return values on failure.
     */
    [[nodiscard]] auto to_string() const -> ystdlib::error_handling::Result<std::string> {
        std::string decoded_string;
        YSTDLIB_ERROR_HANDLING_TRYV(
                decode<true>(
                        [&](std::string_view constant) { decoded_string.append(constant); },
                        [&](encoded_variable_t int_var) {
                            decoded_string.append(decode_integer_var(int_var));
                        },
                        [&](encoded_variable_t float_var) {
                            decoded_string.append(decode_float_var(float_var));
                        },
                        [&](std::string_view dict_var) { decoded_string.append(dict_var); }
                )
        );
        return decoded_string;
    }

private:
    // Constructor
    EncodedTextAst(std::vector<encoded_variable_t> encoded_vars, StringBlob string_blob)
            : m_encoded_vars{std::move(encoded_vars)},
              m_string_blob{std::move(string_blob)},
              m_num_dict_vars{m_string_blob.get_num_strings() - 1} {}

    // Variables
    std::vector<encoded_variable_t> m_encoded_vars;
    StringBlob m_string_blob;
    size_t m_num_dict_vars;
};

using EightByteEncodedTextAst = EncodedTextAst<ir::eight_byte_encoded_variable_t>;
using FourByteEncodedTextAst = EncodedTextAst<ir::four_byte_encoded_variable_t>;

template <ir::EncodedVariableTypeReq encoded_variable_t>
template <bool unescape_logtype>
[[nodiscard]] auto EncodedTextAst<encoded_variable_t>::decode(
        EncodedTextAstConstantHandlerReq auto constant_handler,
        EncodedTextAstIntVarHandlerReq<encoded_variable_t> auto int_var_handler,
        EncodedTextAstFloatVarHandlerReq<encoded_variable_t> auto float_var_handler,
        EncodedTextAstDictVarHandlerReq auto dict_var_handler
) const -> ystdlib::error_handling::Result<void> {
    auto const logtype{get_logtype()};
    auto const logtype_length = logtype.length();
    auto const num_encoded_vars{m_encoded_vars.size()};

    size_t next_static_text_begin_pos{0};
    size_t dictionary_vars_idx{0};
    size_t encoded_vars_idx{0};

    for (size_t curr_pos{0}; curr_pos < logtype_length; ++curr_pos) {
        auto const c{logtype.at(curr_pos)};
        switch (c) {
            case enum_to_underlying_type(ir::VariablePlaceholder::Float): {
                constant_handler(logtype.substr(
                        next_static_text_begin_pos,
                        curr_pos - next_static_text_begin_pos
                ));
                next_static_text_begin_pos = curr_pos + 1;
                if (encoded_vars_idx >= num_encoded_vars) {
                    return EncodedTextAstError{EncodedTextAstErrorEnum::MissingEncodedVar};
                }
                float_var_handler(m_encoded_vars.at(encoded_vars_idx));
                ++encoded_vars_idx;
                break;
            }

            case enum_to_underlying_type(ir::VariablePlaceholder::Integer): {
                constant_handler(logtype.substr(
                        next_static_text_begin_pos,
                        curr_pos - next_static_text_begin_pos
                ));
                next_static_text_begin_pos = curr_pos + 1;
                if (encoded_vars_idx >= num_encoded_vars) {
                    return EncodedTextAstError{EncodedTextAstErrorEnum::MissingEncodedVar};
                }
                int_var_handler(m_encoded_vars.at(encoded_vars_idx));
                ++encoded_vars_idx;
                break;
            }

            case enum_to_underlying_type(ir::VariablePlaceholder::Dictionary): {
                constant_handler(logtype.substr(
                        next_static_text_begin_pos,
                        curr_pos - next_static_text_begin_pos
                ));
                next_static_text_begin_pos = curr_pos + 1;
                if (dictionary_vars_idx >= m_num_dict_vars) {
                    return EncodedTextAstError{EncodedTextAstErrorEnum::MissingDictVar};
                }
                dict_var_handler(m_string_blob.get_string(dictionary_vars_idx).value());
                ++dictionary_vars_idx;
                break;
            }

            case enum_to_underlying_type(ir::VariablePlaceholder::Escape): {
                // Ensure the escape character is followed by a character that's being escaped
                if (curr_pos == logtype_length - 1) {
                    return EncodedTextAstError{
                            EncodedTextAstErrorEnum::UnexpectedTrailingEscapeCharacter
                    };
                }

                if constexpr (unescape_logtype) {
                    constant_handler(logtype.substr(
                            next_static_text_begin_pos,
                            curr_pos - next_static_text_begin_pos
                    ));
                    // Skip the escape character
                    next_static_text_begin_pos = curr_pos + 1;
                }

                // The character after the escape character is static text (regardless of whether it
                // is a variable placeholder), so increment curr_pos by 1 to ensure we don't process
                // the next character in any of the other cases (instead it will be added to the
                // message).
                ++curr_pos;
                break;
            }

            default:
                // Regular characters. Do nothing.
                continue;
        }
    }

    // Add remainder
    if (next_static_text_begin_pos < logtype_length) {
        constant_handler(logtype.substr(
                next_static_text_begin_pos,
                logtype_length - next_static_text_begin_pos
        ));
    }

    return ystdlib::error_handling::success();
}
}  // namespace clp::ffi

#endif  // CLP_FFI_ENCODEDTEXTAST_HPP
