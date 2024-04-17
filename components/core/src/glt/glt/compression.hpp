#ifndef GLT_GLT_COMPRESSION_HPP
#define GLT_GLT_COMPRESSION_HPP

#include <string>
#include <vector>

#include <boost/filesystem/path.hpp>

#include "CommandLineArguments.hpp"
#include "FileToCompress.hpp"

namespace glt::glt {
/**
 * Compresses all given paths into an archive
 * @param command_line_args
 * @param files_to_compress
 * @param empty_directory_paths
 * @param grouped_files_to_compress
 * @param target_encoded_file_size
 * @param reader_parser
 * @param use_heuristic
 * @return true if compression was successful, false otherwise
 */
bool compress(
        CommandLineArguments& command_line_args,
        std::vector<FileToCompress>& files_to_compress,
        std::vector<std::string> const& empty_directory_paths,
        std::vector<FileToCompress>& grouped_files_to_compress,
        size_t target_encoded_file_size
);

/**
 * Reads a list of grouped files and a list of their IDs
 * @param path_prefix_to_remove
 * @param list_path Path of the list of grouped files
 * @param grouped_files
 * @return true on success, false otherwise
 */
bool read_and_validate_grouped_file_list(
        boost::filesystem::path const& path_prefix_to_remove,
        std::string const& list_path,
        std::vector<FileToCompress>& grouped_files
);
}  // namespace glt::glt

#endif  // GLT_GLT_COMPRESSION_HPP
