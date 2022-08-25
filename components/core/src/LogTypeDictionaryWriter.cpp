#include "LogTypeDictionaryWriter.hpp"

// Project headers
#include "dictionary_utils.hpp"

using std::string;

bool LogTypeDictionaryWriter::add_entry (LogTypeDictionaryEntry& logtype_entry, logtype_dictionary_id_t& logtype_id) {
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
