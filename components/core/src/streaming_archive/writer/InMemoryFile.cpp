#include "InMemoryFile.hpp"

// Project headers
#include "../../EncodedVariableInterpreter.hpp"
#include "../../Utils.hpp"

using std::string;
using std::unordered_set;
using std::vector;

namespace streaming_archive { namespace writer {
    InMemoryFile::InMemoryFile (const boost::uuids::uuid& id, const boost::uuids::uuid& orig_file_id, const string& orig_log_path,
                                const string& archive_log_path, const group_id_t group_id, size_t split_ix) :
            File(id, orig_file_id, orig_log_path, group_id, split_ix), m_is_written_out(false), m_is_open(false)
    {
        m_archive_log_path = archive_log_path;
        m_archive_log_path += get_id_as_string();
    }

    void InMemoryFile::open () {
        if (m_is_written_out) {
            throw OperationFailed(ErrorCode_Unsupported, __FILENAME__, __LINE__);
        }

        m_is_open = true;
    }

    void InMemoryFile::close () {
        m_is_open = false;
    }

    bool InMemoryFile::is_open () const {
        return m_is_open;
    }

    void InMemoryFile::write_encoded_msg (epochtime_t timestamp, logtype_dictionary_id_t logtype_id, const vector<encoded_variable_t>& encoded_vars,
            size_t num_uncompressed_bytes)
    {
        m_timestamps.push_back(timestamp);
        m_logtypes.push_back(logtype_id);
        m_variables.push_back_all(encoded_vars);
        increment_num_messages_and_variables(1, encoded_vars.size());

        set_last_message_timestamp(timestamp);
        increment_num_uncompressed_bytes(num_uncompressed_bytes);
    }

    void InMemoryFile::append_to_segment (const LogTypeDictionaryWriter& logtype_dict, Segment& segment,
                                          unordered_set<logtype_dictionary_id_t>& segment_logtype_ids, unordered_set<variable_dictionary_id_t>& segment_var_ids)
    {
        if (m_is_open) {
            throw OperationFailed(ErrorCode_Unsupported, __FILENAME__, __LINE__);
        }

        // Add file's logtype and variable IDs to respective segment sets
        auto logtype_ids = m_logtypes.data();
        auto variables = m_variables.data();
        append_logtype_and_var_ids_to_segment_sets(logtype_dict, logtype_ids, m_logtypes.size(), variables, m_variables.size(), segment_logtype_ids,
                                                   segment_var_ids);

        // Append files to segment
        uint64_t segment_timestamps_uncompressed_pos;
        segment.append(reinterpret_cast<const char*>(m_timestamps.data()), m_timestamps.size_in_bytes(), segment_timestamps_uncompressed_pos);
        uint64_t segment_logtypes_uncompressed_pos;
        segment.append(reinterpret_cast<const char*>(m_logtypes.data()), m_logtypes.size_in_bytes(), segment_logtypes_uncompressed_pos);
        uint64_t segment_variables_uncompressed_pos;
        segment.append(reinterpret_cast<const char*>(m_variables.data()), m_variables.size_in_bytes(), segment_variables_uncompressed_pos);
        set_segment_metadata(segment.get_id(), segment_timestamps_uncompressed_pos, segment_logtypes_uncompressed_pos, segment_variables_uncompressed_pos);
        m_segmentation_state = SegmentationState_MovingToSegment;

        // Mark file as written out and clear in-memory columns
        m_is_written_out = true;
        m_timestamps.clear();
        m_logtypes.clear();
        m_variables.clear();
    }

    void InMemoryFile::cleanup_after_segment_insertion () {
        // Nothing to do
    }

    void InMemoryFile::write_to_disk () {
        string column_path;
        FileWriter file_writer;

        // Write timestamps
        column_path = m_archive_log_path;
        column_path += cTimestampsFileExtension;
        file_writer.open(column_path, FileWriter::OpenMode::CREATE_FOR_WRITING);
        file_writer.write(reinterpret_cast<char*>(m_timestamps.data()), m_timestamps.size_in_bytes());
        file_writer.flush();
        file_writer.close();

        // Write logtypes
        column_path = m_archive_log_path;
        column_path += cLogTypeIdsFileExtension;
        file_writer.open(column_path, FileWriter::OpenMode::CREATE_FOR_WRITING);
        file_writer.write(reinterpret_cast<char*>(m_logtypes.data()), m_logtypes.size_in_bytes());
        file_writer.flush();
        file_writer.close();

        // Write variables
        column_path = m_archive_log_path;
        column_path += cVariablesFileExtension;
        file_writer.open(column_path, FileWriter::OpenMode::CREATE_FOR_WRITING);
        file_writer.write(reinterpret_cast<char*>(m_variables.data()), m_variables.size_in_bytes());
        file_writer.flush();
        file_writer.close();

        // Mark file as written out and clear in-memory columns
        m_is_written_out = true;
        m_timestamps.clear();
        m_logtypes.clear();
        m_variables.clear();
    }
} }