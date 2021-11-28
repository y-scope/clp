#ifndef VARIABLEDICTIONARYWRITER_HPP
#define VARIABLEDICTIONARYWRITER_HPP

// Project headers
#include "Defs.h"
#include "DictionaryWriter.hpp"
#include "VariableDictionaryEntry.hpp"

/**
 * Class for performing operations on variable dictionaries and writing them to disk
 */
class VariableDictionaryWriter : public DictionaryWriter<variable_dictionary_id_t, VariableDictionaryEntry> {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed (ErrorCode error_code, const char* const filename, int line_number) : TraceableException (error_code, filename, line_number) {}

        // Methods
        const char* what () const noexcept override {
            return "VariableDictionaryWriter operation failed";
        }
    };

    /**
     * Adds an entry to the dictionary if it doesn't exist, or increases its occurrence count if it does
     * @param value
     * @param id
     */
    bool add_occurrence (const std::string& value, variable_dictionary_id_t& id);
};

#endif // VARIABLEDICTIONARYWRITER_HPP
