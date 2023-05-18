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
 * Creates a directory with the given path
 * @param path
 * @param mode
 * @param exist_ok
 * @return ErrorCode_Success on success
 * @return ErrorCode_errno on error
 * @return ErrorCode_FileExists if exist_ok was false and the path already existed
 */
ErrorCode create_directory (const std::string& path, mode_t mode, bool exist_ok);

/**
 * Creates every directory in the given path (if they don't exist)
 * NOTE: We assume the path "/" exists
 * @param path The path (must be non-empty)
 * @param mode Permission bits for structure
 * @return ErrorCode_Success on success, ErrorCode_errno otherwise
 */
ErrorCode create_directory_structure (const std::string& path, mode_t mode);

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
 * Read a list of paths from a file
 * @param list_path
 * @param paths
 * @return ErrorCode_Success on success
 * @return Otherwise, same as FileReader::try_open and FileReader::try_read_to_delimiter
 */
ErrorCode read_list_of_paths (const std::string& list_path, std::vector<std::string>& paths);

#endif // UTILS_HPP
