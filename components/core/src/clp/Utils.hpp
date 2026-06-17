#ifndef CLP_UTILS_HPP
#define CLP_UTILS_HPP

#include <list>
#include <set>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

#include <log_surgeon/log_surgeon.hpp>

#include "Defs.h"
#include "ErrorCode.hpp"
#include "FileReader.hpp"
#include "ParsedMessage.hpp"

namespace clp {
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
 * Read a list of paths from a file
 * @param list_path
 * @param paths
 * @return ErrorCode_Success on success
 * @return Otherwise, same as FileReader::try_open and FileReader::try_read_to_delimiter
 */
ErrorCode read_list_of_paths(std::string const& list_path, std::vector<std::string>& paths);

/**
 * Loads a parser from a parsing specification string.
 * @param parsing_spec
 * @return parser
 */
auto load_parser_from_str(std::string const& parsing_spec) -> log_surgeon::ParserHandle;

/**
 * Loads a parser from a parsing specification file.
 * @param parsing_spec_path
 * @return parser
 */
auto load_parser_from_file(std::string const& parsing_spec_path) -> log_surgeon::ParserHandle;
}  // namespace clp

#endif  // CLP_UTILS_HPP
