// Code from CLP

#ifndef CLP_S_VARIABLEENCODER_HPP
#define CLP_S_VARIABLEENCODER_HPP

#include <string>

#include <simdjson.h>

#include "DictionaryEntry.hpp"
#include "DictionaryWriter.hpp"

using namespace simdjson;

namespace clp_s {
class VariableEncoder {
public:
    /**
     * Encodes the given message and adds the encoded variables to the given vector
     * @param message
     * @param logtype_dict_entry
     * @param var_dict
     * @param encoded_vars
     */
    static void encode_and_add_to_dictionary(
            std::string const& message,
            LogTypeDictionaryEntry& logtype_dict_entry,
            VariableDictionaryWriter& var_dict,
            std::vector<int64_t>& encoded_vars
    );

    /**
     * Converts the given string to an int64_t
     * @param raw
     * @param converted
     * @return true if the conversion was successful, false otherwise
     */
    static bool convert_string_to_int64(std::string const& raw, int64_t& converted);

    /**
     * Converts the given string to a representable int64_t
     * @param value
     * @param encoded_var
     * @return true if the conversion was successful, false otherwise
     */
    static bool
    convert_string_to_representable_integer_var(std::string const& value, int64_t& encoded_var);

    /**
     * Converts the given string to a representable encoded double
     * @param value
     * @param encoded_var
     * @return true if the conversion was successful, false otherwise
     */
    static bool
    convert_string_to_representable_double_var(std::string const& value, int64_t& encoded_var);

    /**
     * Encodes the given dictionary id as a variable dictionary id
     * @param id
     * @return the encoded id
     */
    static int64_t encode_var_dict_id(uint64_t id) { return (int64_t)id + cVarDictIdRangeBegin; }

private:
    static constexpr int64_t cVarDictIdRangeBegin = 1LL << 62;
    static constexpr int64_t cVarDictIdRangeEnd = (1ULL << 63) - 1;
};
}  // namespace clp_s

#endif  // CLP_S_VARIABLEENCODER_HPP
