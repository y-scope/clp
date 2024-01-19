#ifndef GLT_UTILS_HPP
#define GLT_UTILS_HPP

#include <list>
#include <set>
#include <sstream>
#include <unordered_set>
#include <vector>

#include "Defs.h"
#include "ErrorCode.hpp"
#include "FileReader.hpp"
#include "ParsedMessage.hpp"

namespace glt {
/**
 * Creates a directory with the given path
 * @param path
 * @param mode
 * @param exist_ok
 * @return ErrorCode_Success on success
 * @return ErrorCode_errno on error
 * @return ErrorCode_FileExists if exist_ok was false and the path already existed
 */
ErrorCode create_directory(std::string const& path, mode_t mode, bool exist_ok);

/**
 * Creates every directory in the given path (if they don't exist)
 * NOTE: We assume the path "/" exists
 * @param path The path (must be non-empty)
 * @param mode Permission bits for structure
 * @return ErrorCode_Success on success, ErrorCode_errno otherwise
 */
ErrorCode create_directory_structure(std::string const& path, mode_t mode);

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
std::string get_parent_directory_path(std::string const& path);

/**
 * Removes ".", "..", and consecutive "/" from a given path and returns the result
 * @param path The given path
 * @return The unambiguous path
 */
std::string get_unambiguous_path(std::string const& path);

/**
 * Read a list of paths from a file
 * @param list_path
 * @param paths
 * @return ErrorCode_Success on success
 * @return Otherwise, same as FileReader::try_open and FileReader::try_read_to_delimiter
 */
ErrorCode read_list_of_paths(std::string const& list_path, std::vector<std::string>& paths);

}  // namespace glt

#endif  // GLT_UTILS_HPP
