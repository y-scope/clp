#ifndef CLP_UTILS_HPP
#define CLP_UTILS_HPP

// C++ standard libraries
#include <string>

// Boost libraries
#include <boost/filesystem/path.hpp>

// Project headers
#include "../streaming_archive/writer/Archive.hpp"
#include "../streaming_archive/writer/File.hpp"
#include "FileToCompress.hpp"

namespace clp {
    /**
     * Recursively finds all files and empty directories at the given path
     * @param path_prefix_to_remove
     * @param path
     * @param file_paths
     * @param empty_directory_paths
     * @return true on success, false otherwise
     */
    bool find_all_files_and_empty_directories (boost::filesystem::path& path_prefix_to_remove, const std::string& path, std::vector<FileToCompress>& file_paths,
                                               std::vector<std::string>& empty_directory_paths);

    /**
     * Checks if the given sequence is valid UTF-8
     * @param sequence_length
     * @param sequence
     * @return true if valid, false otherwise
     */
    bool is_utf8_sequence (size_t sequence_length, const char* sequence);

    /**
     * Reads a list of input paths
     * @param list_path
     * @param paths
     * @return true on success, false otherwise
     */
    bool read_input_paths (const std::string& list_path, std::vector<std::string>& paths);

    /**
     * Removes the given prefix from the given path and cleans the path as follows:
     * - Removes redundant '.' and ".."
     * - Makes the path absolute
     * @param prefix_to_remove
     * @param path
     * @param path_without_prefix_string
     * @return false if the path didn't contain the prefix or it didn't contain anything besides the prefix, true otherwise
     */
    bool remove_prefix_and_clean_up_path (const boost::filesystem::path& prefix_to_remove, const boost::filesystem::path& path,
                                          std::string& path_without_prefix_string);

    /**
     * Validates that all paths in the given list exist
     * @param paths
     * @return true if they all exist, false otherwise
     */
    bool validate_paths_exist (const std::vector<std::string>& paths);

    /**
     * Creates and opens an in-memory file within the given archive
     * @param archive
     * @param path_for_compression
     * @param group_id
     * @param orig_file_id
     * @param split_ix
     * @return A pointer to the in-memory file
     */
    streaming_archive::writer::File* create_and_open_in_memory_file (streaming_archive::writer::Archive& archive, const std::string& path_for_compression,
                                                                     group_id_t group_id, const boost::uuids::uuid& orig_file_id, size_t split_ix);

    /**
     * Closes the given encoded file and marks it ready for a segment
     * @param archive
     * @param file
     */
    void close_file_and_mark_ready_for_segment (streaming_archive::writer::Archive& archive, streaming_archive::writer::File*& file);

    /**
     * Closes the current archive and starts a new one
     * @param archive_user_config
     * @param archive_writer
     */
    void split_archive (streaming_archive::writer::Archive::UserConfig& archive_user_config, streaming_archive::writer::Archive& archive_writer);

    /**
     * Closes the current encoded file and starts a new one
     * @param path_for_compression
     * @param group_id
     * @param last_timestamp_pattern
     * @param archive_writer
     * @param file
     */
    void split_file (const std::string& path_for_compression, group_id_t group_id, const TimestampPattern* last_timestamp_pattern,
                     streaming_archive::writer::Archive& archive_writer, streaming_archive::writer::File*& file);

    /**
     * Closes both the current encoded file and archive, then starts a new archive and encoded file
     * @param archive_user_config
     * @param path_for_compression
     * @param group_id
     * @param last_timestamp_pattern
     * @param archive_writer
     * @param file
     */
    void split_file_and_archive (streaming_archive::writer::Archive::UserConfig& archive_user_config, const std::string& path_for_compression,
                                 group_id_t group_id, const TimestampPattern* last_timestamp_pattern, streaming_archive::writer::Archive& archive_writer,
                                 streaming_archive::writer::File*& file);
}

#endif // CLP_UTILS_HPP
