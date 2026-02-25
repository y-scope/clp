// Code from CLP

#ifndef CLP_S_DICTIONARYENTRY_HPP
#define CLP_S_DICTIONARYENTRY_HPP

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "../clp/Defs.h"
#include "../clp/ir/types.hpp"
#include "ErrorCode.hpp"
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
    std::string m_value;
    DictionaryIdType m_id;
};

/**
 * Class representing a logtype dictionary entry
 */
class LogTypeDictionaryEntry : public DictionaryEntry<clp::logtype_dictionary_id_t> {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}
    };

    // Constructors
    LogTypeDictionaryEntry() : m_init(false) {}

    // Use default copy constructor
    LogTypeDictionaryEntry(LogTypeDictionaryEntry const&) = default;

    // Use default assignment operators
    LogTypeDictionaryEntry& operator=(LogTypeDictionaryEntry const&) = default;

    // Methods
    /**
     * @return The number of variables placeholders (including escaped ones) in the logtype.
     */
    size_t get_num_placeholders() const { return m_placeholder_positions.size(); }

    /**
     * @return The number of variable placeholders (excluding escaped ones) in the logtype.
     */
    size_t get_num_variables() const {
        return m_placeholder_positions.size() - m_num_escaped_placeholders;
    }

    /**
     * Gets all info about a variable placeholder in the logtype
     * @param placeholder_ix The index of the placeholder to get the info for
     * @param placeholder
     * @return The placeholder's position in the logtype, or SIZE_MAX if var_ix is out of bounds
     */
    size_t
    get_placeholder_info(size_t placeholder_ix, clp::ir::VariablePlaceholder& placeholder) const;

    /**
     * Adds a constant to the logtype
     * @param value_containing_constant
     * @param begin_pos Start of the constant in value_containing_constant
     * @param length
     */
    void add_constant(std::string_view value_containing_constant, size_t begin_pos, size_t length);

    /**
     * Adds an int variable placeholder
     */
    void add_int_var();
    /**
     * Adds a float variable placeholder
     */
    void add_float_var();
    /**
     * Adds a dictionary variable placeholder
     */
    void add_dictionary_var();
    /**
     * Adds an escape character
     */
    void add_escape();

    /**
     * Adds static text to the logtype, escaping any placeholder characters.
     */
    auto add_static_text(std::string_view static_text) -> void;

    /**
     * Gets the size (in-memory) of the data contained in this entry
     * @return Size of the data contained in this entry
     */
    size_t get_data_size() const;

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
    auto parse_next_var(
            std::string_view msg,
            size_t& var_begin_pos,
            size_t& var_end_pos,
            std::string_view& var
    ) -> bool;

    /**
     * Reserves space for a constant of the given length
     * @param length
     */
    void reserve_constant_length(size_t length) { m_value.reserve(length); }

    void set_id(clp::logtype_dictionary_id_t id) { m_id = id; }

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
    ErrorCode
    try_read_from_file(ZstdDecompressor& decompressor, clp::logtype_dictionary_id_t id, bool lazy);

    /**
     * Reads an entry from the given decompressor
     * @param decompressor
     * @param lazy apply lazy decoding
     */
    void read_from_file(ZstdDecompressor& decompressor, clp::logtype_dictionary_id_t id, bool lazy);

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
     * Decodes the log type
     * @param escaped_value
     */
    void decode_log_type(std::string& escaped_value);

    // Variables
    std::vector<size_t> m_placeholder_positions;
    size_t m_num_escaped_placeholders{};
    bool m_init{false};
};

class VariableDictionaryEntry : public DictionaryEntry<clp::variable_dictionary_id_t> {
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

    VariableDictionaryEntry(std::string value, clp::variable_dictionary_id_t id)
            : DictionaryEntry<clp::variable_dictionary_id_t>(std::move(value), id) {}

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
     * @param id
     * @return Same as streaming_compression::Decompressor::try_read_numeric_value
     * @return Same as streaming_compression::Decompressor::try_read_string
     */
    ErrorCode try_read_from_file(ZstdDecompressor& decompressor, clp::variable_dictionary_id_t id);

    /**
     * Reads an entry from the given decompressor
     * @param decompressor
     * @param id
     * @param lazy
     */
    void
    read_from_file(ZstdDecompressor& decompressor, clp::variable_dictionary_id_t id, bool lazy);
};
}  // namespace clp_s

#endif  // CLP_S_DICTIONARYENTRY_HPP
