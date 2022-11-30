#ifndef FFI_ENCODING_METHODS_TPP
#define FFI_ENCODING_METHODS_TPP

// Project headers
#include "../string_utils.hpp"
#include "../type_utils.hpp"

namespace ffi {
    template <VariablePlaceholder var_placeholder>
    bool wildcard_query_matches_any_encoded_var (
            std::string_view wildcard_query,
            std::string_view logtype,
            encoded_variable_t* encoded_vars,
            int encoded_vars_length
    ) {
        size_t encoded_vars_ix = 0;
        for (auto c : logtype) {
            if (enum_to_underlying_type(VariablePlaceholder::Float) == c) {
                if (encoded_vars_ix >= encoded_vars_length) {
                    throw EncodingException(ErrorCode_Corrupt, __FILENAME__, __LINE__,
                                            cTooFewEncodedVarsErrorMessage);
                }

                if constexpr (VariablePlaceholder::Float == var_placeholder) {
                    auto decoded_var = decode_float_var(encoded_vars[encoded_vars_ix]);
                    if (wildcard_match_unsafe(decoded_var, wildcard_query)) {
                        return true;
                    }
                }

                ++encoded_vars_ix;
            } else if (enum_to_underlying_type(VariablePlaceholder::Integer) == c) {
                if (encoded_vars_ix >= encoded_vars_length) {
                    throw EncodingException(ErrorCode_Corrupt, __FILENAME__, __LINE__,
                                            cTooFewEncodedVarsErrorMessage);
                }

                if constexpr (VariablePlaceholder::Integer == var_placeholder) {
                    auto decoded_var = decode_integer_var(encoded_vars[encoded_vars_ix]);
                    if (wildcard_match_unsafe(decoded_var, wildcard_query)) {
                        return true;
                    }
                }

                ++encoded_vars_ix;
            }
        }

        return false;
    }
}

#endif // FFI_ENCODING_METHODS_TPP
