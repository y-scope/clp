#ifndef CLP_UTILS_HPP
#define CLP_UTILS_HPP

#include <list>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include <log_surgeon/log_surgeon.hpp>
#include <ystdlib/error_handling/Result.hpp>

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
 * Builds a log-surgeon parser from a parsing specification file.
 * @param spec_path Path to the parsing specification file.
 * @return A result containing the built parser or an error code indicating the failure:
 * - clpp::ClppErrorCodeEnum::BadParam if reading the spec fails or it is empty.
 */
auto build_parser_from_file(std::string_view spec_path)
        -> ystdlib::error_handling::Result<log_surgeon::Parser>;
}  // namespace clp

#endif  // CLP_UTILS_HPP
