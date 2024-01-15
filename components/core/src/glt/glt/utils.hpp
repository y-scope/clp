#ifndef GLT_GLT_UTILS_HPP
#define GLT_GLT_UTILS_HPP

#include <string>

#include <boost/filesystem/path.hpp>

#include "FileToCompress.hpp"

namespace glt::glt {
/**
 * Recursively finds all files and empty directories at the given path
 * @param path_prefix_to_remove
 * @param path
 * @param file_paths
 * @param empty_directory_paths
 * @return true on success, false otherwise
 */
bool find_all_files_and_empty_directories(
        boost::filesystem::path& path_prefix_to_remove,
        std::string const& path,
        std::vector<FileToCompress>& file_paths,
        std::vector<std::string>& empty_directory_paths
);

/**
 * Checks if the given sequence is valid UTF-8
 * @param sequence_length
 * @param sequence
 * @return true if valid, false otherwise
 */
bool is_utf8_sequence(size_t sequence_length, char const* sequence);

/**
 * Reads a list of input paths
 * @param list_path
 * @param paths
 * @return true on success, false otherwise
 */
bool read_input_paths(std::string const& list_path, std::vector<std::string>& paths);

/**
 * Removes the given prefix from the given path and cleans the path as follows:
 * - Removes redundant '.' and ".."
 * - Makes the path absolute
 * @param prefix_to_remove
 * @param path
 * @param path_without_prefix_string
 * @return false if the path didn't contain the prefix or it didn't contain anything besides the
 * prefix, true otherwise
 */
bool remove_prefix_and_clean_up_path(
        boost::filesystem::path const& prefix_to_remove,
        boost::filesystem::path const& path,
        std::string& path_without_prefix_string
);

/**
 * Validates that all paths in the given list exist
 * @param paths
 * @return true if they all exist, false otherwise
 */
bool validate_paths_exist(std::vector<std::string> const& paths);
}  // namespace glt::glt

#endif  // GLT_GLT_UTILS_HPP
