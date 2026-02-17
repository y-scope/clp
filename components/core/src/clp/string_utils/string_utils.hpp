#ifndef CLP_STRING_UTILS_STRINGUTILS_HPP
#define CLP_STRING_UTILS_STRINGUTILS_HPP

#include <charconv>
#include <concepts>
#include <cstddef>
#include <string>
#include <string_view>
#include <system_error>

namespace clp::string_utils {

/**
 * Checks if the given character is an alphabet
 * 
 * @param c
 * @return true if c is an alphabet, false otherwise
 */
[[nodiscard]] inline auto is_alphabet(char c) -> bool {
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z');
}

/**
 * Checks if character is a decimal (base-10) digit
 *
 * @param c
 * @return true if c is a decimal digit, false otherwise
 */
[[nodiscard]] inline auto is_decimal_digit(char c) -> bool {
    return '0' <= c && c <= '9';
}

/**
 * Searches haystack starting at the given position for one of the given needles
 *
 * @param haystack
 * @param needles
 * @param search_start_pos
 * @param needle_ix The index of the needle found
 * @return The position of the match or string::npos if none
 */
[[nodiscard]] auto find_first_of(
        std::string const& haystack,
        char const* needles,
        size_t search_start_pos,
        size_t& needle_ix
) -> size_t;

/**
 * Replaces the given characters in the given value with the given replacements
 * 
 * @param characters_to_escape
 * @param replacement_characters
 * @param value
 * @param escape Whether to precede the replacement with a '\' (e.g., so that a
 * line-feed character is output as "\n")
 * @return The string with replacements
 */
[[nodiscard]] auto replace_characters(
        char const* characters_to_escape,
        char const* replacement_characters,
        std::string const& value,
        bool escape
) -> std::string;

/**
 * Replaces unescaped instances of `from_char` with `to_char` in `str`.
 *
 * NOTE: `from_char` and `escape_char` must not be the same character. If they are, the function's
 * behaviour is undefined.
 *
 * @param escape_char The character used for escaping
 * @param from_char
 * @param to_char
 * @param str String in which to replace the characters
 */
auto replace_unescaped_char(char escape_char, char from_char, char to_char, std::string& str)
        -> void;

/**
 * Converts a string to lowercase
 * 
 * @param str
 */
auto to_lower(std::string& str) -> void;

/**
 * Cleans wildcard search string
 * <ul>
 *   <li>Removes consecutive '*'</li>
 *   <li>Removes escaping from non-wildcard characters</li>
 *   <li>Removes dangling escape character from the end of the string</li>
 * </ul>
 * @param str Wildcard search string to clean
 * @return Cleaned wildcard search string
 */
[[nodiscard]] auto clean_up_wildcard_search_string(std::string_view str) -> std::string;

/**
 * Unescapes a string according to the following rules:
 * <ul>
 *   <li>Escape sequences `\<char>` are replaced by `<char>`</li>
 *   <li>Lone dangling `\` is removed from the end of the string</li>
 * </ul>
 * @param str
 * @return An unescaped version of `str`.
 */
[[nodiscard]] auto unescape_string(std::string_view str) -> std::string;

/**
 * Checks if character is a wildcard
 * 
 * @param c
 * @return true if c is a wildcard, false otherwise
 */
[[nodiscard]] auto is_wildcard(char c) -> bool;

/**
 * Same as ``wildcard_match_unsafe_case_sensitive`` except this method allows
 * the caller to specify whether the match should be case sensitive.
 *
 * @param tame The literal string
 * @param wild The wildcard string
 * @param case_sensitive_match Whether to consider case when matching
 * @return Whether the two strings match
 */
[[nodiscard]] auto wildcard_match_unsafe(
        std::string_view tame,
        std::string_view wild,
        bool case_sensitive_match = true
) -> bool;

/**
 * Checks if a string matches a wildcard string. Two wildcards are currently
 * supported: '*' to match 0 or more characters, and '?' to match any single
 * character. Each can be escaped using a preceding '\'. Other characters which
 * are escaped are treated as normal characters.
 * <br/>
 * This method is optimized for performance by omitting some checks on the
 * wildcard string that are unnecessary if the caller cleans up the wildcard
 * string as follows:
 * <ul>
 *   <li>The wildcard string should not contain consecutive '*'.</li>
 *   <li>The wildcard string should not contain an escape character without a
 *   character following it.</li>
 * </ul>
 *
 * @param tame The literal string
 * @param wild The wildcard string
 * @return Whether the two strings match
 */
[[nodiscard]] auto wildcard_match_unsafe_case_sensitive(std::string_view tame, std::string_view wild) -> bool;

/**
 * Converts the given string to a 64-bit integer if possible
 * 
 * @tparam integer_t
 * @param raw
 * @param converted
 * @return true if the conversion was successful, false otherwise
 */
template <std::integral integer_t>
[[nodiscard]] auto convert_string_to_int(std::string_view raw, integer_t& converted) -> bool;

template <std::integral integer_t>
[[nodiscard]] auto convert_string_to_int(std::string_view raw, integer_t& converted) -> bool {
    auto const* raw_begin{raw.data()};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    auto const* raw_end{raw_begin + raw.size()};
    auto const result{std::from_chars(raw_begin, raw_end, converted)};

    return result.ptr == raw_end && std::errc{} == result.ec;
}
}  // namespace clp::string_utils

#endif  // CLP_STRING_UTILS_STRINGUTILS_HPP
