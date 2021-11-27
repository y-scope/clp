#ifndef LOGTYPEDICTIONARYWRITER_HPP
#define LOGTYPEDICTIONARYWRITER_HPP

// C++ standard libraries
#include <memory>

// Project headers
#include "Defs.h"
#include "DictionaryWriter.hpp"
#include "FileWriter.hpp"
#include "LogTypeDictionaryEntry.hpp"

/**
 * Class for performing operations on logtype dictionaries and writing them to disk
 */
class LogTypeDictionaryWriter : public DictionaryWriter<logtype_dictionary_id_t, LogTypeDictionaryEntry> {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed (ErrorCode error_code, const char* const filename, int line_number) : TraceableException (error_code, filename, line_number) {}

        // Methods
        const char* what () const noexcept override {
            return "LogTypeDictionaryWriter operation failed";
        }
    };

    // Methods
    /**
     * Opens dictionary, loads entries, and then sets it up for writing
     * @param dictionary_path
     * @param segment_index_path
     * @param max_id
     */
    void open_and_preload (const std::string& dictionary_path, const std::string& segment_index_path, logtype_dictionary_id_t max_id);

    /**
     * Adds an entry to the dictionary if it doesn't exist, or increases its occurrence count if it does. If the entry does not exist, the entry pointer is
     * released from entry_wrapper and stored in the dictionary.
     * @param logtype_entry
     * @param logtype_id ID of the logtype matching the given entry
     */
    bool add_occurrence (LogTypeDictionaryEntry& logtype_entry, logtype_dictionary_id_t& logtype_id);
};

#endif // LOGTYPEDICTIONARYWRITER_HPP
