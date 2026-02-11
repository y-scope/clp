// Code from CLP

#ifndef CLP_S_DICTIONARYENTRY_HPP
#define CLP_S_DICTIONARYENTRY_HPP

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <clp/Defs.h>
#include <clp/ir/types.hpp>
#include <clp_s/ErrorCode.hpp>
#include <clp_s/TraceableException.hpp>
#include <clp_s/ZstdCompressor.hpp>
#include <clp_s/ZstdDecompressor.hpp>

namespace clp_s {
/**
 * Template class representing a dictionary entry
 * @tparam DictionaryIdType
 */
template <typename DictionaryIdType>
class DictionaryEntry {
public:
    DictionaryEntry() = default;

    DictionaryEntry(std::string value, DictionaryIdType id) : m_value(std::move(value)), m_id(id) {}

    [[nodiscard]] auto get_id() const -> DictionaryIdType { return m_id; }

    [[nodiscard]] auto get_value() const -> std::string const& { return m_value; }

protected:
    std::string m_value;
    DictionaryIdType m_id;
};

/**
 * Class representing a logtype dictionary entry
 */
class LogTypeDictionaryEntry : public DictionaryEntry<clp::logtype_dictionary_id_t> {
public:
    class OperationFailed : public TraceableException {
    public:
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}
    };

    /**
     * @return The number of variables placeholders (including escaped ones) in the logtype.
     */
    [[nodiscard]] auto get_num_placeholders() const -> size_t {
        return m_placeholder_positions.size();
    }

    /**
     * @return The number of variable placeholders (excluding escaped ones) in the logtype.
     */
    [[nodiscard]] auto get_num_variables() const -> size_t {
        return m_placeholder_positions.size() - m_num_escaped_placeholders;
    }

    /**
     * Gets all info about a variable placeholder in the logtype
     * @param placeholder_ix The index of the placeholder to get the info for
     * @param placeholder
     * @return The placeholder's position in the logtype, or SIZE_MAX if var_ix is out of bounds
     */
    auto
    get_placeholder_info(size_t placeholder_ix, clp::ir::VariablePlaceholder& placeholder) const
            -> size_t;

    /**
     * Adds a constant to the logtype
     * @param value_containing_constant
     * @param begin_pos Start of the constant in value_containing_constant
     * @param length
     */
    auto add_constant(std::string_view value_containing_constant, size_t begin_pos, size_t length)
            -> void;

    /**
     * Adds an int variable placeholder
     */
    auto add_int_var() -> void;

    /**
     * Adds a float variable placeholder
     */
    auto add_float_var() -> void;

    /**
     * Adds a dictionary variable placeholder
     */
    auto add_dictionary_var() -> void;

    /**
     * Adds an escape character
     */
    auto add_escape() -> void;

    /**
     * Adds static text to the logtype, escaping any placeholder characters.
     */
    auto add_static_text(std::string_view static_text) -> void;

    /**
     * Gets the size (in-memory) of the data contained in this entry
     * @return Size of the data contained in this entry
     */
    [[nodiscard]] auto get_data_size() const -> size_t;

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
    auto reserve_constant_length(size_t length) -> void { m_value.reserve(length); }

    auto set_id(clp::logtype_dictionary_id_t id) -> void { m_id = id; }

    /**
     * Clears the entry
     */
    auto clear() -> void;

    /**
     * Writes an entry to a compressed file
     * @param compressor
     */
    auto write_to_file(ZstdCompressor& compressor) const -> void;

    /**
     * Tries to read an entry from the given decompressor
     * @param decompressor
     * @return Same as streaming_compression::Decompressor::try_read_numeric_value
     * @return Same as streaming_compression::Decompressor::try_read_string
     */
    auto
    try_read_from_file(ZstdDecompressor& decompressor, clp::logtype_dictionary_id_t id, bool lazy)
            -> ErrorCode;

    /**
     * Reads an entry from the given decompressor
     * @param decompressor
     * @param lazy apply lazy decoding
     */
    auto read_from_file(ZstdDecompressor& decompressor, clp::logtype_dictionary_id_t id, bool lazy)
            -> void;

    /**
     * Decodes the log type
     */
    auto decode_log_type() -> void;

    /**
     * Checks if the entry has been initialized
     * @return true if the entry has been initialized, false otherwise
     */
    [[nodiscard]] auto initialized() const -> bool { return m_init; }

private:
    /**
     * Decodes the log type
     * @param escaped_value
     */
    auto decode_log_type(std::string& escaped_value) -> void;

    std::vector<size_t> m_placeholder_positions;
    size_t m_num_escaped_placeholders{};
    bool m_init{false};
};

class VariableDictionaryEntry : public DictionaryEntry<clp::variable_dictionary_id_t> {
public:
    class OperationFailed : public TraceableException {
    public:
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}
    };

    VariableDictionaryEntry() = default;

    VariableDictionaryEntry(std::string value, clp::variable_dictionary_id_t id)
            : DictionaryEntry<clp::variable_dictionary_id_t>(std::move(value), id) {}

    /**
     * Gets the size (in-memory) of the data contained in this entry
     * @return Size of the data contained in this entry
     */
    [[nodiscard]] auto get_data_size() const -> size_t;

    /**
     * Clears the entry
     */
    auto clear() -> void { m_value.clear(); }

    /**
     * Writes an entry to a compressed file
     * @param compressor
     */
    auto write_to_file(ZstdCompressor& compressor) const -> void;

    /**
     * Tries to read an entry from the given decompressor
     * @param decompressor
     * @param id
     * @return Same as streaming_compression::Decompressor::try_read_numeric_value
     * @return Same as streaming_compression::Decompressor::try_read_string
     */
    auto try_read_from_file(ZstdDecompressor& decompressor, clp::variable_dictionary_id_t id)
            -> ErrorCode;

    /**
     * Reads an entry from the given decompressor
     * @param decompressor
     * @param id
     * @param lazy
     */
    auto read_from_file(ZstdDecompressor& decompressor, clp::variable_dictionary_id_t id, bool lazy)
            -> void;
};
}  // namespace clp_s

#endif  // CLP_S_DICTIONARYENTRY_HPP
