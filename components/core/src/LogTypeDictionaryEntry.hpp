#ifndef LOGTYPEDICTIONARYENTRY_HPP
#define LOGTYPEDICTIONARYENTRY_HPP

// C++ standard libraries
#include <vector>

// Project headers
#include "Defs.h"
#include "DictionaryEntry.hpp"
#include "ErrorCode.hpp"
#include "FileReader.hpp"
#include "streaming_compression/zstd/Compressor.hpp"
#include "streaming_compression/zstd/Decompressor.hpp"
#include "TraceableException.hpp"
#include "type_utils.hpp"

/**
 * Class representing a logtype dictionary entry
 */
class LogTypeDictionaryEntry : public DictionaryEntry<logtype_dictionary_id_t> {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed (ErrorCode error_code, const char* const filename, int line_number) : TraceableException (error_code, filename, line_number) {}

        // Methods
        const char* what () const noexcept override {
            return "LogTypeDictionaryEntry operation failed";
        }
    };

    // Constants
    enum class VarDelim : char {
        // NOTE: These values are used within logtypes to denote variables, so care must be taken when changing them
        Integer = 0x11,
        Dictionary = 0x12,
        Float = 0x13,
        Length = 3
    };

    // Constructors
    LogTypeDictionaryEntry () : m_verbosity(LogVerbosity_Length) {}
    // Use default copy constructor
    LogTypeDictionaryEntry (const LogTypeDictionaryEntry&) = default;

    // Assignment operators
    // Use default
    LogTypeDictionaryEntry& operator= (const LogTypeDictionaryEntry&) = default;

    // Methods
    /**
     * Adds a dictionary variable delimiter to the given logtype
     * @param logtype
     */
    static void add_dict_var (std::string& logtype) {
        logtype += enum_to_underlying_type(VarDelim::Dictionary);
    }
    /**
     * Adds an integer variable delimiter to the given logtype
     * @param logtype
     */
    static void add_int_var (std::string& logtype) {
        logtype += enum_to_underlying_type(VarDelim::Integer);
    }
    /**
     * Adds a float variable delimiter to the given logtype
     * @param logtype
     */
    static void add_float_var (std::string& logtype) {
        logtype += enum_to_underlying_type(VarDelim::Float);
    }

    size_t get_num_vars () const { return m_var_positions.size(); }
    /**
     * Gets all info about a variable in the logtype
     * @param var_ix The index of the variable to get the info for
     * @param var_delim
     * @return The variable's position in the logtype, or SIZE_MAX if var_ix is out of bounds
     */
    size_t get_var_info (size_t var_ix, VarDelim& var_delim) const;

    /**
     * Gets the size (in-memory) of the data contained in this entry
     * @return Size of the data contained in this entry
     */
    size_t get_data_size () const;

    /**
     * Adds a constant to the logtype
     * @param value_containing_constant
     * @param begin_pos Start of the constant in value_containing_constant
     * @param length
     */
    void add_constant (const std::string& value_containing_constant, size_t begin_pos, size_t length);
    /**
     * Adds an int variable delimiter
     */
    void add_int_var ();
    /**
     * Adds a float variable delimiter
     */
    void add_float_var ();
    /**
     * Adds a dictionary variable delimiter
     */
    void add_dictionary_var ();

    /**
     * Parses next variable from a message, constructing the constant part of the message's logtype as well
     * @param msg
     * @param var_begin_pos Beginning position of last variable. Changes to beginning position of current variable.
     * @param var_end_pos End position of last variable (exclusive). Changes to end position of current variable.
     * @param var
     * @return true if another variable was found, false otherwise
     */
    bool parse_next_var (const std::string& msg, size_t& var_begin_pos, size_t& var_end_pos, std::string& var);

    /**
     * Reserves space for a constant of the given length
     * @param length
     */
    void reserve_constant_length (size_t length) { m_value.reserve(length); }
    void set_id (logtype_dictionary_id_t id) { m_id = id; }
    void set_verbosity (LogVerbosity verbosity) { m_verbosity = verbosity; }

    void clear ();

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

private:
    // Methods
    /**
     * Escapes any variable delimiters that don't correspond to the positions of variables in the logtype entry's value
     * @param escaped_logtype_value
     */
    void get_value_with_unfounded_variables_escaped (std::string& escaped_logtype_value) const;

    // Variables
    LogVerbosity m_verbosity;
    std::vector<size_t> m_var_positions;
};

#endif // LOGTYPEDICTIONARYENTRY_HPP
