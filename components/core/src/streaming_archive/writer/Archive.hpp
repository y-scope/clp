#ifndef STREAMING_ARCHIVE_WRITER_ARCHIVE_HPP
#define STREAMING_ARCHIVE_WRITER_ARCHIVE_HPP

// C++ libraries
#include <memory>
#include <set>
#include <string>
#include <unordered_set>
#include <vector>
#include <filesystem>

// Boost libraries
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid.hpp>

// Project headers
#include "../../ArrayBackedPosIntSet.hpp"
#include "../../ErrorCode.hpp"
#include "../../GlobalMetadataDB.hpp"
#include "../../LogTypeDictionaryWriter.hpp"
#include "../../VariableDictionaryWriter.hpp"
#include "../../compressor_frontend/Token.hpp"
#include "../MetadataDB.hpp"

namespace streaming_archive { namespace writer { 
    class Archive {
    public:
        // Types
        /**
         * Structure used to pass settings when opening a new archive
         * @param id
         * @param creator_id
         * @param creation_num
         * @param target_segment_uncompressed_size
         * @param compression_level Compression level of the compressor being opened
         * @param output_dir Output directory
         * @param global_metadata_db
         * @param print_archive_stats_progress Enable printing statistics about the archive as it's compressed
         */
        struct UserConfig {
            boost::uuids::uuid id;
            boost::uuids::uuid creator_id;
            size_t creation_num;
            size_t target_segment_uncompressed_size;
            int compression_level;
            std::string output_dir;
            GlobalMetadataDB* global_metadata_db;
            bool print_archive_stats_progress;
        };

        class OperationFailed : public TraceableException {
        public:
            // Constructors
            OperationFailed (ErrorCode error_code, const char* const filename, int line_number) : TraceableException (error_code, filename, line_number) {}

            // Methods
            const char* what () const noexcept override {
                return "streaming_archive::writer::Archive operation failed";
            }
        };

        TimestampPattern old_ts_pattern;

        size_t m_target_data_size_of_dicts;
        UserConfig m_archive_user_config;
        std::string m_path_for_compression;
        group_id_t m_group_id;
        size_t m_target_encoded_file_size;
        std::string m_schema_file_path;

        // Constructors
        Archive () : m_segments_dir_fd(-1), m_compression_level(0), m_global_metadata_db(nullptr),
                old_ts_pattern(), m_schema_file_path() {}

        // Destructor
        ~Archive ();

        // Methods
        /**
         * Creates the directory structure for the archive and opens writers for the dictionaries
         * @param user_config Settings configurable by the user
         * @throw FileWriter::OperationFailed if any dictionary writer could not be opened
         * @throw streaming_archive::writer::Archive::OperationFailed if archive already exists, if it could not be stat-ed, if the directory structure could
                  not be created, if the file is not reset or problems with medatadata.
         */
        void open (const UserConfig& user_config);
        /**
         * Writes a final snapshot of the archive, closes all open files, and closes the dictionaries
         * @throw FileWriter::OperationFailed if any writer could not be closed
         * @throw streaming_archive::writer::Archive::OperationFailed if any empty directories could not be removed
         * @throw streaming_archive::writer::Archive::OperationFailed if the file is not reset
         * @throw Same as streaming_archive::writer::SegmentManager::close
         * @throw Same as streaming_archive::writer::Archive::write_dir_snapshot
         */
        void close ();

        /**
         * Creates and opens a file with the given path
         * @param path
         * @param group_id
         * @param orig_file_id
         * @param split_ix
         * @return Pointer to the new file
         */
        void create_and_open_file (const std::string& path, group_id_t group_id, const boost::uuids::uuid& orig_file_id, size_t split_ix);

        void close_file ();

        const File& get_file () const;

        /**
         * Sets the split status of the current encoded file
         * @param is_split
         */
        void set_file_is_split (bool is_split);

        /**
         * Wrapper for streaming_archive::writer::File::change_ts_pattern
         * @param pattern
         */
        void change_ts_pattern (const TimestampPattern* pattern);
        /**
         * Encodes and writes a message to the current encoded file
         * @param timestamp
         * @param message
         * @param num_uncompressed_bytes
         * @throw FileWriter::OperationFailed if any write fails
         */
        void write_msg (epochtime_t timestamp, const std::string& message, size_t num_uncompressed_bytes);
        /**
         * Encodes and writes a message to the given file using schema file
         * @param file
         * @param uncompressed_msg
         * @param uncompressed_msg_pos
         * @param has_delimiter
         * @param has_timestamp
         * @throw FileWriter::OperationFailed if any write fails
         */
        void write_msg_using_schema (compressor_frontend::Token*& uncompressed_msg, uint32_t uncompressed_msg_pos, bool has_delimiter, bool has_timestamp);

        /**
         * Writes snapshot of archive to disk including metadata of all files and new dictionary entries
         * @throw FileWriter::OperationFailed if failed to write or flush dictionaries
         * @throw std::out_of_range if dictionary ID unexpectedly didn't exist
         * @throw Same as streaming_archive::writer::Archive::persist_file_metadata
         */
        void write_dir_snapshot ();

        /**
         * Adds the encoded file to the segment
         * @throw streaming_archive::writer::Archive::OperationFailed if failed the file is not tracked by the current archive
         * @throw Same as streaming_archive::writer::Archive::persist_file_metadata
         */
        void append_file_to_segment ();

        /**
         * Adds empty directories to the archive
         * @param empty_directory_paths
         * @throw streaming_archive::writer::Archive::OperationFailed if failed to insert paths to the database
         */
        void add_empty_directories (const std::vector<std::string>& empty_directory_paths);

        const boost::uuids::uuid& get_id () const { return m_id; }
        const std::string& get_id_as_string () const { return m_id_as_string; }

        size_t get_data_size_of_dictionaries () const { return m_logtype_dict.get_data_size() + m_var_dict.get_data_size(); }

    private:
        // Types
        /**
         * Custom less-than comparator for sets to:
         * - Primary sort order File pointers in increasing order of their group ID, then
         * - Secondary sort order File pointers in increasing order of their end timestamp, then
         * - Tertiary sort order File pointers in alphabetical order of their paths, then
         * - Determine uniqueness by their ID
         */
        class FileGroupIdAndEndTimestampLTSetComparator {
        public:
            // Methods
            bool operator() (const File* lhs, const File* rhs) const {
                // Primary sort by file's group ID
                if (lhs->get_group_id() != rhs->get_group_id()) {
                    return lhs->get_group_id() < rhs->get_group_id();
                } else {
                    // Secondary sort by file's end timestamp, from earliest to latest
                    if (lhs->get_end_ts() != rhs->get_end_ts()) {
                        return lhs->get_end_ts() < rhs->get_end_ts();
                    } else {
                        // Tertiary sort by file path, alphabetically
                        if (lhs->get_orig_path() != rhs->get_orig_path()) {
                            return lhs->get_orig_path() < rhs->get_orig_path();
                        } else {
                            return lhs->get_id() < rhs->get_id();
                        }
                    }
                }
            }
        };

        // Methods
        /**
         * Appends the content of the current encoded file to the given segment
         * @param segment
         * @param logtype_ids_in_segment
         * @param var_ids_in_segment
         * @param files_in_segment
         */
        void append_file_contents_to_segment (Segment& segment, ArrayBackedPosIntSet<logtype_dictionary_id_t>& logtype_ids_in_segment,
                                              ArrayBackedPosIntSet<variable_dictionary_id_t>& var_ids_in_segment, std::vector<File*>& files_in_segment);
        /**
         * Writes the given files' metadata to the database using bulk writes
         * @param files
         * @throw streaming_archive::writer::Archive::OperationFailed if failed to replace old metadata for any file
         * @throw mongocxx::logic_error if invalid database operation is created
         */
        void persist_file_metadata (const std::vector<File*>& files);
        /**
         * Closes a given segment, persists the metadata of the files in the segment, and cleans up any data remaining outside the segment
         * @param segment
         * @param files
         * @param segment_logtype_ids
         * @param segment_var_ids
         * @throw Same as streaming_archive::writer::Segment::close
         * @throw Same as streaming_archive::writer::Archive::persist_file_metadata
         */
        void close_segment_and_persist_file_metadata (Segment& segment, std::vector<File*>& files,
                                                      ArrayBackedPosIntSet<logtype_dictionary_id_t>& segment_logtype_ids,
                                                      ArrayBackedPosIntSet<variable_dictionary_id_t>& segment_var_ids);

        /**
         * Gets the size of uncompressed data that has been compressed into the archive and will not be changed
         * @return Size in bytes
         */
        size_t get_stable_uncompressed_size () const;
        /**
         * Gets the size of the portion of the archive that will not be changed
         * @return Size in bytes
         */
        size_t get_stable_size () const;
        /**
         * Updates the archive's metadata
         */
        void update_metadata ();

        // Variables
        boost::uuids::uuid m_id;
        std::string m_id_as_string;

        // Used to order the archives created by a single thread
        // NOTE: This is necessary because files may be split across archives and we want to decompress their parts in order.
        boost::uuids::uuid m_creator_id;
        std::string m_creator_id_as_string;
        size_t m_creation_num;

        std::string m_path;
        std::string m_segments_dir_path;
        int m_segments_dir_fd;

        // Holds the file being compressed
        File* m_file;

        LogTypeDictionaryWriter m_logtype_dict;
        // Holds preallocated logtype dictionary entry for performance
        LogTypeDictionaryEntry m_logtype_dict_entry;
        std::vector<encoded_variable_t> m_encoded_vars;
        std::vector<variable_dictionary_id_t> m_var_ids;
        VariableDictionaryWriter m_var_dict;

        boost::uuids::random_generator m_uuid_generator;

        file_id_t m_next_file_id;
        // Since we batch metadata persistence operations, we need to keep track of files whose metadata should be persisted
        // Accordingly:
        // - m_files_with_timestamps_in_segment contains files that 1) have been moved to an open segment and 2) contain timestamps
        // - m_files_without_timestamps_in_segment contains files that 1) have been moved to an open segment and 2) do not contain timestamps
        segment_id_t m_next_segment_id;
        std::vector<File*> m_files_with_timestamps_in_segment;
        std::vector<File*> m_files_without_timestamps_in_segment;

        size_t m_target_segment_uncompressed_size;
        Segment m_segment_for_files_with_timestamps;
        ArrayBackedPosIntSet<logtype_dictionary_id_t> m_logtype_ids_in_segment_for_files_with_timestamps;
        ArrayBackedPosIntSet<variable_dictionary_id_t> m_var_ids_in_segment_for_files_with_timestamps;
        // Logtype and variable IDs for a file that hasn't yet been assigned to the timestamp or timestamp-less segment
        std::unordered_set<logtype_dictionary_id_t> m_logtype_ids_for_file_with_unassigned_segment;
        std::unordered_set<variable_dictionary_id_t> m_var_ids_for_file_with_unassigned_segment;
        Segment m_segment_for_files_without_timestamps;
        ArrayBackedPosIntSet<logtype_dictionary_id_t> m_logtype_ids_in_segment_for_files_without_timestamps;
        ArrayBackedPosIntSet<variable_dictionary_id_t> m_var_ids_in_segment_for_files_without_timestamps;

        size_t m_stable_uncompressed_size;
        size_t m_stable_size;

        int m_compression_level;

        MetadataDB m_metadata_db;

        FileWriter m_metadata_file_writer;

        GlobalMetadataDB* m_global_metadata_db;

        bool m_print_archive_stats_progress;
    };
} }

#endif // STREAMING_ARCHIVE_WRITER_ARCHIVE_HPP
