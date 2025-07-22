#include "parsing.hpp"

#include <cstddef>
#include <string>
#include <string_view>

#include <string_utils/string_utils.hpp>

#include "../type_utils.hpp"
#include "types.hpp"

using std::string;
using std::string_view;

namespace clp::ir {
/*
 * For performance, we rely on the ASCII ordering of characters to compare ranges of characters at a
 * time instead of comparing individual characters
 */
bool is_delim(signed char c) {
    return false
           == ('+' == c || ('-' <= c && c <= '.') || ('0' <= c && c <= '9')
               || ('A' <= c && c <= 'Z') || '\\' == c || '_' == c || ('a' <= c && c <= 'z'));
}

bool is_variable_placeholder(char c) {
    return (enum_to_underlying_type(VariablePlaceholder::Integer) == c)
           || (enum_to_underlying_type(VariablePlaceholder::Dictionary) == c)
           || (enum_to_underlying_type(VariablePlaceholder::Float) == c);
}

bool is_var(std::string_view value) {
    size_t begin_pos = 0;
    size_t end_pos = 0;
    if (get_bounds_of_next_var(value, begin_pos, end_pos)) {
        // Ensure the entire value is a variable
        return (0 == begin_pos && value.length() == end_pos);
    } else {
        return false;
    }
}

bool get_bounds_of_next_var(string_view const str, size_t& begin_pos, size_t& end_pos) {
    auto const msg_length = str.length();
    if (msg_length <= end_pos) {
        return false;
    }

    while (true) {
        begin_pos = end_pos;

        // Find next non-delimiter
        for (; begin_pos < msg_length; ++begin_pos) {
            auto c = str[begin_pos];
            if (false == is_delim(c)) {
                break;
            }
        }
        if (msg_length == begin_pos) {
            // Early exit for performance
            return false;
        }

        bool contains_decimal_digit = false;
        bool contains_alphabet = false;

        // Find next delimiter
        end_pos = begin_pos;
        for (; end_pos < msg_length; ++end_pos) {
            auto c = str[end_pos];
            if (string_utils::is_decimal_digit(c)) {
                contains_decimal_digit = true;
            } else if (string_utils::is_alphabet(c)) {
                contains_alphabet = true;
            } else if (is_delim(c)) {
                break;
            }
        }

        auto variable = str.substr(begin_pos, end_pos - begin_pos);
        // Treat token as variable if:
        // - it contains a decimal digit, or
        // - it's directly preceded by '=' and contains an alphabet char, or
        // - it could be a multi-digit hex value
        if (contains_decimal_digit
            || (0 < begin_pos && '=' == str[begin_pos - 1] && contains_alphabet)
            || could_be_multi_digit_hex_value(variable))
        {
            break;
        }
    }

    return (msg_length != begin_pos);
}

void escape_and_append_const_to_logtype(string_view constant, string& logtype) {
    // clang-format off
    auto escape_handler = [&](
            [[maybe_unused]] string_view constant,
            [[maybe_unused]] size_t char_to_escape_pos,
            string& logtype
    ) -> void {
        logtype += enum_to_underlying_type(VariablePlaceholder::Escape);
    };
    // clang-format on
    append_constant_to_logtype(constant, escape_handler, logtype);
}
}  // namespace clp::ir
