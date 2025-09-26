#ifndef CLP_VARIABLEDICTIONARYWRITERREQ_HPP
#define CLP_VARIABLEDICTIONARYWRITERREQ_HPP
#include <concepts>
#include <string_view>

#include "Defs.h"

namespace clp {
/**
 * Requirement for the variable dictionary writer interface.
 * @tparam VariableDictionaryWriterType The type of the variable dictionary writer.
 */
template <typename VariableDictionaryWriterType>
concept VariableDictionaryWriterReq = requires(
        VariableDictionaryWriterType writer,
        std::string_view value,
        variable_dictionary_id_t& id_ref
) {
    /**
     * Adds the given variable to the dictionary if it doesn't exist.
     * @param value
     * @param id_ref Returns the entry ID of the given variable.
     * @return Whether this call resulted in inserting a new entry.
     */
    {
        writer.add_entry(value, id_ref)
    } -> std::same_as<bool>;
};
}  // namespace clp

#endif  // CLP_VARIABLEDICTIONARYWRITERREQ_HPP
