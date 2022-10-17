#ifndef CLP_COMPRESSION_HPP
#define CLP_COMPRESSION_HPP

// C++ standard libraries
#include <string>
#include <vector>

// Boost libraries
#include <boost/filesystem/path.hpp>

// Project headers
#include "CommandLineArguments.hpp"
#include "FileToCompress.hpp"
#include "StructuredFileToCompress.hpp"
#include "../compressor_frontend/LogParser.hpp"

namespace clp {
    /**
     * Compresses all given paths into an archive
     * @param command_line_args
     * @param files_to_compress
     * @param empty_directory_paths
     * @param grouped_files_to_compress
     * @param target_encoded_file_size
     * @param log_parser
     * @param use_heuristic
     * @return true if compression was successful, false otherwise
     */
    bool compress (CommandLineArguments& command_line_args, std::vector<FileToCompress>& files_to_compress,
                   const std::vector<std::string>& empty_directory_paths, std::vector<FileToCompress>& grouped_files_to_compress,
                   size_t target_encoded_file_size, std::unique_ptr<compressor_frontend::LogParser> log_parser, bool use_heuristic);

    /**
     * Reads a list of grouped files and a list of their IDs
     * @param path_prefix_to_remove
     * @param list_path Path of the list of grouped files
     * @param grouped_files
     * @return true on success, false otherwise
     */
    bool read_and_validate_grouped_file_list (const boost::filesystem::path& path_prefix_to_remove, const std::string& list_path,
                                              std::vector<FileToCompress>& grouped_files);
}

#endif // CLP_COMPRESSION_HPP
