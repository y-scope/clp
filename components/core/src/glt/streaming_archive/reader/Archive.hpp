#ifndef GLT_STREAMING_ARCHIVE_READER_ARCHIVE_HPP
#define GLT_STREAMING_ARCHIVE_READER_ARCHIVE_HPP

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
#include "SingleLogtypeTableManager.hpp"

namespace glt::streaming_archive::reader {
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

    // GLT TODO: deduplicate this and use the definition in Grep
    /**
     * Handles search result
     * @param orig_file_path Path of uncompressed file
     * @param compressed_msg
     * @param decompressed_msg
     * @param custom_arg Custom argument for the output function
     */
    using OutputFunc = void (*)(
            std::string const& orig_file_path,
            streaming_archive::reader::Message const& compressed_msg,
            std::string const& decompressed_msg,
            void* custom_arg
    );
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
     * Wrapper for streaming_archive::reader::File::get_next_message
     */
    bool get_next_message(File& file, Message& msg);

    /**
     * Decompresses a given message from a given file
     * @param file
     * @param compressed_msg
     * @param decompressed_msg
     * @return true if message was successfully decompressed, false otherwise
     * @throw TimestampPattern::OperationFailed if failed to insert timestamp
     */
    bool
    decompress_message(File& file, Message const& compressed_msg, std::string& decompressed_msg);

    void decompress_empty_directories(std::string const& output_dir);

    std::unique_ptr<MetadataDB::FileIterator> get_file_iterator() {
        return m_metadata_db
                .get_file_iterator(cEpochTimeMin, cEpochTimeMax, "", false, cInvalidSegmentId);
    }

    std::unique_ptr<MetadataDB::FileIterator> get_file_iterator(std::string const& file_path) {
        return m_metadata_db.get_file_iterator(
                cEpochTimeMin,
                cEpochTimeMax,
                file_path,
                false,
                cInvalidSegmentId
        );
    }

    std::unique_ptr<MetadataDB::FileIterator>
    get_file_iterator(epochtime_t begin_ts, epochtime_t end_ts, std::string const& file_path) {
        return m_metadata_db
                .get_file_iterator(begin_ts, end_ts, file_path, false, cInvalidSegmentId);
    }

    std::unique_ptr<MetadataDB::FileIterator> get_file_iterator(
            epochtime_t begin_ts,
            epochtime_t end_ts,
            std::string const& file_path,
            segment_id_t segment_id
    ) {
        return m_metadata_db.get_file_iterator(begin_ts, end_ts, file_path, true, segment_id);
    }

    // GLT search specific
    /**
     * This functions assumes a specific logtype is loaded with m_variable_column_manager.
     * The function takes in all logtype_query associated with the logtype,
     * and finds next matching message in the 2D variable table
     *
     * @param logtype_query
     * @param msg
     * @param wildcard (by reference)
     * @param query (to provide time range info)
     * @return Return true if a matching message is found. wildcard gets set to true if the matching
     * message still requires wildcard match
     * @throw Same as streaming_archive::reader::File::open_me
     */
    bool find_message_matching_with_logtype_query(
            std::vector<LogtypeQuery> const& logtype_query,
            Message& msg,
            bool& wildcard,
            Query const& query
    );
    /**
     * This functions assumes a specific logtype is loaded with m_variable_column_manager.
     * The function takes in all logtype_query associated with the logtype,
     * and finds next matching message in the 2D variable table
     *
     * @param logtype_query
     * @param matched_rows,
     * @param wildcard (by reference)
     * @param query (to provide time range info)
     * @return Return true if a matching message is found. wildcard gets set to true if the matching
     * message still requires wildcard match
     * @throw Same as streaming_archive::reader::File::open_me
     */
    void find_message_matching_with_logtype_query_optimized(
            std::vector<LogtypeQuery> const& logtype_query,
            std::vector<size_t>& matched_rows,
            std::vector<bool>& wildcard,
            Query const& query
    );
    bool find_message_matching_with_logtype_query_from_combined(
            std::vector<LogtypeQuery> const& logtype_query,
            Message& msg,
            bool& wildcard,
            Query const& query,
            size_t left,
            size_t right
    );

    /**
     * This functions assumes a specific logtype is loaded with m_variable_column_manager.
     * The function loads variable of the next message from the 2D variable table belonging to the
     * specific logtype. The variable are stored into the msg argument passed by reference
     *
     * @param msg
     * @return true if a row is successfully loaded into msg. false if the 2D table has reached the
     * end
     */
    bool get_next_message_in_logtype_table(Message& msg);

    // called upon opening the archive. figure out which segments
    // are valid (i.e. non-0 size)
    void update_valid_segment_ids();

    std::vector<size_t> get_valid_segment() const { return m_valid_segment_id; }

    // read the filename.dict that maps id to filename
    void load_filename_dict();

    std::string get_file_name(file_id_t file_id) const;

    streaming_archive::reader::SingleLogtypeTableManager& get_logtype_table_manager() {
        return m_logtype_table_manager;
    }

    void open_logtype_table_manager(size_t segment_id);
    void close_logtype_table_manager();

    // Message decompression methods
    size_t decompress_messages_and_output(
            logtype_dictionary_id_t logtype_id,
            std::vector<epochtime_t>& ts,
            std::vector<file_id_t>& id,
            std::vector<encoded_variable_t>& vars,
            std::vector<bool>& wildcard_required,
            Query const& query,
            OutputFunc output_func,
            void* output_func_arg
    );
    /**
     * Decompresses a given message using a fixed timestamp pattern
     * @param file
     * @param compressed_msg
     * @param decompressed_msg
     * @return true if message was successfully decompressed, false otherwise
     * @throw TimestampPattern::OperationFailed if failed to insert timestamp
     */
    bool decompress_message_with_fixed_timestamp_pattern(
            Message const& compressed_msg,
            std::string& decompressed_msg
    );

private:
    // Variables
    std::string m_id;
    std::string m_path;
    std::string m_segments_dir_path;
    LogTypeDictionaryReader m_logtype_dictionary;
    VariableDictionaryReader m_var_dictionary;

    MetadataDB m_metadata_db;

    // GLT Specific
    segment_id_t m_current_segment_id;
    GLTSegment m_segment;
    Segment m_message_order_table;

    // Search specific
    std::vector<size_t> m_valid_segment_id;
    streaming_archive::reader::SingleLogtypeTableManager m_logtype_table_manager;
    std::vector<std::string> m_filename_dict;
};
}  // namespace glt::streaming_archive::reader

#endif  // GLT_STREAMING_ARCHIVE_READER_ARCHIVE_HPP
