// Code from CLP

#ifndef CLP_S_DICTIONARYENTRY_HPP
#define CLP_S_DICTIONARYENTRY_HPP

#include <string>
#include <utility>

#include "TraceableException.hpp"
#include "ZstdCompressor.hpp"
#include "ZstdDecompressor.hpp"

namespace clp_s {
/**
 * Template class representing a dictionary entry
 * @tparam DictionaryIdType
 */
template <typename DictionaryIdType>
class DictionaryEntry {
public:
    // Constructors
    DictionaryEntry() = default;

    DictionaryEntry(std::string value, DictionaryIdType id) : m_value(std::move(value)), m_id(id) {}

    // Methods
    DictionaryIdType get_id() const { return m_id; }

    std::string const& get_value() const { return m_value; }

protected:
    // Variables
    DictionaryIdType m_id;
    std::string m_value;
};

/**
 * Class representing a logtype dictionary entry
 */
class LogTypeDictionaryEntry : public DictionaryEntry<uint64_t> {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}
    };

    // Constants
    enum class VarDelim {
        // NOTE: These values are used within logtypes to denote variables, so care must be taken
        // when changing them
        NonDouble = 17,
        Double = 18,
        Length = 2,
    };

    static constexpr char cEscapeChar = '\\';

    // Constructors
    LogTypeDictionaryEntry() : m_init(false) {}

    // Use default copy constructor
    LogTypeDictionaryEntry(LogTypeDictionaryEntry const&) = default;

    // Use default assignment operators
    LogTypeDictionaryEntry& operator=(LogTypeDictionaryEntry const&) = default;

    // Methods
    /**
     * Adds a non-double variable delimiter to the given logtype
     * @param logtype
     */
    static void add_non_double_var(std::string& logtype) { logtype += (char)VarDelim::NonDouble; }

    /**
     * Adds a double variable delimiter to the given logtype
     * @param logtype
     */
    static void add_double_var(std::string& logtype) { logtype += (char)VarDelim::Double; }

    /**
     * @return The number of variables in the logtype
     */
    size_t get_num_vars() const { return m_var_positions.size(); }

    /**
     * Gets all info about a variable in the logtype
     * @param var_ix The index of the variable to get the info for
     * @param var_delim
     * @return The variable's position in the logtype, or SIZE_MAX if var_ix is out of bounds
     */
    size_t get_var_info(size_t var_ix, VarDelim& var_delim) const;

    /**
     * Gets the variable delimiter at the given index
     * @param var_ix The index of the variable delimiter to get
     * @return The variable delimiter, or LogTypeDictionaryEntry::VarDelim::Length if var_ix is out
     * of bounds
     */
    VarDelim get_var_delim(size_t var_ix) const;

    /**
     * Gets the length of the specified variable's representation in the logtype
     * @param var_ix The index of the variable
     * @return The length
     */
    size_t get_var_length_in_logtype(size_t var_ix) const;

    /**
     * Gets the size (in-memory) of the data contained in this entry
     * @return Size of the data contained in this entry
     */
    size_t get_data_size() const;

    /**
     * Adds a constant to the logtype
     * @param value_containing_constant
     * @param begin_pos Start of the constant in value_containing_constant
     * @param length
     */
    void
    add_constant(std::string const& value_containing_constant, size_t begin_pos, size_t length);

    /**
     * Adds a non-double variable delimiter
     */
    void add_non_double_var();

    /**
     * Adds a double variable delimiter
     */
    void add_double_var();

    /**
     * Parses next variable from a message, constructing the constant part of the message's logtype
     * as well
     * @param msg
     * @param var_begin_pos Beginning position of last variable. Changes to beginning position of
     * current variable.
     * @param var_end_pos End position of last variable (exclusive). Changes to end position of
     * current variable.
     * @param var
     * @return true if another variable was found, false otherwise
     */
    bool parse_next_var(
            std::string const& msg,
            size_t& var_begin_pos,
            size_t& var_end_pos,
            std::string& var
    );

    /**
     * Reserves space for a constant of the given length
     * @param length
     */
    void reserve_constant_length(size_t length) { m_value.reserve(length); }

    void set_id(uint64_t id) { m_id = id; }

    /**
     * Clears the entry
     */
    void clear();

    /**
     * Writes an entry to a compressed file
     * @param compressor
     */
    void write_to_file(ZstdCompressor& compressor) const;

    /**
     * Tries to read an entry from the given decompressor
     * @param decompressor
     * @return Same as streaming_compression::Decompressor::try_read_numeric_value
     * @return Same as streaming_compression::Decompressor::try_read_string
     */
    ErrorCode try_read_from_file(ZstdDecompressor& decompressor, uint64_t id, bool lazy);

    /**
     * Reads an entry from the given decompressor
     * @param decompressor
     * @param lazy apply lazy decoding
     */
    void read_from_file(ZstdDecompressor& decompressor, uint64_t id, bool lazy);

    /**
     * Decodes the log type
     * @param escaped_value
     */
    void decode_log_type(std::string& escaped_value);

    /**
     * Decodes the log type
     */
    void decode_log_type();

    /**
     * Checks if the entry has been initialized
     * @return true if the entry has been initialized, false otherwise
     */
    bool initialized() const { return m_init; }

private:
    // Methods
    /**
     * Escapes any variable delimiters that don't correspond to the positions of variables in the
     * logtype entry's value
     * @param escaped_logtype_value
     */
    void get_value_with_unfounded_variables_escaped(std::string& escaped_logtype_value) const;

    /**
     * Escapes any variable delimiters in the identified portion of the given value
     * @param value
     * @param begin_ix
     * @param end_ix
     * @param escaped_value
     */
    static void escape_variable_delimiters(
            std::string const& value,
            size_t begin_ix,
            size_t end_ix,
            std::string& escaped_value
    );

    // Variables
    std::vector<size_t> m_var_positions;
    bool m_init;
};

class VariableDictionaryEntry : public DictionaryEntry<uint64_t> {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}
    };

    // Constructors
    VariableDictionaryEntry() = default;

    VariableDictionaryEntry(std::string value, uint64_t id)
            : DictionaryEntry<uint64_t>(std::move(value), id) {}

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

    /**
     * Clears the entry
     */
    void clear() { m_value.clear(); }

    /**
     * Writes an entry to a compressed file
     * @param compressor
     */
    void write_to_file(ZstdCompressor& compressor) const;

    /**
     * Tries to read an entry from the given decompressor
     * @param decompressor
     * @return Same as streaming_compression::Decompressor::try_read_numeric_value
     * @return Same as streaming_compression::Decompressor::try_read_string
     */
    ErrorCode try_read_from_file(ZstdDecompressor& decompressor, uint64_t id);

    /**
     * Reads an entry from the given decompressor
     * @param decompressor
     */
    void read_from_file(ZstdDecompressor& decompressor, uint64_t id, bool lazy);
};
}  // namespace clp_s

#endif  // CLP_S_DICTIONARYENTRY_HPP
