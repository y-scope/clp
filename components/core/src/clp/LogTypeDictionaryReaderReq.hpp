#ifndef CLP_LOGTYPEDICTIONARYREADERREQ_HPP
#define CLP_LOGTYPEDICTIONARYREADERREQ_HPP

#include <concepts>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "Defs.h"
#include "LogTypeDictionaryEntryReq.hpp"

namespace clp {
/**
 * Requirement for the logtype dictionary reader interface.
 * @tparam LogTypeDictionaryReaderType The type of the logtype dictionary reader.
 */
template <typename LogTypeDictionaryReaderType>
concept LogTypeDictionaryReaderReq = requires(
        LogTypeDictionaryReaderType reader,
        std::string_view logtype,
        bool ignore_case,
        std::unordered_set<typename LogTypeDictionaryReaderType::Entry const*>& entries
) {
    requires LogTypeDictionaryEntryReq<typename LogTypeDictionaryReaderType::Entry>;

    /**
     * Gets entries matching a given logtype.
     * @param logtype
     * @param ignore_case Whether the search should be case insensitive.
     * @return A vector of entries matching the given logtype.
     */
    {
        reader.get_entry_matching_value(logtype, ignore_case)
    } -> std::same_as<std::vector<typename LogTypeDictionaryReaderType::Entry const*>>;

    /**
     * Gets entries matching a wildcard string.
     * @param logtype A wildcard search string.
     * @param ignore_case Whether the search should be case insensitive.
     * @param entries Returns all the matching entries.
     */
    {
        reader.get_entries_matching_wildcard_string(logtype, ignore_case, entries)
    } -> std::same_as<void>;

    requires std::
            same_as<typename LogTypeDictionaryReaderType::dictionary_id_t, logtype_dictionary_id_t>;
};
}  // namespace clp

#endif  // CLP_LOGTYPEDICTIONARYREADERREQ_HPP
