#ifndef CLP_S_UTILS_HPP
#define CLP_S_UTILS_HPP

#include <charconv>
#include <cstring>
#include <string>

#include <boost/filesystem.hpp>

namespace clp_s {
class FileUtils {
public:
    /**
     * Find all files in a directory
     * @param path
     * @param file_paths
     * @return true if successful, false otherwise
     */
    static bool find_all_files(std::string const& path, std::vector<std::string>& file_paths);

    /**
     * Validate if all paths exist
     * @param paths
     * @return true if all paths exist, false otherwise
     */
    static bool validate_path(std::vector<std::string> const& paths);
};

class StringUtils {
public:
    /**
     * Checks if the given character is an alphabet
     * @param c
     * @return true if c is an alphabet, false otherwise
     */
    static inline bool is_alphabet(char c) {
        return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z');
    }

    /**
     * Checks if character is a decimal (base-10) digit
     * @param c
     * @return true if c is a decimal digit, false otherwise
     */
    static inline bool is_decimal_digit(char c) { return '0' <= c && c <= '9'; }

    /**
     * Checks if character is a hexadecimal (base-16) digit
     * @param c
     * @return true if c is a hexadecimal digit, false otherwise
     */
    static inline bool is_delim(char c) {
        return !(
                '+' == c || ('-' <= c && c <= '9') || ('A' <= c && c <= 'Z') || '\\' == c
                || '_' == c || ('a' <= c && c <= 'z')
        );
    }

    /**
     * Checks if the string could be a hexadecimal value
     * @param str
     * @param begin_pos
     * @param end_pos
     * @return true if str could be a hexadecimal value, false otherwise
     */
    static inline bool
    could_be_multi_digit_hex_value(std::string const& str, size_t begin_pos, size_t end_pos) {
        if (end_pos - begin_pos < 2) {
            return false;
        }

        for (size_t i = begin_pos; i < end_pos; ++i) {
            auto c = str[i];
            if (false
                == (('a' <= c && c <= 'f') || ('A' <= c && c <= 'F') || ('0' <= c && c <= '9')))
            {
                return false;
            }
        }

        return true;
    }

    /**
     * Returns bounds of next variable in given string
     * A variable is a token (word between two delimiters) that contains numbers or is directly
     * preceded by an equals sign
     * @param msg
     * @param begin_pos Begin position of last variable, changes to begin position of next variable
     * @param end_pos End position of last variable, changes to end position of next variable
     * @return true if a variable was found, false otherwise
     */
    static bool get_bounds_of_next_var(std::string const& msg, size_t& begin_pos, size_t& end_pos);

    /**
     * Searches haystack starting at the given position for one of the given needles
     * @param haystack
     * @param needles
     * @param search_start_pos
     * @param needle_ix The index of the needle found
     * @return The position of the match or string::npos if none
     */
    static size_t find_first_of(
            std::string const& haystack,
            char const* needles,
            size_t search_start_pos,
            size_t& needle_ix
    );

    /**
     * Replaces the given characters in the given value with the given replacements
     * @param characters_to_escape
     * @param replacement_characters
     * @param value
     * @param escape Whether to precede the replacement with a '\' (e.g., so that a
     * line-feed character is output as "\n")
     * @return The string with replacements
     */
    static std::string replace_characters(
            char const* characters_to_escape,
            char const* replacement_characters,
            std::string const& value,
            bool escape
    );

    /**
     * Converts a string to lowercase
     * @param str
     */
    static void to_lower(std::string& str);

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
    static std::string clean_up_wildcard_search_string(std::string_view str);

    /**
     * Checks if character is a wildcard
     * @param c
     * @return true if c is a wildcard, false otherwise
     */
    static bool is_wildcard(char c);

    /**
     * Checks if the given string has unescaped wildcards
     * @param str
     * @return true if the string has unescaped wildcards, false otherwise
     */
    static bool has_unescaped_wildcards(std::string const& str);

    /**
     * Same as ``wildcard_match_unsafe_case_sensitive`` except this method
     * allows the caller to specify whether the match should be case sensitive.
     *
     * @param tame The literal string
     * @param wild The wildcard string
     * @param case_sensitive_match Whether to consider case when matching
     * @return Whether the two strings match
     */
    static bool wildcard_match_unsafe(
            std::string_view tame,
            std::string_view wild,
            bool case_sensitive_match = true
    );

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
    static bool wildcard_match_unsafe_case_sensitive(std::string_view tame, std::string_view wild);

    /**
     * Converts the given string to a 64-bit integer if possible
     * @param raw
     * @param converted
     * @return true if the conversion was successful, false otherwise
     */
    static bool convert_string_to_int64(std::string_view raw, int64_t& converted);

    /**
     * Converts the given string to a double if possible
     * @param raw
     * @param converted
     * @return true if the conversion was successful, false otherwise
     */
    static bool convert_string_to_double(std::string const& raw, double& converted);

    /**
     * Converts a string column descriptor delimited by '.' into a list of tokens
     * @param descriptor
     * @param tokens
     * @return the list of tokens pushed into the 'tokens' parameter
     */
    static void
    tokenize_column_descriptor(std::string const& descriptor, std::vector<std::string>& tokens);

private:
    /**
     * Helper for ``wildcard_match_unsafe_case_sensitive`` to advance the
     * pointer in tame to the next character which matches wild. This method
     * should be inlined for performance.
     * @param tame_current
     * @param tame_bookmark
     * @param tame_end
     * @param wild_current
     * @param wild_bookmark
     * @return true on success, false if wild cannot match tame
     */
    static inline bool advance_tame_to_next_match(
            char const*& tame_current,
            char const*& tame_bookmark,
            char const* tame_end,
            char const*& wild_current,
            char const*& wild_bookmark
    );
};

enum EvaluatedValue {
    True,
    False,
    Unknown
};

template <class T2, class T1>
inline T2 bit_cast(T1 t1) {
    static_assert(sizeof(T1) == sizeof(T2), "Must match size");
    static_assert(std::is_standard_layout<T1>::value, "Need to be standard layout");
    static_assert(std::is_standard_layout<T2>::value, "Need to be standard layout");

    T2 t2;
    std::memcpy(std::addressof(t2), std::addressof(t1), sizeof(T1));
    return t2;
}

/**
 * A span of memory where the underlying memory may not be aligned correctly for type T.
 *
 * This class should be used whenever we need a view into some memory, and we do not know whether it
 * is aligned correctly for type T. If the alignment of the underlying memory is known std::span
 * should be used instead.
 *
 * In C++ creating a pointer to objects of type T that is not correctly aligned for type T is
 * undefined behaviour, as is dereferencing such a pointer. This class avoids this undefined
 * behaviour by using memcpy (which any modern compiler should be able to optimize away).
 *
 * For any modern x86 platform the performance difference between using std::span and
 * UnalignedMemSpan should be fairly minimal.
 *
 * @tparam T
 */
template <typename T>
class UnalignedMemSpan {
public:
    UnalignedMemSpan() = default;
    UnalignedMemSpan(char* begin, size_t size) : m_begin(begin), m_size(size) {};

    size_t size() { return m_size; }

    T operator[](size_t i) {
        T tmp;
        memcpy(&tmp, m_begin + i * sizeof(T), sizeof(T));
        return tmp;
    }

    UnalignedMemSpan<T> sub_span(size_t start, size_t size) {
        return {m_begin + start * sizeof(T), size};
    }

private:
    char* m_begin{nullptr};
    size_t m_size{0};
};
}  // namespace clp_s
#endif  // CLP_S_UTILS_HPP
