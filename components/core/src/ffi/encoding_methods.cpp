#include "encoding_methods.hpp"

// C++ standard libraries
#include <algorithm>
#include <string_view>

using std::string_view;

namespace ffi {
    /*
     * For performance, we rely on the ASCII ordering of characters to compare
     * ranges of characters at a time instead of comparing individual
     * characters
     */
    bool is_delim (signed char c) {
        return !('+' == c || ('-' <= c && c <= '.') || ('0' <= c && c <= '9') ||
                 ('A' <= c && c <= 'Z') || '\\' == c || '_' == c || ('a' <= c && c <= 'z'));
    }

    bool is_variable_placeholder (char c) {
        return (enum_to_underlying_type(VariablePlaceholder::Integer) == c) ||
               (enum_to_underlying_type(VariablePlaceholder::Dictionary) == c) ||
               (enum_to_underlying_type(VariablePlaceholder::Float) == c);
    }

    bool could_be_multi_digit_hex_value (const string_view str) {
        if (str.length() < 2) {
            return false;
        }

        return std::all_of(str.cbegin(), str.cend(), [] (char c) {
            return ('a' <= c && c <= 'f') || ('A' <= c && c <= 'F') || ('0' <= c && c <= '9');
        });
    }

    bool is_var (std::string_view value) {
        size_t begin_pos = 0;
        size_t end_pos = 0;
        bool contains_var_placeholder;
        if (get_bounds_of_next_var(value, begin_pos, end_pos, contains_var_placeholder)) {
            // Ensure the entire value is a variable
            return (0 == begin_pos && value.length() == end_pos);
        } else {
            return false;
        }
    }

    bool get_bounds_of_next_var (const string_view str, size_t& begin_pos, size_t& end_pos,
                                 bool& contains_var_placeholder)
    {
        const auto msg_length = str.length();
        if (end_pos >= msg_length) {
            return false;
        }

        while (true) {
            begin_pos = end_pos;

            // Find next non-delimiter
            bool char_before_var_was_equals_sign = false;
            char previous_char = 0;
            for (; begin_pos < msg_length; ++begin_pos) {
                auto c = str[begin_pos];
                if (false == is_delim(c)) {
                    char_before_var_was_equals_sign = ('=' == previous_char);
                    break;
                }
                previous_char = c;

                if (enum_to_underlying_type(VariablePlaceholder::Integer) == c ||
                    enum_to_underlying_type(VariablePlaceholder::Dictionary) == c ||
                    enum_to_underlying_type(VariablePlaceholder::Float) == c)
                {
                    contains_var_placeholder = true;
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

            string_view variable(str.cbegin() + begin_pos, end_pos - begin_pos);
            // Treat token as variable if:
            // - it contains a decimal digit, or
            // - it's directly preceded by an equals sign and contains an alphabet, or
            // - it could be a multi-digit hex value
            if (contains_decimal_digit ||
                (char_before_var_was_equals_sign && contains_alphabet) ||
                could_be_multi_digit_hex_value(variable))
            {
                break;
            }
        }

        return (msg_length != begin_pos);
    }
}
