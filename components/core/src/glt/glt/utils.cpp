#include "utils.hpp"

#include <iostream>

#include <boost/filesystem.hpp>

#include "../ErrorCode.hpp"
#include "../spdlog_with_specializations.hpp"
#include "../Utils.hpp"

using std::string;
using std::vector;

namespace glt::glt {
bool find_all_files_and_empty_directories(
        boost::filesystem::path& path_prefix_to_remove,
        string const& path,
        vector<FileToCompress>& file_paths,
        vector<string>& empty_directory_paths
) {
    string path_without_prefix;
    if (false == remove_prefix_and_clean_up_path(path_prefix_to_remove, path, path_without_prefix))
    {
        SPDLOG_ERROR(
                "'{}' does not contain prefix '{}'.",
                path.c_str(),
                path_prefix_to_remove.c_str()
        );
        return false;
    }

    try {
        if (false == boost::filesystem::is_directory(path)) {
            // path is a file
            file_paths.emplace_back(path, path_without_prefix, 0);
            return true;
        }

        if (boost::filesystem::is_empty(path)) {
            // path is an empty directory
            empty_directory_paths.push_back(path_without_prefix);
            return true;
        }

        // Iterate directory
        boost::filesystem::recursive_directory_iterator iter(
                path,
                boost::filesystem::directory_options::follow_directory_symlink
        );
        boost::filesystem::recursive_directory_iterator end;
        for (; iter != end; ++iter) {
            // Check if current entry is an empty directory or a file
            if (boost::filesystem::is_directory(iter->path())) {
                if (boost::filesystem::is_empty(iter->path())) {
                    remove_prefix_and_clean_up_path(
                            path_prefix_to_remove,
                            iter->path(),
                            path_without_prefix
                    );
                    empty_directory_paths.push_back(path_without_prefix);
                    iter.disable_recursion_pending();
                }
            } else {
                remove_prefix_and_clean_up_path(
                        path_prefix_to_remove,
                        iter->path(),
                        path_without_prefix
                );
                file_paths.emplace_back(iter->path().string(), path_without_prefix, 0);
            }
        }
    } catch (boost::filesystem::filesystem_error& exception) {
        SPDLOG_ERROR(
                "Failed to find files/directories at '{}' - {}.",
                path.c_str(),
                exception.what()
        );
        return false;
    }

    return true;
}

bool is_utf8_sequence(size_t sequence_length, char const* sequence) {
    size_t num_utf8_bytes_to_read = 0;
    for (size_t i = 0; i < sequence_length; ++i) {
        auto byte = sequence[i];

        if (num_utf8_bytes_to_read > 0) {
            // Validate that byte matches 0b10xx_xxxx
            if ((byte & 0xC0) != 0x80) {
                return false;
            }
            --num_utf8_bytes_to_read;
        } else {
            if (byte & 0x80) {
                // Check if byte is valid UTF-8 length-indicator
                if ((byte & 0xF8) == 0xF0) {
                    // Matches 0b1111_0xxx
                    num_utf8_bytes_to_read = 3;
                } else if ((byte & 0xF0) == 0xE0) {
                    // Matches 0b1110_xxxx
                    num_utf8_bytes_to_read = 2;
                } else if ((byte & 0xE0) == 0xC0) {
                    // Matches 0b110x_xxxx
                    num_utf8_bytes_to_read = 1;
                } else {
                    // Invalid UTF-8 length-indicator
                    return false;
                }
            }  // else byte is ASCII
        }
    }

    return true;
}

bool read_input_paths(string const& list_path, vector<string>& paths) {
    ErrorCode error_code = read_list_of_paths(list_path, paths);
    if (ErrorCode_Success != error_code) {
        if (ErrorCode_FileNotFound == error_code) {
            SPDLOG_ERROR("'{}' does not exist.", list_path.c_str());
        } else if (ErrorCode_errno == error_code) {
            SPDLOG_ERROR("Failed to read '{}', errno={}", list_path.c_str(), errno);
        } else {
            SPDLOG_ERROR("Failed to read '{}', error_code={}", list_path.c_str(), error_code);
        }
        return false;
    }

    // Validate the file contained at least one input path
    if (paths.empty()) {
        SPDLOG_ERROR("'{}' did not contain any paths", list_path.c_str());
        return false;
    }

    return true;
}

bool remove_prefix_and_clean_up_path(
        boost::filesystem::path const& prefix_to_remove,
        boost::filesystem::path const& path,
        string& path_without_prefix_string
) {
    auto prefix_to_remove_ix = prefix_to_remove.begin();
    auto prefix_to_remove_end_ix = prefix_to_remove.end();
    // Remove trailing '.' if necessary
    if (*prefix_to_remove.rbegin() == ".") {
        --prefix_to_remove_end_ix;
    }

    auto path_ix = path.begin();
    auto path_end_ix = path.end();
    // Remove trailing '.' if necessary
    if (*path.rbegin() == ".") {
        --path_end_ix;
    }

    // Compare prefix with path
    while (prefix_to_remove_end_ix != prefix_to_remove_ix) {
        if (path_end_ix == path_ix) {
            return false;
        }
        if (*prefix_to_remove_ix != *path_ix) {
            return false;
        }
        ++prefix_to_remove_ix;
        ++path_ix;
    }

    // Construct path without prefix
    // NOTE: We initialize the path to '/' so that it remains an absolute path even if a prefix was
    // removed
    bool found_valid_path_element = false;
    boost::filesystem::path path_without_prefix("/");
    for (; path_end_ix != path_ix; ++path_ix) {
        if (false == found_valid_path_element) {
            if (".." == *path_ix || "." == *path_ix || "/" == *path_ix) {
                continue;
            }
            found_valid_path_element = true;
        }
        path_without_prefix.append(path_ix->string());
    }
    path_without_prefix_string = path_without_prefix.lexically_normal().string();

    // Path can't be empty
    return false == path_without_prefix_string.empty();
}

bool validate_paths_exist(vector<string> const& paths) {
    // Ensure all paths in the list exist
    bool all_paths_exist = true;
    for (auto const& path : paths) {
        if (boost::filesystem::exists(path) == false) {
            SPDLOG_ERROR("'{}' does not exist.", path.c_str());
            all_paths_exist = false;
        }
    }

    return all_paths_exist;
}
}  // namespace glt::glt
