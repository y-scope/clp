#ifndef CLP_ENCODEDVARIABLEINTERPRETER_HPP
#define CLP_ENCODEDVARIABLEINTERPRETER_HPP

#include <string>
#include <vector>

#include "ir/LogEvent.hpp"
#include "ir/types.hpp"
#include "Query.hpp"
#include "TraceableException.hpp"
#include "VariableDictionaryReader.hpp"
#include "VariableDictionaryWriter.hpp"

namespace clp {
/**
 * Class to parse and encode strings into encoded variables and to interpret encoded variables back
 * into strings. An encoded variable is one of:
 * i)   a variable dictionary ID, referring to an entry in the variable dictionary, or
 * ii)  a value, representing an integer variable exactly as it appears in the original log message,
 *      or
 * iii) a value, representing a base-10, 16-digit number with a decimal point, where at least one
 *      digit is after the decimal point, encoded with a custom format.
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

        // Methods
        char const* what() const noexcept override {
            return "EncodedVariableInterpreter operation failed";
        }
    };

    // Methods
    static encoded_variable_t encode_var_dict_id(variable_dictionary_id_t id);
    static variable_dictionary_id_t decode_var_dict_id(encoded_variable_t encoded_var);
    /**
     * Converts the given string into a representable integer variable if possible
     * @param value
     * @param encoded_var
     * @return true if was successfully converted, false otherwise
     */
    static bool convert_string_to_representable_integer_var(
            std::string const& value,
            encoded_variable_t& encoded_var
    );
    /**
     * Converts the given string into a representable float variable if possible
     * @param value
     * @param encoded_var
     * @return true if was successfully converted, false otherwise
     */
    static bool convert_string_to_representable_float_var(
            std::string const& value,
            encoded_variable_t& encoded_var
    );
    /**
     * Converts the given encoded float into a string
     * @param encoded_var
     * @param value
     */
    static void convert_encoded_float_to_string(encoded_variable_t encoded_var, std::string& value);

    /**
     * Parses all variables from a message (while constructing the logtype) and encodes them (adding
     * them to the variable dictionary if necessary)
     * @param message
     * @param logtype_dict_entry
     * @param var_dict
     * @param encoded_vars
     * @param var_ids
     */
    static void encode_and_add_to_dictionary(
            std::string const& message,
            LogTypeDictionaryEntry& logtype_dict_entry,
            VariableDictionaryWriter& var_dict,
            std::vector<encoded_variable_t>& encoded_vars,
            std::vector<variable_dictionary_id_t>& var_ids
    );

    /**
     * Encodes the given IR log event, constructing a logtype dictionary entry, and adding any
     * dictionary variables to the dictionary. NOTE: Four-byte encoded variables will be converted
     * to eight-byte encoded variables.
     * @tparam encoded_variable_t The type of the encoded variables in the log event
     * @param log_event
     * @param logtype_dict_entry
     * @param var_dict
     * @param encoded_vars A container to store the encoded variables in
     * @param var_ids A container to store the dictionary IDs for dictionary variables
     * @param raw_num_bytes Returns an estimate of the number of bytes that this log event would
     * occupy if it was not encoded in CLP's IR
     */
    template <typename encoded_variable_t>
    static void encode_and_add_to_dictionary(
            ir::LogEvent<encoded_variable_t> const& log_event,
            LogTypeDictionaryEntry& logtype_dict_entry,
            VariableDictionaryWriter& var_dict,
            std::vector<ir::eight_byte_encoded_variable_t>& encoded_vars,
            std::vector<variable_dictionary_id_t>& var_ids,
            size_t& raw_num_bytes
    );

    /**
     * Decodes all variables and decompresses them into a message
     * @param logtype_dict_entry
     * @param var_dict
     * @param encoded_vars
     * @param decompressed_msg
     * @return true if successful, false otherwise
     */
    static bool decode_variables_into_message(
            LogTypeDictionaryEntry const& logtype_dict_entry,
            VariableDictionaryReader const& var_dict,
            std::vector<encoded_variable_t> const& encoded_vars,
            std::string& decompressed_msg
    );

    /**
     * Encodes a string-form variable, and if it is dictionary variable, searches for its ID in the
     * given variable dictionary
     * @param var_str
     * @param var_dict
     * @param ignore_case
     * @param logtype
     * @param sub_query
     * @return true if variable is a non-dictionary variable or was found in the given variable
     * dictionary
     * @return false otherwise
     */
    static bool encode_and_search_dictionary(
            std::string const& var_str,
            VariableDictionaryReader const& var_dict,
            bool ignore_case,
            std::string& logtype,
            SubQuery& sub_query
    );
    /**
     * Search for the given string-form variable in the variable dictionary, encode any matches, and
     * add them to the given sub-query
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
    /**
     * Encodes the given string as a dictionary or non-dictionary variable and adds a corresponding
     * placeholder to the logtype
     * @param var
     * @param logtype_dict_entry
     * @param var_dict
     * @param var_ids A container to add the dictionary ID to (if the string is a dictionary
     * variable)
     * @return The encoded variable
     */
    static encoded_variable_t encode_var(
            std::string const& var,
            LogTypeDictionaryEntry& logtype_dict_entry,
            VariableDictionaryWriter& var_dict,
            std::vector<variable_dictionary_id_t>& var_ids
    );

    /**
     * Adds the given string to the variable dictionary and adds a corresponding placeholder to
     * logtype
     * @param var
     * @param logtype_dict_entry
     * @param var_dict
     * @param var_ids A container to add the dictionary ID to
     * @return The dictionary ID
     */
    static variable_dictionary_id_t add_dict_var(
            std::string const& var,
            LogTypeDictionaryEntry& logtype_dict_entry,
            VariableDictionaryWriter& var_dict,
            std::vector<variable_dictionary_id_t>& var_ids
    );
};
}  // namespace clp

#endif  // CLP_ENCODEDVARIABLEINTERPRETER_HPP
