#ifndef CLP_LOGTYPEDICTIONARYENTRYREQ_HPP
#define CLP_LOGTYPEDICTIONARYENTRYREQ_HPP

#include <concepts>
#include <cstddef>
#include <string>
#include <string_view>

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
        size_t placeholder_idx,
        ir::VariablePlaceholder& placeholder_ref,
        std::string_view static_text
) {
    /**
     * Clears all internal state.
     */
    { entry.clear() } -> std::same_as<void>;

    /**
     * Reserves space for a logtype string of a given length.
     * @param length
     */
    { entry.reserve_constant_length(length) } -> std::same_as<void>;

    /**
     * Parses the next variable from a message according to `ir::get_bounds_of_next_var`,
     * constructing the constant part of the message's logtype in the processed range at the same
     * time.
     * @param msg The original log message.
     * @param begin_pos_ref The beginning position of the last variable. Returns the beginning
     * position of the next variable.
     * @param end_pos_ref The ending position of the last variable (exclusive). Returns the ending
     * position of the next variable (exclusive).
     * @param parsed_var_ref Returns a view to the parsed variable, if one was found.
     * @return Whether a new variable was parsed.
     */
    { entry.parse_next_var(msg, begin_pos_ref, end_pos_ref, parsed_var_ref) } -> std::same_as<bool>;

    /**
     * Adds a substring of `msg` to the constant part of the logtype.
     * @param msg The value containing a constant to add to the logtype.
     * @param begin_pos The starting offset into `msg` to add to the logtype.
     * @param length The length of the substring to add to the logtype.
     */
    { entry.add_constant(msg, begin_pos, length) } -> std::same_as<void>;

    /**
     * Adds an integer variable placeholder to the constant part of the logtype.
     */
    { entry.add_int_var() } -> std::same_as<void>;

    /**
     * Adds a float variable placeholder to the constant part of the logtype.
     */
    { entry.add_float_var() } -> std::same_as<void>;

    /**
     * Adds a dictionary variable placeholder to the constant part of the logtype.
     */
    { entry.add_dictionary_var() } -> std::same_as<void>;

    /**
     * Adds an escape character
     */
    { entry.add_escape() } -> std::same_as<void>;

    /**
     * Adds static text to the logtype escaping any placeholder characters.
     */
    { entry.add_static_text(static_text) } -> std::same_as<void>;

    /**
     * @return The constant part of the logtype.
     */
    { entry.get_value() } -> std::same_as<std::string const&>;

    /**
     * @return The number of variables in the constant part of the logtype.
     */
    { entry.get_num_variables() } -> std::same_as<size_t>;

    /**
     * @return The number of variable placeholders (including escaped ones) in the constant part of
     * the logtype.
     */
    { entry.get_num_placeholders() } -> std::same_as<size_t>;

    /**
     * Gets the position and type of a variable placeholder in the logtype.
     * @param placeholder_idx The index of the placeholder to get the info for.
     * @param placeholder_ref Returns the type of the placeholder at `placeholder_idx`.
     * @return The placeholder's position in the logtype, or `SIZE_MAX` if `placeholder_idx` is out
     * of bounds.
     */
    { entry.get_placeholder_info(placeholder_idx, placeholder_ref) } -> std::same_as<size_t>;

    /**
     * @return The dictionary ID for this logtype.
     */
    { entry.get_id() } -> std::same_as<logtype_dictionary_id_t>;
};
}  // namespace clp

#endif  // CLP_LOGTYPEDICTIONARYENTRYREQ_HPP
