#include "ExactVariableToken.hpp"

#include "../../ir/types.hpp"

using clp::ir::VariablePlaceholder;
using std::string_view;

namespace clp::ffi::search {
template <typename encoded_variable_t>
ExactVariableToken<encoded_variable_t>::ExactVariableToken(
        string_view query,
        size_t begin_pos,
        size_t end_pos
)
        : QueryToken(query, begin_pos, end_pos) {
    auto token = query.substr(begin_pos, end_pos - begin_pos);
    if (encode_float_string(token, m_encoded_value)) {
        m_type = TokenType::FloatVariable;
        m_placeholder = VariablePlaceholder::Float;
    } else if (encode_integer_string(token, m_encoded_value)) {
        m_type = TokenType::IntegerVariable;
        m_placeholder = VariablePlaceholder::Integer;
    } else {
        m_type = TokenType::DictionaryVariable;
        m_placeholder = VariablePlaceholder::Dictionary;
        m_encoded_value = 0;
    }
}

// Explicitly declare specializations to avoid having to validate that the template parameters are
// supported
template class ExactVariableToken<ir::eight_byte_encoded_variable_t>;
template class ExactVariableToken<ir::four_byte_encoded_variable_t>;
}  // namespace clp::ffi::search
