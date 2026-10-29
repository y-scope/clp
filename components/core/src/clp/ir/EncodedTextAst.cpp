#include "EncodedTextAst.hpp"

#include <cstddef>
#include <optional>
#include <string>

#include "../ffi/encoding_methods.hpp"
#include "../ffi/ir_stream/decoding_methods.hpp"

using clp::ffi::decode_float_var;
using clp::ffi::decode_integer_var;
using clp::ffi::ir_stream::DecodingException;
using clp::ffi::ir_stream::generic_decode_message;
using std::optional;
using std::string;

namespace clp::ir {
template <typename encoded_variable_t>
auto EncodedTextAst<encoded_variable_t>::decode_and_unparse() const -> optional<string> {
    string decoded_string;

    auto constant_handler = [&](string const& value, size_t begin_pos, size_t length) {
        decoded_string.append(value, begin_pos, length);
    };

    auto encoded_int_handler
            = [&](encoded_variable_t value) { decoded_string.append(decode_integer_var(value)); };

    auto encoded_float_handler = [&](encoded_variable_t encoded_float) {
        decoded_string.append(decode_float_var(encoded_float));
    };

    auto dict_var_handler = [&](string const& dict_var) { decoded_string.append(dict_var); };

    try {
        generic_decode_message<true>(
                m_logtype,
                m_encoded_vars,
                m_dict_vars,
                constant_handler,
                encoded_int_handler,
                encoded_float_handler,
                dict_var_handler
        );
    } catch (DecodingException const& e) {
        return std::nullopt;
    }
    return std::make_optional<string>(decoded_string);
}

// Explicitly declare template specializations so that we can define the template methods in this
// file
template auto EncodedTextAst<eight_byte_encoded_variable_t>::decode_and_unparse() const
        -> optional<string>;
template auto EncodedTextAst<four_byte_encoded_variable_t>::decode_and_unparse() const
        -> optional<string>;
}  // namespace clp::ir
