// Code from CLP

#ifndef CLP_S_SEARCH_CLP_SEARCH_ENCODEDVARIABLEINTERPRETER_HPP
#define CLP_S_SEARCH_CLP_SEARCH_ENCODEDVARIABLEINTERPRETER_HPP

#include <string>
#include <vector>

#include "../../DictionaryReader.hpp"
#include "../../DictionaryWriter.hpp"
#include "../../TraceableException.hpp"
#include "Query.hpp"

namespace clp_s::search::clp_search {
/**
 * Class to parse and encode strings into encoded variables and to interpret encoded variables
 * back into strings. An encoded variable is one of: i)   a variable dictionary ID, referring to
 * an entry in the variable dictionary, or ii)  a value, representing an integer variable
 * exactly as it appears in the original log message, or iii) a value, representing a base-10,
 * 16-digit number with a decimal point, where at least one digit is after the decimal point,
 * encoded with a custom format.
 *
 * To decode an encoded variable, the logtype specifies whether the variable is either:
 * - i/ii, or
 * - iii
 * This class differentiates between i & ii by using a certain range of values for variable
 * dictionary IDs, and the rest for non-dictionary variables.
 *
 * We collectively refer to ii & iii as non-dictionary variables.
 */
class EncodedVariableInterpreter {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}
    };

    /**
     * Encodes a string-form variable, and if it is dictionary variable, searches for its ID in
     * the given variable dictionary
     * @param var_str
     * @param var_dict
     * @param ignore_case
     * @param logtype
     * @param sub_query
     * @return true if variable is a non-dictionary variable or was found in the given variable
     * dictionary, false otherwise
     */
    static bool encode_and_search_dictionary(
            std::string const& var_str,
            VariableDictionaryReader const& var_dict,
            bool ignore_case,
            std::string& logtype,
            SubQuery& sub_query
    );
    /**
     * Search for the given string-form variable in the variable dictionary, encode any matches,
     * and add them to the given sub-query
     * @param var_wildcard_str
     * @param var_dict
     * @param ignore_case
     * @param sub_query
     * @return true if any match found, false otherwise
     */
    static bool wildcard_search_dictionary_and_get_encoded_matches(
            std::string const& var_wildcard_str,
            VariableDictionaryReader const& var_dict,
            bool ignore_case,
            SubQuery& sub_query
    );

private:
    // Variables
    // The beginning of the range used for encoding variable dictionary IDs
    static constexpr encoded_variable_t cVarDictIdRangeBegin = 1LL << 62;
    // The end (exclusive) of the range used for encoding variable dictionary IDs
    static constexpr encoded_variable_t cVarDictIdRangeEnd = (1ULL << 63) - 1;
};
}  // namespace clp_s::search::clp_search

#endif  // CLP_S_SEARCH_CLP_SEARCH_ENCODEDVARIABLEINTERPRETER_HPP
