#ifndef CLP_VARIABLEDICTIONARYREADERREQ_HPP
#define CLP_VARIABLEDICTIONARYREADERREQ_HPP

#include <concepts>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "Defs.h"
#include "VariableDictionaryEntryReq.hpp"

namespace clp {
/**
 * Requirement for the variable dictionary reader interface.
 * @tparam VariableDictionaryReaderType The type of the variable dictionary reader.
 */
template <typename VariableDictionaryReaderType>
concept VariableDictionaryReaderReq = requires(
        VariableDictionaryReaderType reader,
        variable_dictionary_id_t id,
        std::string_view variable,
        bool ignore_case,
        std::unordered_set<typename VariableDictionaryReaderType::Entry const*>& entries
) {
    requires VariableDictionaryEntryReq<typename VariableDictionaryReaderType::Entry>;

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
     * @param ignore_case Whether the search should be case insensitive.
     * @return A vector of entries matching the given variable value.
     */
    {
        reader.get_entry_matching_value(variable, ignore_case)
    } -> std::same_as<std::vector<typename VariableDictionaryReaderType::Entry const*>>;

    /**
     * Gets entries matching a given wildcard string.
     * @param variable A wildcard search string.
     * @param ignore_case Whether the search should be case insensitive.
     * @param entries Returns all the matching entries.
     */
    {
        reader.get_entries_matching_wildcard_string(variable, ignore_case, entries)
    } -> std::same_as<void>;

    requires std::same_as<
            typename VariableDictionaryReaderType::dictionary_id_t,
            variable_dictionary_id_t>;
};
}  // namespace clp

#endif  // CLP_VARIABLEDICTIONARYREADERREQ_HPP
