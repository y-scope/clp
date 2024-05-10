// Code from CLP

#ifndef CLP_S_VARIABLEDECODER_HPP
#define CLP_S_VARIABLEDECODER_HPP

#include "DictionaryEntry.hpp"
#include "DictionaryReader.hpp"
#include "Utils.hpp"

namespace clp_s {
class VariableDecoder {
public:
    /**
     * Decode variables into a message
     * @param logtype_dict_entry
     * @param var_dict
     * @param encoded_var
     * @param value
     */
    static bool decode_variables_into_message(
            LogTypeDictionaryEntry const& logtype_dict_entry,
            VariableDictionaryReader const& var_dict,
            UnalignedMemSpan<int64_t> encoded_vars,
            std::string& decompressed_msg
    );

private:
    /**
     * Convert an encoded double into a string
     * @param logtype_dict_entry
     * @param var_dict
     * @param encoded_var
     * @param value
     */
    static void convert_encoded_double_to_string(int64_t encoded_var, std::string& value);

    /**
     * Checks if the given encoded variable is a variable dictionary id
     * @param encoded_var
     * @return true if encoded_var is a variable dictionary id, false otherwise
     */
    static bool is_var_dict_id(int64_t encoded_var) {
        return (cVarDictIdRangeBegin <= encoded_var && encoded_var < cVarDictIdRangeEnd);
    }

    /**
     * Decodes the given variable dictionary id
     * @param encoded_var
     * @return the decoded id
     */
    static uint64_t decode_var_dict_id(int64_t encoded_var) {
        uint64_t id = encoded_var - cVarDictIdRangeBegin;
        return id;
    }

    static constexpr int64_t cVarDictIdRangeBegin = 1LL << 62;
    static constexpr int64_t cVarDictIdRangeEnd = (1ULL << 63) - 1;
};
}  // namespace clp_s

#endif  // CLP_S_VARIABLEDECODER_HPP
