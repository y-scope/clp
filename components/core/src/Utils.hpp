#ifndef UTILS_HPP
#define UTILS_HPP

// C++ libraries
#include <list>
#include <set>
#include <sstream>
#include <unordered_set>
#include <vector>

// Project headers
#include "Defs.h"
#include "ErrorCode.hpp"
#include "FileReader.hpp"
#include "ParsedMessage.hpp"
#include "frontend/Lexer.hpp"

/**
 * Checks if the given character is an alphabet
 * @param c
 * @return true if c is an alphabet, false otherwise
 */
inline bool is_alphabet (char c) {
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z');
}

/**
 * Checks if character is a decimal (base-10) digit
 * @param c
 * @return true if c is a decimal digit, false otherwise
 */
inline bool is_decimal_digit (char c)  {
    return '0' <= c && c <= '9';
}

/**
 * Checks if the given segment of the stringcould be a multi-digit hex value
 * @param str
 * @param begin_pos
 * @param end_pos
 * @return true if yes, false otherwise
 */
inline bool could_be_multi_digit_hex_value (const std::string& str, size_t begin_pos, size_t end_pos) {
    if (end_pos - begin_pos < 2) {
        return false;
    }

    for (size_t i = begin_pos; i < end_pos; ++i) {
        auto c = str[i];
        if (!( ('a' <= c && c <= 'f') || ('A' <= c && c <= 'F') || ('0' <= c && c <= '9') )) {
            return false;
        }
    }

    return true;
}

/**
 * Checks if character is a delimiter
 * We treat everything except the following quoted characters as a delimiter: "+-./0-9A-Z\a-z"
 * NOTE: For performance, we rely on the ASCII ordering of characters to compare ranges of characters at a time instead of comparing individual characters
 * @param c
 * @return true if c is a delimiter, false otherwise
 */
inline bool is_delim (char c) {
    return !('+' == c || ('-' <= c && c <= '9') || ('A' <= c && c <= 'Z') || '\\' == c || '_' == c || ('a' <= c && c <= 'z'));
}

/**
 * Checks if character is a wildcard
 * @param c
 * @return true if c is a wildcard, false otherwise
 */
bool is_wildcard (char c);

/**
 * Cleans wildcard search string
 * - Removes consecutive '*'
 * - Removes escaping from non-wildcard characters
 * - Adds wildcard to beginning and end of string
 * @param str Wildcard search string to clean
 * @return Cleaned wildcard search string
 */
std::string clean_up_wildcard_search_string (const std::string& str);

/**
 * Converts the given string to a 64-bit integer if possible
 * @param raw
 * @param converted
 * @return true if the conversion was successful, false otherwise
 */
bool convert_string_to_int64 (const std::string& raw, int64_t& converted);

/**
 * Converts the given string to a double if possible
 * @param raw
 * @param converted
 * @return true if the conversion was successful, false otherwise
 */
bool convert_string_to_double (const std::string& raw, double& converted);

/**
 * Creates a directory with the given path
 * @param path
 * @param mode
 * @param exist_ok
 * @return ErrorCode_Success on success
 * @return ErrorCode_errno on error
 * @return ErrorCode_FileExists if exist_ok was false and the path already existed
 */
ErrorCode create_directory (const std::string& path, __mode_t mode, bool exist_ok);

/**
 * Creates every directory in the given path (if they don't exist)
 * NOTE: We assume the path "/" exists
 * @param path The path (must be non-empty)
 * @param mode Permission bits for structure
 * @return ErrorCode_Success on success, ErrorCode_errno otherwise
 */
ErrorCode create_directory_structure (const std::string& path, __mode_t mode);

/**
 * Searches haystack starting at the given position for one of the given needles
 * @param haystack
 * @param needles
 * @param search_start_pos
 * @param needle_ix The index of the needle found
 * @return The position of the match or string::npos if none
 */
size_t find_first_of (const std::string& haystack, const char* needles, size_t search_start_pos, size_t& needle_ix);

/**
 * Gets the parent directory path for a given path
 * Corner cases:
 * - get_dirname("abc") = "."
 * - get_dirname(".") = "."
 * - get_dirname("..") = "."
 * - get_dirname("/") = "/"
 * - get_dirname("/.") = "/"
 * - get_dirname("/..") = "/"
 * - get_dirname("/abc") = "/"
 * @param path
 * @return Parent directory path
 */
std::string get_parent_directory_path (const std::string& path);

/**
 * Returns bounds of next variable in given string
 * A variable is a token (word between two delimiters) that contains numbers or is directly preceded by an equals sign
 * @param msg
 * @param begin_pos Begin position of last variable, changes to begin position of next variable
 * @param end_pos End position of last variable, changes to end position of next variable
 * @return true if a variable was found, false otherwise
 */
bool get_bounds_of_next_var (const std::string& msg, size_t& begin_pos, size_t& end_pos);

/**
 * Removes ".", "..", and consecutive "/" from a given path and returns the result
 * @param path The given path
 * @return The unambiguous path
 */
std::string get_unambiguous_path (const std::string& path);

/**
 * Replaces the given characters in the given value with the given replacements
 * @param characters_to_escape
 * @param replacement_characters
 * @param value
 * @param escape Whether to precede the replacement with a '\' (e.g., so that a line-feed character is output as "\n")
 * @return The string with replacements
 */
std::string replace_characters (const char* characters_to_escape, const char* replacement_characters, const std::string& value, bool escape);

/**
 * Perform wildcard match
 * @param string_tame A cpp string without wildcards
 * @param string_wild A (potentially) corresponding cpp string with wildcards
 * @return
 */
bool wildCardMatch_caseSensitive (
        const std::string& string_tame,
        const std::string& string_wild
);

/**
 * Perform wildcard match with case sentivity flag
 * @param string_tame A string without wildcards
 * @param string_wild A (potentially) corresponding string with wildcards
 * @param isCaseSensitive
 * @return
 */
bool wildCardMatch (
        const std::string& string_tame,
        const std::string& string_wild,
        const bool isCaseSensitive
);

/**
 * Maps a given file into memory
 * @param path
 * @param read_ahead Whether to read-ahead in the file
 * @param fd Reference to file descriptor opened for file
 * @param file_size Reference to file size determined for file
 * @param ptr Reference to pointer to mapped region
 * @return ErrorCode_errno on error
 * @return ErrorCode_FileNotFound if file not found
 * @return ErrorCode_Success on success
 */
ErrorCode memory_map_file (const std::string& path, bool read_ahead, int& fd, size_t& file_size, void*& ptr);

/**
 * Unmaps a memory-mapped file
 * @param fd File's file descriptor
 * @param file_size File's size
 * @param ptr Pointer to mapped region
 * @return ErrorCode_errno on error
 * @return ErrorCode_Success on success
 */
ErrorCode memory_unmap_file (int fd, size_t file_size, void* ptr);

/**
 * Read a list of paths from a file
 * @param list_path
 * @param paths
 * @return ErrorCode_Success on success
 * @return Otherwise, same as FileReader::try_open and FileReader::try_read_to_delimiter
 */
ErrorCode read_list_of_paths (const std::string& list_path, std::vector<std::string>& paths);

/**
 * Gets the underlying type of the given enum
 * @tparam T
 * @param enum_member
 * @return The underlying type of the given enum
 */
template <typename T>
constexpr typename std::underlying_type<T>::type enum_to_underlying_type(T enum_member) {
    return static_cast<typename std::underlying_type<T>::type>(enum_member);
}

#endif // UTILS_HPP
