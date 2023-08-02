#ifndef TIMESTAMPDICTIONARYENTRY_HPP
#define TIMESTAMPDICTIONARYENTRY_HPP

#include "Defs.h"
#include "DictionaryEntry.hpp"
#include "ErrorCode.hpp"
#include "streaming_compression/zstd/Compressor.hpp"
#include "streaming_compression/zstd/Decompressor.hpp"
#include "TraceableException.hpp"

/**
 * Class representing a timestamp dictionary entry
 */
class TimestampDictionaryEntry : public DictionaryEntry<timestamp_dictionary_id_t> {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed (ErrorCode error_code, const char* const filename, int line_number) : TraceableException (error_code, filename, line_number) {}

        // Methods
        const char* what () const noexcept override {
            return "TimestampDictionaryEntry operation failed";
        }
    };

    // Constructors
    TimestampDictionaryEntry () = default;
    TimestampDictionaryEntry (const std::string& value, timestamp_dictionary_id_t id) : DictionaryEntry<timestamp_dictionary_id_t>(value, id) {}

    // Use default copy constructor
    TimestampDictionaryEntry (const TimestampDictionaryEntry&) = default;

    // Assignment operators
    // Use default
    TimestampDictionaryEntry& operator= (const TimestampDictionaryEntry&) = default;

    // Methods
    /**
     * Gets the size (in-memory) of the data contained in this entry
     * @return Size of the data contained in this entry
     */
    size_t get_data_size () const;

    void clear () { m_value.clear(); }

    /**
     * Writes an entry to file
     * @param compressor
     */
    void write_to_file (streaming_compression::Compressor& compressor) const;
    /**
     * Tries to read an entry from the given decompressor
     * @param decompressor
     * @return Same as streaming_compression::Decompressor::try_read_numeric_value
     * @return Same as streaming_compression::Decompressor::try_read_string
     */
    ErrorCode try_read_from_file (streaming_compression::Decompressor& decompressor);
    /**
     * Reads an entry from the given decompressor
     * @param decompressor
     */
    void read_from_file (streaming_compression::Decompressor& decompressor);
};

#endif // TIMESTAMPDICTIONARYENTRY_HPP
