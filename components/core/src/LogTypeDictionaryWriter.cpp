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

    auto num_dictionary_entries = read_dictionary_header(dictionary_file_reader);

    // Read new dictionary entries
    logtype_dictionary_id_t id;
    for (size_t i = 0; i < num_dictionary_entries; ++i) {
        auto logtype_dict_entry_wrapper = std::make_unique<LogTypeDictionaryEntry>();
        logtype_dict_entry_wrapper->read_from_file(dictionary_decompressor);
        add_occurrence(logtype_dict_entry_wrapper, id);
    }

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

bool LogTypeDictionaryWriter::add_occurrence (std::unique_ptr<LogTypeDictionaryEntry>& entry_wrapper, logtype_dictionary_id_t& logtype_id) {
    if (nullptr == entry_wrapper) {
        throw OperationFailed(ErrorCode_BadParam, __FILENAME__, __LINE__);
    }
    auto& entry = *entry_wrapper;

    bool is_new_entry = false;

    const string& value = entry.get_value();
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
        entry.set_verbosity(verbosity);

        // Assign ID
        logtype_id = m_next_id;
        ++m_next_id;
        entry.set_id(logtype_id);

        // Insert new entry into dictionary
        auto entry_ptr = entry_wrapper.release();
        m_value_to_id[value] = logtype_id;

        is_new_entry = true;

        // TODO: This doesn't account for the segment index that's constantly updated
        m_data_size += entry_ptr->get_data_size();

        entry_ptr->write_to_file(m_dictionary_compressor);
        delete entry_ptr;
    }
    return is_new_entry;
}
