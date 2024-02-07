#ifndef GLT_IR_PARSING_INC
#define GLT_IR_PARSING_INC

#include <string>
#include <string_view>

#include "../type_utils.hpp"
#include "types.hpp"

namespace glt::ir {
template <typename EscapeHandler>
void append_constant_to_logtype(
        std::string_view constant,
        EscapeHandler escape_handler,
        std::string& logtype
) {
    size_t begin_pos = 0;
    auto constant_len = constant.length();
    for (size_t i = 0; i < constant_len; ++i) {
        auto const c = constant[i];
        bool const is_escape_char = (enum_to_underlying_type(VariablePlaceholder::Escape) == c);
        if (false == is_escape_char && false == is_variable_placeholder(c)) {
            continue;
        }
        logtype.append(constant, begin_pos, i - begin_pos);
        // NOTE: We don't need to append the character of interest immediately since the next
        // constant copy operation will get it
        begin_pos = i;
        escape_handler(constant, i, logtype);
    }
    logtype.append(constant, begin_pos, constant_len - begin_pos);
}
}  // namespace glt::ir
#endif  // GLT_IR_PARSING_INC
