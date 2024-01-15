#ifndef CLP_LOGTYPEDICTIONARYWRITER_HPP
#define CLP_LOGTYPEDICTIONARYWRITER_HPP

#include <memory>

#include "Defs.h"
#include "DictionaryWriter.hpp"
#include "FileWriter.hpp"
#include "LogTypeDictionaryEntry.hpp"

namespace clp {
/**
 * Class for performing operations on logtype dictionaries and writing them to disk
 */
class LogTypeDictionaryWriter
        : public DictionaryWriter<logtype_dictionary_id_t, LogTypeDictionaryEntry> {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}

        // Methods
        char const* what() const noexcept override {
            return "LogTypeDictionaryWriter operation failed";
        }
    };

    // Methods
    /**
     * Adds the given entry to the dictionary if it doesn't exist
     * @param logtype_entry
     * @param logtype_id ID of the logtype matching the given entry
     */
    bool add_entry(LogTypeDictionaryEntry& logtype_entry, logtype_dictionary_id_t& logtype_id);
};
}  // namespace clp

#endif  // CLP_LOGTYPEDICTIONARYWRITER_HPP
