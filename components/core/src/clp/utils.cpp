#include "utils.hpp"

// C++ standard libraries
#include <iostream>

// Boost libraries
#include <boost/filesystem/operations.hpp>
#include <boost/uuid/random_generator.hpp>

// Project headers
#include "../ErrorCode.hpp"
#include "../spdlog_with_specializations.hpp"
#include "../Utils.hpp"

using std::string;
using std::vector;

namespace clp {
    bool find_all_files_and_empty_directories (boost::filesystem::path& path_prefix_to_remove, const string& path, vector<FileToCompress>& file_paths,
                                               vector<string>& empty_directory_paths)
    {
        string path_without_prefix;
        if (false == remove_prefix_and_clean_up_path(path_prefix_to_remove, path, path_without_prefix)) {
            SPDLOG_ERROR("'{}' does not contain prefix '{}'.", path.c_str(), path_prefix_to_remove.c_str());
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
            boost::filesystem::recursive_directory_iterator iter(path, boost::filesystem::symlink_option::recurse);
            boost::filesystem::recursive_directory_iterator end;
            for (; iter != end; ++iter) {
                // Check if current entry is an empty directory or a file
                if (boost::filesystem::is_directory(iter->path())) {
                    if (boost::filesystem::is_empty(iter->path())) {
                        remove_prefix_and_clean_up_path(path_prefix_to_remove, iter->path(), path_without_prefix);
                        empty_directory_paths.push_back(path_without_prefix);
                        iter.no_push();
                    }
                } else {
                    remove_prefix_and_clean_up_path(path_prefix_to_remove, iter->path(), path_without_prefix);
                    file_paths.emplace_back(iter->path().string(), path_without_prefix, 0);
                }
            }
        } catch (boost::filesystem::filesystem_error& exception) {
            SPDLOG_ERROR("Failed to find files/directories at '{}' - {}.", path.c_str(), exception.what());
            return false;
        }

        return true;
    }

    bool is_utf8_sequence (size_t sequence_length, const char* sequence) {
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
                } // else byte is ASCII
            }
        }

        return true;
    }

    bool read_input_paths (const string& list_path, vector<string>& paths) {
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

    bool remove_prefix_and_clean_up_path (const boost::filesystem::path& prefix_to_remove, const boost::filesystem::path& path,
                                          string& path_without_prefix_string) {
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
        // NOTE: We initialize the path to '/' so that it remains an absolute path even if a prefix was removed
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

    bool validate_paths_exist (const vector<string>& paths) {
        // Ensure all paths in the list exist
        bool all_paths_exist = true;
        for (const auto& path : paths) {
            if (boost::filesystem::exists(path) == false) {
                SPDLOG_ERROR("'{}' does not exist.", path.c_str());
                all_paths_exist = false;
            }
        }

        return all_paths_exist;
    }

    void close_file_and_append_to_segment (streaming_archive::writer::Archive& archive_writer) {
        archive_writer.close_file();
        archive_writer.append_file_to_segment();
    }

    void split_archive (streaming_archive::writer::Archive::UserConfig& archive_user_config, streaming_archive::writer::Archive& archive_writer) {
        archive_writer.close();
        archive_user_config.id = boost::uuids::random_generator()();
        ++archive_user_config.creation_num;
        archive_writer.open(archive_user_config);
    }

    void split_file (const string& path_for_compression, group_id_t group_id, const TimestampPattern* last_timestamp_pattern,
                     streaming_archive::writer::Archive& archive_writer)
    {
        const auto& encoded_file = archive_writer.get_file();
        auto orig_file_id = encoded_file.get_orig_file_id();
        auto split_ix = encoded_file.get_split_ix();
        archive_writer.set_file_is_split(true);
        close_file_and_append_to_segment(archive_writer);

        archive_writer.create_and_open_file(path_for_compression, group_id, orig_file_id, ++split_ix);
        // Initialize the file's timestamp pattern to the previous split's pattern
        archive_writer.change_ts_pattern(last_timestamp_pattern);
    }

    void split_file_and_archive (streaming_archive::writer::Archive::UserConfig& archive_user_config, const string& path_for_compression, group_id_t group_id,
                                 const TimestampPattern* last_timestamp_pattern, streaming_archive::writer::Archive& archive_writer)
    {
        const auto& encoded_file = archive_writer.get_file();
        auto orig_file_id = encoded_file.get_orig_file_id();
        auto split_ix = encoded_file.get_split_ix();
        archive_writer.set_file_is_split(true);
        close_file_and_append_to_segment(archive_writer);

        split_archive(archive_user_config, archive_writer);

        archive_writer.create_and_open_file(path_for_compression, group_id, orig_file_id, ++split_ix);
        // Initialize the file's timestamp pattern to the previous split's pattern
        archive_writer.change_ts_pattern(last_timestamp_pattern);
    }
}
