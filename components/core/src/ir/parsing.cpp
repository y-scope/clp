#include "parsing.hpp"

#include "../string_utils.hpp"
#include "../type_utils.hpp"

using std::string_view;

namespace ir {
/*
 * For performance, we rely on the ASCII ordering of characters to compare
 * ranges of characters at a time instead of comparing individual characters
 */
bool is_delim(signed char c) {
    return !(
            '+' == c || ('-' <= c && c <= '.') || ('0' <= c && c <= '9') || ('A' <= c && c <= 'Z')
            || '\\' == c || '_' == c || ('a' <= c && c <= 'z')
    );
}

bool is_variable_placeholder(char c) {
    return (enum_to_underlying_type(VariablePlaceholder::Integer) == c)
           || (enum_to_underlying_type(VariablePlaceholder::Dictionary) == c)
           || (enum_to_underlying_type(VariablePlaceholder::Float) == c);
}

// NOTE: `inline` improves performance by 1-2%
inline bool could_be_multi_digit_hex_value(string_view str) {
    if (str.length() < 2) {
        return false;
    }

    // NOTE: This is 1-2% faster than using std::all_of with the opposite
    // condition
    for (auto c : str) {
        if (!(('a' <= c && c <= 'f') || ('A' <= c && c <= 'F') || ('0' <= c && c <= '9'))) {
            return false;
        }
    }

    return true;
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

bool get_bounds_of_next_var(
        string_view const str,
        size_t& begin_pos,
        size_t& end_pos
) {
    auto const msg_length = str.length();
    if (end_pos >= msg_length) {
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
            if (is_decimal_digit(c)) {
                contains_decimal_digit = true;
            } else if (is_alphabet(c)) {
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
            || (begin_pos > 0 && '=' == str[begin_pos - 1] && contains_alphabet)
            || could_be_multi_digit_hex_value(variable))
        {
            break;
        }
    }

    return (msg_length != begin_pos);
}
}  // namespace ir
