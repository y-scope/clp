#ifndef ENCODEDVARIABLEINTERPRETER_HPP
#define ENCODEDVARIABLEINTERPRETER_HPP

// C++ standard libraries
#include <string>
#include <vector>

// Project headers
#include "Query.hpp"
#include "TraceableException.hpp"
#include "VariableDictionaryReader.hpp"
#include "VariableDictionaryWriter.hpp"

/**
 * Class to parse and encode strings into encoded variables and to interpret encoded variables back into strings. An encoded variable is one of:
 * i)   a variable dictionary ID, referring to an entry in the variable dictionary, or
 * ii)  a value, representing an integer variable exactly as it appears in the original log message, or
 * iii) a value, representing a base-10, 16-digit number with a decimal point, where at least one digit is after the decimal point, encoded with a custom
 *      format.
 *
 * To decode an encoded variable, the logtype specifies whether the variable is either:
 * - i/ii, or
 * - iii
 * This class differentiates between i & ii by using a certain range of values for variable dictionary IDs, and the rest for non-dictionary variables.
 *
 * We collectively refer to ii & iii as non-dictionary variables.
 */
class EncodedVariableInterpreter {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed (ErrorCode error_code, const char* const filename, int line_number) : TraceableException (error_code, filename, line_number) {}

        // Methods
        const char* what () const noexcept override {
            return "EncodedVariableInterpreter operation failed";
        }
    };

    // Methods
    static encoded_variable_t encode_var_dict_id (variable_dictionary_id_t id);
    static variable_dictionary_id_t decode_var_dict_id (encoded_variable_t encoded_var);
    /**
     * Converts the given string into a representable integer variable if possible
     * @param value
     * @param encoded_var
     * @return true if was successfully converted, false otherwise
     */
    static bool convert_string_to_representable_integer_var (const std::string& value, encoded_variable_t& encoded_var);
    /**
     * Converts the given string into a representable float variable if possible
     * @param value
     * @param encoded_var
     * @return true if was successfully converted, false otherwise
     */
    static bool convert_string_to_representable_float_var (const std::string& value, encoded_variable_t& encoded_var);
    /**
     * Converts the given encoded float into a string
     * @param encoded_var
     * @param value
     */
    static void convert_encoded_float_to_string (encoded_variable_t encoded_var, std::string& value);

    /**
     * Parses all variables from a message (while constructing the logtype) and encodes them (adding them to the variable dictionary if necessary)
     * @param message
     * @param logtype_dict_entry
     * @param var_dict
     * @param encoded_vars
     * @param var_ids
     */
    static void encode_and_add_to_dictionary (const std::string& message, LogTypeDictionaryEntry& logtype_dict_entry, VariableDictionaryWriter& var_dict,
                                              std::vector<encoded_variable_t>& encoded_vars, std::vector<variable_dictionary_id_t>& var_ids);
    /**
     * Decodes all variables and decompresses them into a message
     * @param logtype_dict_entry
     * @param var_dict
     * @param encoded_vars
     * @param decompressed_msg
     * @return true if successful, false otherwise
     */
    static bool decode_variables_into_message (const LogTypeDictionaryEntry& logtype_dict_entry, const VariableDictionaryReader& var_dict,
                                               const std::vector<encoded_variable_t>& encoded_vars, std::string& decompressed_msg);

    /**
     * Encodes a string-form variable, and if it is dictionary variable, searches for its ID in the given variable dictionary
     * @param var_str
     * @param var_dict
     * @param ignore_case
     * @param logtype
     * @param sub_query
     * @return true if variable is a non-dictionary variable or was found in the given variable dictionary
     * @return false otherwise
     */
    static bool encode_and_search_dictionary (const std::string& var_str, const VariableDictionaryReader& var_dict, bool ignore_case, std::string& logtype,
                                              SubQuery& sub_query);
    /**
     * Search for the given string-form variable in the variable dictionary, encode any matches, and add them to the given sub-query
     * @param var_wildcard_str
     * @param var_dict
     * @param ignore_case
     * @param sub_query
     * @return true if any match found, false otherwise
     */
    static bool wildcard_search_dictionary_and_get_encoded_matches (const std::string& var_wildcard_str, const VariableDictionaryReader& var_dict,
                                                                    bool ignore_case, SubQuery& sub_query);
};

#endif // ENCODEDVARIABLEINTERPRETER_HPP
