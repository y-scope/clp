#include "LogTypeDictionaryWriter.hpp"

// Project headers
#include "dictionary_utils.hpp"

using std::string;

void LogTypeDictionaryWriter::open_and_preload (const std::string& dictionary_path, const std::string& segment_index_path, logtype_dictionary_id_t max_id) {
    if (m_is_open) {
        throw OperationFailed(ErrorCode_NotReady, __FILENAME__, __LINE__);
    }

    m_max_id = max_id;

    FileReader dictionary_file_reader;
    streaming_compression::zstd::Decompressor dictionary_decompressor;
    FileReader segment_index_file_reader;
    streaming_compression::zstd::Decompressor segment_index_decompressor;
    constexpr size_t cDecompressorFileReadBufferCapacity = 64 * 1024; // 64 KB
    open_dictionary_for_reading(dictionary_path, segment_index_path, cDecompressorFileReadBufferCapacity, dictionary_file_reader, dictionary_decompressor,
                                segment_index_file_reader, segment_index_decompressor);

    preload_dictionary_value_to_map(dictionary_decompressor, dictionary_file_reader);

    segment_index_decompressor.close();
    segment_index_file_reader.close();
    dictionary_decompressor.close();
    dictionary_file_reader.close();

    m_dictionary_file_writer.open(dictionary_path, FileWriter::OpenMode::CREATE_IF_NONEXISTENT_FOR_SEEKABLE_WRITING);
    // Open compressor
    m_dictionary_compressor.open(m_dictionary_file_writer);

    m_segment_index_file_writer.open(segment_index_path, FileWriter::OpenMode::CREATE_IF_NONEXISTENT_FOR_SEEKABLE_WRITING);
    // Open compressor
    m_segment_index_compressor.open(m_segment_index_file_writer);

    m_is_open = true;
}

bool LogTypeDictionaryWriter::add_occurrence (LogTypeDictionaryEntry& logtype_entry, logtype_dictionary_id_t& logtype_id) {
    if (logtype_entry.get_value().empty()) {
        throw OperationFailed(ErrorCode_BadParam, __FILENAME__, __LINE__);
    }

    bool is_new_entry = false;

    const string& value = logtype_entry.get_value();
    const auto ix = m_value_to_id.find(value);
    if (m_value_to_id.end() != ix) {
        // Entry exists so get its ID
        logtype_id = ix->second;
    } else {
        // Dictionary entry doesn't exist so create it

        // Determine verbosity
        LogVerbosity verbosity;
        if (string::npos != value.find("FATAL")) {
            verbosity = LogVerbosity_FATAL;
        } else if (string::npos != value.find("ERROR")) {
            verbosity = LogVerbosity_ERROR;
        } else if (string::npos != value.find("WARN")) {
            verbosity = LogVerbosity_WARN;
        } else if (string::npos != value.find("INFO")) {
            verbosity = LogVerbosity_INFO;
        } else if (string::npos != value.find("DEBUG")) {
            verbosity = LogVerbosity_DEBUG;
        } else if (string::npos != value.find("TRACE")) {
            verbosity = LogVerbosity_TRACE;
        } else {
            verbosity = LogVerbosity_UNKNOWN;
        }
        logtype_entry.set_verbosity(verbosity);

        // Assign ID
        logtype_id = m_next_id;
        ++m_next_id;
        logtype_entry.set_id(logtype_id);

        // Insert new entry into dictionary
        m_value_to_id[value] = logtype_id;

        is_new_entry = true;

        // TODO: This doesn't account for the segment index that's constantly updated
        m_data_size += logtype_entry.get_data_size();

        logtype_entry.write_to_file(m_dictionary_compressor);
    }
    return is_new_entry;
}
