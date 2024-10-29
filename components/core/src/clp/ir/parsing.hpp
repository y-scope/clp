#ifndef CLP_IR_PARSING_HPP
#define CLP_IR_PARSING_HPP

/**
 * TODO Technically, the methods in this file are more general than for their use in generating
 * CLP's IR. However, introducing a parsing namespace in the root source directory would be
 * confusing since we also have the compressor_frontend namespace. Once most of
 * compressor_frontend is moved into https://github.com/y-scope/log-surgeon, we should reconsider
 * the placement of the methods in this file.
 */

#include <cstddef>
#include <string>
#include <string_view>

namespace clp::ir {
/**
 * Checks if the given character is a delimiter
 * We treat everything *except* the following quoted characters as a delimiter: "+-.0-9A-Z\_a-z"
 * @param c
 * @return Whether c is a delimiter
 */
bool is_delim(signed char c);

/**
 * @param c
 * @return Whether the character is a variable placeholder
 */
bool is_variable_placeholder(char c);

/**
 * NOTE: This method is marked inline for a 1-2% performance improvement
 * @param str
 * @return Whether the given string could be a multi-digit hex value
 */
inline bool could_be_multi_digit_hex_value(std::string_view str) {
    if (str.length() < 2) {
        return false;
    }

    // NOTE: This is 1-2% faster than using std::all_of with the opposite condition
    for (auto c : str) {
        if (false == (('a' <= c && c <= 'f') || ('A' <= c && c <= 'F') || ('0' <= c && c <= '9'))) {
            return false;
        }
    }

    return true;
}

/**
 * @param value
 * @return Whether the given value is a variable according to the schemas specified in
 * ffi::get_bounds_of_next_var
 */
bool is_var(std::string_view value);

/**
 * Gets the bounds of the next variable in the given string
 * A variable is a token (word between two delimiters) that matches one of these schemas:
 * - ".*[0-9].*"
 * - "=(.*[a-zA-Z].*)" (the variable is within the capturing group)
 * - "[a-fA-F0-9]{2,}"
 * @param str String to search within
 * @param begin_pos Begin position of last variable, changes to begin position of next variable
 * @param end_pos End position of last variable, changes to end position of next variable
 * @return true if a variable was found, false otherwise
 */
bool get_bounds_of_next_var(std::string_view str, size_t& begin_pos, size_t& end_pos);

/**
 * Appends a constant to the logtype, escaping any variable placeholders.
 * @param constant
 * @param logtype
 */
void escape_and_append_const_to_logtype(std::string_view constant, std::string& logtype);

/**
 * Appends the given constant to the logtype, optionally escaping any variable placeholders found
 * within the constant using the given handler.
 * @tparam EscapeHandler Method to optionally escape any variable placeholders found within the
 * constant. Signature: (
 *         [[maybe_unused]] std::string_view constant,
 *         [[maybe_unused]] size_t char_to_escape_pos,
 *         std::string& logtype
 * ) -> void
 * @param constant
 * @param escape_handler
 * @param logtype
 */
template <typename EscapeHandler>
void append_constant_to_logtype(
        std::string_view constant,
        EscapeHandler escape_handler,
        std::string& logtype
);
}  // namespace clp::ir

#include "parsing.inc"
#endif  // CLP_IR_PARSING_HPP
