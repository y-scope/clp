#ifndef GLT_VARIABLEDICTIONARYENTRY_HPP
#define GLT_VARIABLEDICTIONARYENTRY_HPP

#include "Defs.h"
#include "DictionaryEntry.hpp"
#include "ErrorCode.hpp"
#include "FileReader.hpp"
#include "streaming_compression/zstd/Compressor.hpp"
#include "streaming_compression/zstd/Decompressor.hpp"

namespace glt {
/**
 * Class representing a variable dictionary entry
 */
class VariableDictionaryEntry : public DictionaryEntry<variable_dictionary_id_t> {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}

        // Methods
        char const* what() const noexcept override {
            return "VariableDictionaryEntry operation failed";
        }
    };

    // Constructors
    VariableDictionaryEntry() = default;

    VariableDictionaryEntry(std::string const& value, variable_dictionary_id_t id)
            : DictionaryEntry<variable_dictionary_id_t>(value, id) {}

    // Use default copy constructor
    VariableDictionaryEntry(VariableDictionaryEntry const&) = default;

    // Assignment operators
    // Use default
    VariableDictionaryEntry& operator=(VariableDictionaryEntry const&) = default;

    // Methods
    /**
     * Gets the size (in-memory) of the data contained in this entry
     * @return Size of the data contained in this entry
     */
    size_t get_data_size() const;

    void clear() { m_value.clear(); }

    /**
     * Writes an entry to file
     * @param compressor
     */
    void write_to_file(streaming_compression::Compressor& compressor) const;
    /**
     * Tries to read an entry from the given decompressor
     * @param decompressor
     * @return Same as streaming_compression::Decompressor::try_read_numeric_value
     * @return Same as streaming_compression::Decompressor::try_read_string
     */
    ErrorCode try_read_from_file(streaming_compression::Decompressor& decompressor);
    /**
     * Reads an entry from the given decompressor
     * @param decompressor
     */
    void read_from_file(streaming_compression::Decompressor& decompressor);
};
}  // namespace glt

#endif  // GLT_VARIABLEDICTIONARYENTRY_HPP
