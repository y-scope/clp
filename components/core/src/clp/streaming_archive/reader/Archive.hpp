#ifndef CLP_STREAMING_ARCHIVE_READER_ARCHIVE_HPP
#define CLP_STREAMING_ARCHIVE_READER_ARCHIVE_HPP

#include <filesystem>
#include <iterator>
#include <list>
#include <memory>
#include <string>
#include <utility>

#include "../../ErrorCode.hpp"
#include "../../LogTypeDictionaryReader.hpp"
#include "../../Query.hpp"
#include "../../SQLiteDB.hpp"
#include "../../VariableDictionaryReader.hpp"
#include "../MetadataDB.hpp"
#include "File.hpp"
#include "Message.hpp"

namespace clp::streaming_archive::reader {
class Archive {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}

        // Methods
        char const* what() const noexcept override {
            return "streaming_archive::reader::Archive operation failed";
        }
    };

    // Methods
    /**
     * Opens archive for reading
     * @param path
     * @throw streaming_archive::reader::Archive::OperationFailed if could not stat file or it
     * isn't a directory or metadata is corrupted
     * @throw FileReader::OperationFailed if failed to open any dictionary
     */
    void open(std::string const& path);
    void close();

    /**
     * Reads any new entries added to the dictionaries
     * @throw Same as LogTypeDictionary::read_from_file and VariableDictionary::read_from_file
     */
    void refresh_dictionaries();
    LogTypeDictionaryReader const& get_logtype_dictionary() const;
    VariableDictionaryReader const& get_var_dictionary() const;

    /**
     * Opens file with given path
     * @param file
     * @param file_metadata_ix
     * @return Same as streaming_archive::reader::File::open_me
     */
    ErrorCode open_file(File& file, MetadataDB::FileIterator const& file_metadata_ix);
    /**
     * Wrapper for streaming_archive::reader::File::close_me
     * @param file
     */
    void close_file(File& file);
    /**
     * Wrapper for streaming_archive::reader::File::reset_indices
     * @param file
     */
    void reset_file_indices(File& file);

    /**
     * Wrapper for streaming_archive::reader::File::find_message_in_time_range
     */
    bool find_message_in_time_range(
            File& file,
            epochtime_t search_begin_timestamp,
            epochtime_t search_end_timestamp,
            Message& msg
    );
    /**
     * Wrapper for streaming_archive::reader::File::find_message_matching_query
     */
    SubQuery const* find_message_matching_query(File& file, Query const& query, Message& msg);
    /**
     * Wrapper for streaming_archive::reader::File::get_next_message
     */
    bool get_next_message(File& file, Message& msg);

    /**
     * Decompresses the given message from the given file, including inserting and formatting its
     * timestamp if necessary.
     * @param file
     * @param compressed_msg
     * @param decompressed_msg
     * @return true if message was successfully decompressed, false otherwise
     * @throw TimestampPattern::OperationFailed if failed to insert timestamp
     */
    bool
    decompress_message(File& file, Message const& compressed_msg, std::string& decompressed_msg);

    /**
     * Decompresses the given message without inserting its timestamp.
     * @param compressed_msg
     * @param decompressed_msg
     * @return Whether the message was successfully decompressed
     */
    bool
    decompress_message_without_ts(Message const& compressed_msg, std::string& decompressed_msg);

    void decompress_empty_directories(std::string const& output_dir);

    std::unique_ptr<MetadataDB::FileIterator> get_file_iterator_by_split_id(
            std::string const& file_split_id
    ) {
        return m_metadata_db.get_file_iterator(
                cEpochTimeMin,
                cEpochTimeMax,
                "",
                file_split_id,
                false,
                cInvalidSegmentId,
                false
        );
    }

    std::unique_ptr<MetadataDB::FileIterator> get_file_iterator_by_path(std::string const& file_path
    ) {
        return m_metadata_db.get_file_iterator(
                cEpochTimeMin,
                cEpochTimeMax,
                file_path,
                "",
                false,
                cInvalidSegmentId,
                false
        );
    }

    std::unique_ptr<MetadataDB::FileIterator> get_file_iterator() {
        return m_metadata_db.get_file_iterator(
                cEpochTimeMin,
                cEpochTimeMax,
                "",
                "",
                false,
                cInvalidSegmentId,
                false
        );
    }

    std::unique_ptr<MetadataDB::FileIterator> get_file_iterator(
            epochtime_t begin_ts,
            epochtime_t end_ts,
            std::string const& file_path,
            bool order_by_segment_end_ts
    ) {
        return m_metadata_db.get_file_iterator(
                begin_ts,
                end_ts,
                file_path,
                "",
                false,
                cInvalidSegmentId,
                order_by_segment_end_ts
        );
    }

    std::unique_ptr<MetadataDB::FileIterator> get_file_iterator(
            epochtime_t begin_ts,
            epochtime_t end_ts,
            std::string const& file_path,
            segment_id_t segment_id,
            bool order_by_segment_end_ts
    ) {
        return m_metadata_db.get_file_iterator(
                begin_ts,
                end_ts,
                file_path,
                "",
                true,
                segment_id,
                order_by_segment_end_ts
        );
    }

private:
    // Variables
    std::string m_id;
    std::string m_path;
    std::string m_segments_dir_path;
    LogTypeDictionaryReader m_logtype_dictionary;
    VariableDictionaryReader m_var_dictionary;

    SegmentManager m_segment_manager;

    MetadataDB m_metadata_db;
};
}  // namespace clp::streaming_archive::reader

#endif  // CLP_STREAMING_ARCHIVE_READER_ARCHIVE_HPP
