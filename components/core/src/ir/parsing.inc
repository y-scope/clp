#ifndef IR_PARSING_INC
#define IR_PARSING_INC

#include <string>
#include <string_view>

namespace ir {
template <bool double_escape, typename EscapeHandler>
void append_constant_to_logtype(
        std::string_view constant,
        std::string& logtype,
        EscapeHandler escape_handler
) {
    size_t begin_pos = 0;
    auto constant_len = constant.length();
    for (size_t i = 0; i < constant_len; ++i) {
        auto const c = constant[i];
        bool const is_escape_char = (cVariablePlaceholderEscapeCharacter == c);
        if (is_escape_char || is_variable_placeholder(c)) {
            logtype.append(constant, begin_pos, i - begin_pos);
            escape_handler(logtype);
            if constexpr (double_escape) {
                if (false == is_escape_char) {
                    escape_handler(logtype);
                }
            }
            // NOTE: We don't need to append the character of interest
            // immediately since the next constant copy operation will get it
            begin_pos = i;
        }
    }
    logtype.append(constant, begin_pos, constant_len - begin_pos);
}

template <bool double_escape>
void escape_and_append_constant_to_logtype(
        std::string_view constant,
        std::string& logtype
) {
    auto escape_handler = [&](std::string& logtype) {
        logtype += ir::cVariablePlaceholderEscapeCharacter;
    };
    append_constant_to_logtype<double_escape>(constant, logtype, escape_handler);
}
}
#endif
