#ifndef TIMESTAMPDICTIONARYWRITER_HPP
#define TIMESTAMPDICTIONARYWRITER_HPP

// Project headers
#include "Defs.h"
#include "DictionaryWriter.hpp"
#include "TimestampDictionaryEntry.hpp"

/**
 * Class for performing operations on timestamp dictionaries and writing them to disk
 */
class TimestampDictionaryWriter : public DictionaryWriter<timestamp_dictionary_id_t, TimestampDictionaryEntry> {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed (ErrorCode error_code, const char* const filename, int line_number) : TraceableException (error_code, filename, line_number) {}

        // Methods
        const char* what () const noexcept override {
            return "TimestampDictionaryWriter operation failed";
        }
    };

    // Constructor
    TimestampDictionaryWriter (): DictionaryWriter<timestamp_dictionary_id_t, TimestampDictionaryEntry> (false) {}

    // Methods
    /**
     * Adds the given timestamp to the dictionary if it doesn't exist.
     * @param value
     * @param id ID of the timestamp matching the given entry
     */
    bool add_entry (const std::string& value, timestamp_dictionary_id_t& id);
};

#endif // TIMESTAMPDICTIONARYWRITER_HPP
