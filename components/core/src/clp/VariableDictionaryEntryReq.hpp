#ifndef CLP_VARIABLEDICTIONARYENTRYREQ_HPP
#define CLP_VARIABLEDICTIONARYENTRYREQ_HPP

#include <concepts>

#include "Defs.h"

namespace clp {
/**
 * Requirement for the variable dictionary entry interface.
 * @tparam VariableDictionaryEntryType The type of the variable dictionary entry.
 */
template <typename VariableDictionaryEntryType>
concept VariableDictionaryEntryReq = requires(VariableDictionaryEntryType entry) {
    /**
     * @return The dictionary ID for this variable.
     */
    {
        entry.get_id()
    } -> std::same_as<variable_dictionary_id_t>;
};
}  // namespace clp

#endif  // CLP_VARIABLEDICTIONARYENTRYREQ_HPP
