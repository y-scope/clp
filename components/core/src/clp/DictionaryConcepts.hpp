#ifndef CLP_DICTIONARYCONCEPTS_HPP
#define CLP_DICTIONARYCONCEPTS_HPP

#include <concepts>
#include <cstddef>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "Defs.h"
#include "ir/types.hpp"

namespace clp {
/**
 * Requirement for the logtype dictionary entry interface.
 * @tparam LogTypeDictionaryEntryType The type of the logtype dictionary entry.
 */
template <typename LogTypeDictionaryEntryType>
concept LogTypeDictionaryEntryReq = requires(
        LogTypeDictionaryEntryType entry,
        size_t length,
        std::string_view msg,
        size_t& begin_pos_ref,
        size_t& end_pos_ref,
        std::string_view& parsed_var_ref,
        size_t begin_pos,
        size_t placeholder_ix,
        ir::VariablePlaceholder& placeholder_ref,
        std::string& logtype
) {
    /**
     * Clears all internal state.
     */
    {
        entry.clear()
    } -> std::same_as<void>;

    /**
     * Reserves space for a logtype string of a given length.
     * @param length
     */
    {
        entry.reserve_constant_length(length)
    } -> std::same_as<void>;

    /**
     * Parses the next variable from a message according to `ir::get_bounds_of_next_var`,
     * constructing the constant part of the message's logtype in the processed range at the same
     * time.
     * @param msg The original log message.
     * @param begin_pos_ref The beginning position of the last variable. This parameter is updated
     * to the beginning position of the next variable.
     * @param end_pos_ref The ending position of the last variable (exclusive). This parameter is
     * updated to the ending position of the next variable (exclusive).
     * @return Whether another variable was found or not.
     */
    {
        entry.parse_next_var(msg, begin_pos_ref, end_pos_ref, parsed_var_ref)
    } -> std::same_as<bool>;

    /**
     * Adds a substring of `msg` to the constant part of the logtype.
     * @param msg The value containing a constant to add to the logtype.
     * @param begin_pos The starting offset into `msg` to add to the logtype.
     * @param length The length of the substring to add to the logtype.
     */
    {
        entry.add_constant(msg, begin_pos, length)
    } -> std::same_as<void>;

    /**
     * Adds an integer variable placeholder to the constant part of the logtype.
     */
    {
        entry.add_int_var()
    } -> std::same_as<void>;

    /**
     * Adds a float variable placeholder to the constant part of the logtype.
     */
    {
        entry.add_float_var()
    } -> std::same_as<void>;

    /**
     * Adds a dictionary variable placeholder to the constant part of the logtype.
     */
    {
        entry.add_dictionary_var()
    } -> std::same_as<void>;

    /**
     * @return The constant part of the logtype.
     */
    {
        entry.get_value()
    } -> std::same_as<std::string const&>;

    /**
     * @return The number of variables in the constant part of the logtype.
     */
    {
        entry.get_num_variables()
    } -> std::same_as<size_t>;

    /**
     * @return The number of variable placeholders (including escaped ones) in the constant part of
     * the logtype.
     */
    {
        entry.get_num_placeholders()
    } -> std::same_as<size_t>;

    /**
     * Gets the position and type of a variable placeholder in the logtype.
     * @param placeholder_ix The index of the placeholder to get the info for.
     * @param placeholder_ref The type of the placeholder at `placeholder_ix`.
     * @return The placeholder's position in the logtype, or `SIZE_MAX` if `placeholder_ix` is out
     * of bounds.
     */
    {
        entry.get_placeholder_info(placeholder_ix, placeholder_ref)
    } -> std::same_as<size_t>;

    /**
     * @return The dictionary Id for this logtype.
     */
    {
        entry.get_id()
    } -> std::same_as<logtype_dictionary_id_t>;
};

/**
 * Requirement for the variable dictionary entry interface.
 * @tparam VariableDictionaryEntryType The type of the variable dictionary entry.
 */
template <typename VariableDictionaryEntryType>
concept VariableDictionaryEntryReq = requires(VariableDictionaryEntryType entry) {
    /**
     * @return The dictionary Id for this variable.
     */
    {
        entry.get_id()
    } -> std::same_as<variable_dictionary_id_t>;
};

/**
 * Requirement for the logtype dictionary reader interface.
 * @tparam LogTypeDictionaryReaderType The type of the logtype dictionary reader.
 * @tparam LogTypeDictionaryEntryType The type of the entries in the logtype dictionary reader.
 */
template <
        typename LogTypeDictionaryReaderType,
        typename LogTypeDictionaryEntryType = LogTypeDictionaryReaderType::entry_t>
concept LogTypeDictionaryReaderReq = requires(
        LogTypeDictionaryReaderType reader,
        std::string_view logtype,
        bool ignore_case,
        std::unordered_set<LogTypeDictionaryEntryType const*>& entries
) {
    /**
     * Gets entries matching a given logtype.
     * @param logtype
     * @param ignore_case Whether the lookup should be case insensitive.
     * @return A vector of entries matching the given logtype.
     */
    {
        reader.get_entry_matching_value(logtype, ignore_case)
    } -> std::same_as<std::vector<LogTypeDictionaryEntryType const*>>;

    /**
     * Gets entries matching a wildcard string.
     * @param logtype A wildcard search string.
     * @param ignore_case Whether the search should be case insensitive.
     * @param entries A hash set in which to store the found entries.
     */
    {
        reader.get_entries_matching_wildcard_string(logtype, ignore_case, entries)
    } -> std::same_as<void>;

    std::same_as<typename LogTypeDictionaryReaderType::dictionary_id_t, logtype_dictionary_id_t>;

    std::same_as<typename LogTypeDictionaryReaderType::entry_t, LogTypeDictionaryEntryType>;
};

/**
 * Requirement for the variable dictionary writer interface.
 * @tparam VariableDictionaryWriterType The type of the variable dictionary writer.
 */
template <typename VariableDictionaryWriterType>
concept VariableDictionaryWriterReq = requires(
        VariableDictionaryWriterType writer,
        std::string_view value,
        variable_dictionary_id_t id
) {
    /**
     * Adds the given variable to the dictionary if it doesn't exist.
     * @param value
     * @param id The Id of the variable matching the given entry.
     */
    {
        writer.add_entry(value, id)
    } -> std::same_as<bool>;
};

/**
 * Requirement for the variable dictionary reader interface.
 * @tparam VariableDictionaryReaderType The type of the variable dictionary reader.
 * @tparam VariableDictionaryEntryType The type of the entries in the variable dictionary reader.
 */
template <
        typename VariableDictionaryReaderType,
        typename VariableDictionaryEntryType = VariableDictionaryReaderType::entry_t>
concept VariableDictionaryReaderReq = requires(
        VariableDictionaryReaderType reader,
        variable_dictionary_id_t id,
        std::string_view variable,
        bool ignore_case,
        std::unordered_set<VariableDictionaryEntryType const*>& entries
) {
    /**
     * @param id
     * @return The value of the dictionary entry with the given Id.
     */
    {
        reader.get_value(id)
    } -> std::same_as<std::string const&>;

    /**
     * Gets entries matching a given variable value.
     * @param variable The variable value to look up.
     * @param ignore_case Whether the lookup should be case insensitive.
     * @return A vector of entries matching the given variable value.
     */
    {
        reader.get_entry_matching_value(variable, ignore_case)
    } -> std::same_as<std::vector<VariableDictionaryEntryType const*>>;

    /**
     * Gets entries matching a given wildcard string.
     * @param variable A wildcard search string.
     * @param ignore_case Whether the search should be case insensitive.
     * @param entries A hash set in which to store the found entries.
     */
    {
        reader.get_entries_matching_wildcard_string(variable, ignore_case, entries)
    } -> std::same_as<void>;

    std::same_as<typename VariableDictionaryReaderType::dictionary_id_t, variable_dictionary_id_t>;

    std::same_as<typename VariableDictionaryReaderType::entry_t, VariableDictionaryEntryType>;
};
}  // namespace clp

#endif  // CLP_DICTIONARYCONCEPTS_HPP
