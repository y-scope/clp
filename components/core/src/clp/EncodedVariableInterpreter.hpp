#ifndef CLP_ENCODEDVARIABLEINTERPRETER_HPP
#define CLP_ENCODEDVARIABLEINTERPRETER_HPP

#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "ffi/ir_stream/decoding_methods.hpp"
#include "ir/LogEvent.hpp"
#include "ir/types.hpp"
#include "Query.hpp"
#include "spdlog_with_specializations.hpp"
#include "TraceableException.hpp"
#include "type_utils.hpp"

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
     * Adds a dictionary variable placeholder to the given logtype
     * @param logtype
     */
    static void add_dict_var(std::string& logtype) {
        logtype.push_back(enum_to_underlying_type(ir::VariablePlaceholder::Dictionary));
    }

    /**
     * Adds an integer variable placeholder to the given logtype
     * @param logtype
     */
    static void add_int_var(std::string& logtype) {
        logtype.push_back(enum_to_underlying_type(ir::VariablePlaceholder::Integer));
    }

    /**
     * Adds a float variable placeholder to the given logtype
     * @param logtype
     */
    static void add_float_var(std::string& logtype) {
        logtype.push_back(enum_to_underlying_type(ir::VariablePlaceholder::Float));
    }

    /**
     * Adds an escape character to the given logtype
     * @param logtype
     */
    static void add_escape(std::string& logtype) {
        logtype.push_back(enum_to_underlying_type(ir::VariablePlaceholder::Escape));
    }

    /**
     * Converts the given string into a representable integer variable if possible
     * @param value
     * @param encoded_var
     * @return true if was successfully converted, false otherwise
     */
    static bool convert_string_to_representable_integer_var(
            std::string_view value,
            encoded_variable_t& encoded_var
    );
    /**
     * Converts the given string into a representable float variable if possible
     * @param value
     * @param encoded_var
     * @return true if was successfully converted, false otherwise
     */
    static bool convert_string_to_representable_float_var(
            std::string_view value,
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
     * @tparam LogTypeDictionaryEntryType
     * @tparam VariableDictionaryWriterType
     * @param message
     * @param logtype_dict_entry
     * @param var_dict
     * @param encoded_vars
     * @param var_ids
     */
    template <typename LogTypeDictionaryEntryType, typename VariableDictionaryWriterType>
    static void encode_and_add_to_dictionary(
            std::string_view message,
            LogTypeDictionaryEntryType& logtype_dict_entry,
            VariableDictionaryWriterType& var_dict,
            std::vector<encoded_variable_t>& encoded_vars,
            std::vector<variable_dictionary_id_t>& var_ids
    );

    /**
     * Encodes the given IR log event, constructing a logtype dictionary entry, and adding any
     * dictionary variables to the dictionary. NOTE: Four-byte encoded variables will be converted
     * to eight-byte encoded variables.
     * @tparam EncodedVariableType The type of the encoded variables in the log event.
     * @tparam LogTypeDictionaryEntryType
     * @tparam VariableDictionaryWriterType
     * @param log_event
     * @param logtype_dict_entry
     * @param var_dict
     * @param encoded_vars A container to store the encoded variables in
     * @param var_ids A container to store the dictionary IDs for dictionary variables
     * @param raw_num_bytes Returns an estimate of the number of bytes that this log event would
     * occupy if it was not encoded in CLP's IR
     */
    template <
            typename EncodedVariableType,
            typename LogTypeDictionaryEntryType,
            typename VariableDictionaryWriterType>
    static void encode_and_add_to_dictionary(
            ir::LogEvent<EncodedVariableType> const& log_event,
            LogTypeDictionaryEntryType& logtype_dict_entry,
            VariableDictionaryWriterType& var_dict,
            std::vector<ir::eight_byte_encoded_variable_t>& encoded_vars,
            std::vector<variable_dictionary_id_t>& var_ids,
            size_t& raw_num_bytes
    );

    /**
     * Decodes all variables and decompresses them into a message
     * @tparam LogTypeDictionaryEntryType
     * @tparam VariableDictionaryReaderType
     * @tparam EncodedVariableContainerType A random access list of `clp::encoded_variable_t`.
     * @param logtype_dict_entry
     * @param var_dict
     * @param encoded_vars
     * @param decompressed_msg
     * @return true if successful, false otherwise
     */
    template <
            typename LogTypeDictionaryEntryType,
            typename VariableDictionaryReaderType,
            typename EncodedVariableContainerType>
    static bool decode_variables_into_message(
            LogTypeDictionaryEntryType const& logtype_dict_entry,
            VariableDictionaryReaderType const& var_dict,
            EncodedVariableContainerType const& encoded_vars,
            std::string& decompressed_msg
    );

    /**
     * Encodes a string-form variable, and if it is dictionary variable, searches for its ID in the
     * given variable dictionary.
     * @tparam VariableDictionaryReaderType
     * @tparam VariableDictionaryEntryType
     * @param var_str
     * @param var_dict
     * @param ignore_case
     * @param logtype
     * @param sub_query
     * @return true if variable is a non-dictionary variable or was found in the given variable
     * dictionary
     * @return false otherwise
     */
    template <
            typename VariableDictionaryReaderType,
            typename VariableDictionaryEntryType = typename VariableDictionaryReaderType::entry_t>
    static bool encode_and_search_dictionary(
            std::string_view var_str,
            VariableDictionaryReaderType const& var_dict,
            bool ignore_case,
            std::string& logtype,
            SubQuery& sub_query
    );
    /**
     * Search for the given string-form variable in the variable dictionary, encode any matches, and
     * add them to the given sub-query.
     * @tparam VariableDictionaryReaderType
     * @tparam VariableDictionaryEntryType
     * @param var_wildcard_str
     * @param var_dict
     * @param ignore_case
     * @param sub_query
     * @return true if any match found, false otherwise
     */
    template <
            typename VariableDictionaryReaderType,
            typename VariableDictionaryEntryType = typename VariableDictionaryReaderType::entry_t>
    static bool wildcard_search_dictionary_and_get_encoded_matches(
            std::string_view var_wildcard_str,
            VariableDictionaryReaderType const& var_dict,
            bool ignore_case,
            SubQuery& sub_query
    );

private:
    /**
     * Encodes the given string as a dictionary or non-dictionary variable and adds a corresponding
     * placeholder to the logtype.
     * @tparam LogTypeDictionaryEntryType
     * @tparam VariableDictionaryWriterType
     * @param var
     * @param logtype_dict_entry
     * @param var_dict
     * @param var_ids A container to add the dictionary ID to (if the string is a dictionary
     * variable)
     * @return The encoded variable
     */
    template <typename LogTypeDictionaryEntryType, typename VariableDictionaryWriterType>
    static encoded_variable_t encode_var(
            std::string_view var,
            LogTypeDictionaryEntryType& logtype_dict_entry,
            VariableDictionaryWriterType& var_dict,
            std::vector<variable_dictionary_id_t>& var_ids
    );

    /**
     * Adds the given string to the variable dictionary and adds a corresponding placeholder to
     * logtype.
     * @tparam LogTypeDictionaryEntryType
     * @tparam VariableDictionaryWriterType
     * @param var
     * @param logtype_dict_entry
     * @param var_dict
     * @param var_ids A container to add the dictionary ID to
     * @return The dictionary ID
     */
    template <typename LogTypeDictionaryEntryType, typename VariableDictionaryWriterType>
    static variable_dictionary_id_t add_dict_var(
            std::string_view var,
            LogTypeDictionaryEntryType& logtype_dict_entry,
            VariableDictionaryWriterType& var_dict,
            std::vector<variable_dictionary_id_t>& var_ids
    );
};

template <typename LogTypeDictionaryEntryType, typename VariableDictionaryWriterType>
void EncodedVariableInterpreter::encode_and_add_to_dictionary(
        std::string_view message,
        LogTypeDictionaryEntryType& logtype_dict_entry,
        VariableDictionaryWriterType& var_dict,
        std::vector<encoded_variable_t>& encoded_vars,
        std::vector<variable_dictionary_id_t>& var_ids
) {
    // Extract all variables and add to dictionary while building logtype
    size_t var_begin_pos = 0;
    size_t var_end_pos = 0;
    std::string_view var_str;
    logtype_dict_entry.clear();
    // To avoid reallocating the logtype as we append to it, reserve enough space to hold the entire
    // message
    logtype_dict_entry.reserve_constant_length(message.length());
    while (logtype_dict_entry.parse_next_var(message, var_begin_pos, var_end_pos, var_str)) {
        auto encoded_var = encode_var(var_str, logtype_dict_entry, var_dict, var_ids);
        encoded_vars.push_back(encoded_var);
    }
}

template <
        typename EncodedVariableType,
        typename LogTypeDictionaryEntryType,
        typename VariableDictionaryWriterType>
void EncodedVariableInterpreter::encode_and_add_to_dictionary(
        ir::LogEvent<EncodedVariableType> const& log_event,
        LogTypeDictionaryEntryType& logtype_dict_entry,
        VariableDictionaryWriterType& var_dict,
        std::vector<ir::eight_byte_encoded_variable_t>& encoded_vars,
        std::vector<variable_dictionary_id_t>& var_ids,
        size_t& raw_num_bytes
) {
    logtype_dict_entry.clear();
    auto const& log_message = log_event.get_message();
    logtype_dict_entry.reserve_constant_length(log_message.get_logtype().length());

    raw_num_bytes = 0;

    auto constant_handler = [&](std::string const& value, size_t begin_pos, size_t length) {
        raw_num_bytes += length;
        logtype_dict_entry.add_constant(value, begin_pos, length);
    };

    auto encoded_int_handler = [&](EncodedVariableType encoded_var) {
        raw_num_bytes += ffi::decode_integer_var(encoded_var).length();
        logtype_dict_entry.add_int_var();

        ir::eight_byte_encoded_variable_t eight_byte_encoded_var{};
        if constexpr (std::is_same_v<EncodedVariableType, ir::eight_byte_encoded_variable_t>) {
            eight_byte_encoded_var = encoded_var;
        } else {  // std::is_same_v<EncodedVariableType, ir::four_byte_encoded_variable_t>
            eight_byte_encoded_var = ffi::encode_four_byte_integer_as_eight_byte(encoded_var);
        }
        encoded_vars.push_back(eight_byte_encoded_var);
    };

    auto encoded_float_handler = [&](EncodedVariableType encoded_var) {
        raw_num_bytes += ffi::decode_float_var(encoded_var).length();
        logtype_dict_entry.add_float_var();

        ir::eight_byte_encoded_variable_t eight_byte_encoded_var{};
        if constexpr (std::is_same_v<EncodedVariableType, ir::eight_byte_encoded_variable_t>) {
            eight_byte_encoded_var = encoded_var;
        } else {  // std::is_same_v<EncodedVariableType, ir::four_byte_encoded_variable_t>
            eight_byte_encoded_var = ffi::encode_four_byte_float_as_eight_byte(encoded_var);
        }
        encoded_vars.push_back(eight_byte_encoded_var);
    };

    auto dict_var_handler = [&](std::string const& dict_var) {
        raw_num_bytes += dict_var.length();

        ir::eight_byte_encoded_variable_t encoded_var{};
        if constexpr (std::is_same_v<EncodedVariableType, ir::eight_byte_encoded_variable_t>) {
            encoded_var = encode_var_dict_id(
                    add_dict_var(dict_var, logtype_dict_entry, var_dict, var_ids)
            );
        } else {  // std::is_same_v<EncodedVariableType, ir::four_byte_encoded_variable_t>
            encoded_var = encode_var(dict_var, logtype_dict_entry, var_dict, var_ids);
        }
        encoded_vars.push_back(encoded_var);
    };

    ffi::ir_stream::generic_decode_message<false>(
            log_message.get_logtype(),
            log_message.get_encoded_vars(),
            log_message.get_dict_vars(),
            constant_handler,
            encoded_int_handler,
            encoded_float_handler,
            dict_var_handler
    );
}

template <
        typename LogTypeDictionaryEntryType,
        typename VariableDictionaryReaderType,
        typename EncodedVariableContainerType>
bool EncodedVariableInterpreter::decode_variables_into_message(
        LogTypeDictionaryEntryType const& logtype_dict_entry,
        VariableDictionaryReaderType const& var_dict,
        EncodedVariableContainerType const& encoded_vars,
        std::string& decompressed_msg
) {
    // Ensure the number of variables in the logtype matches the number of encoded variables given
    auto const& logtype_value = logtype_dict_entry.get_value();
    size_t const num_vars = logtype_dict_entry.get_num_variables();
    if (num_vars != encoded_vars.size()) {
        SPDLOG_ERROR(
                "EncodedVariableInterpreter: Logtype '{}' contains {} variables, but {} were given "
                "for decoding.",
                logtype_value.c_str(),
                num_vars,
                encoded_vars.size()
        );
        return false;
    }

    ir::VariablePlaceholder var_placeholder;
    size_t constant_begin_pos = 0;
    std::string float_str;
    variable_dictionary_id_t var_dict_id;
    size_t const num_placeholders_in_logtype = logtype_dict_entry.get_num_placeholders();
    for (size_t placeholder_ix = 0, var_ix = 0; placeholder_ix < num_placeholders_in_logtype;
         ++placeholder_ix)
    {
        size_t placeholder_position
                = logtype_dict_entry.get_placeholder_info(placeholder_ix, var_placeholder);

        // Add the constant that's between the last placeholder and this one
        decompressed_msg.append(
                logtype_value,
                constant_begin_pos,
                placeholder_position - constant_begin_pos
        );
        switch (var_placeholder) {
            case ir::VariablePlaceholder::Integer:
                decompressed_msg += std::to_string(encoded_vars[var_ix++]);
                break;
            case ir::VariablePlaceholder::Float:
                convert_encoded_float_to_string(encoded_vars[var_ix++], float_str);
                decompressed_msg += float_str;
                break;
            case ir::VariablePlaceholder::Dictionary:
                var_dict_id = decode_var_dict_id(encoded_vars[var_ix++]);
                decompressed_msg += var_dict.get_value(var_dict_id);
                break;
            case ir::VariablePlaceholder::Escape:
                break;
            default:
                SPDLOG_ERROR(
                        "EncodedVariableInterpreter: Logtype '{}' contains unexpected variable "
                        "placeholder 0x{:x}",
                        logtype_value,
                        enum_to_underlying_type(var_placeholder)
                );
                return false;
        }
        // Move past the variable placeholder
        constant_begin_pos = placeholder_position + 1;
    }
    // Append remainder of logtype, if any
    if (constant_begin_pos < logtype_value.length()) {
        decompressed_msg.append(logtype_value, constant_begin_pos, std::string::npos);
    }

    return true;
}

template <typename VariableDictionaryReaderType, typename VariableDictionaryEntryType>
bool EncodedVariableInterpreter::encode_and_search_dictionary(
        std::string_view var_str,
        VariableDictionaryReaderType const& var_dict,
        bool ignore_case,
        std::string& logtype,
        SubQuery& sub_query
) {
    size_t length = var_str.length();
    if (0 == length) {
        throw OperationFailed(ErrorCode_BadParam, __FILENAME__, __LINE__);
    }

    encoded_variable_t encoded_var;
    if (convert_string_to_representable_integer_var(var_str, encoded_var)) {
        add_int_var(logtype);
        sub_query.add_non_dict_var(encoded_var);
    } else if (convert_string_to_representable_float_var(var_str, encoded_var)) {
        add_float_var(logtype);
        sub_query.add_non_dict_var(encoded_var);
    } else {
        auto const entries = var_dict.get_entry_matching_value(var_str, ignore_case);
        if (entries.empty()) {
            // Not in dictionary
            return false;
        }

        add_dict_var(logtype);

        if (entries.size() == 1) {
            auto const* entry = entries.at(0);
            sub_query.add_dict_var(encode_var_dict_id(entry->get_id()), entry);
            return true;
        }

        std::unordered_set<VariableDictionaryEntryType const*> const entries_set{
                entries.cbegin(),
                entries.cend()
        };
        std::unordered_set<encoded_variable_t> encoded_vars;
        encoded_vars.reserve(entries.size());
        for (auto const* entry : entries) {
            encoded_vars.emplace(encode_var_dict_id(entry->get_id()));
        }
        sub_query.add_imprecise_dict_var(encoded_vars, entries_set);
    }

    return true;
}

template <typename VariableDictionaryReaderType, typename VariableDictionaryEntryType>
bool EncodedVariableInterpreter::wildcard_search_dictionary_and_get_encoded_matches(
        std::string_view var_wildcard_str,
        VariableDictionaryReaderType const& var_dict,
        bool ignore_case,
        SubQuery& sub_query
) {
    // Find matches
    std::unordered_set<VariableDictionaryEntryType const*> var_dict_entries;
    var_dict.get_entries_matching_wildcard_string(var_wildcard_str, ignore_case, var_dict_entries);
    if (var_dict_entries.empty()) {
        // Not in dictionary
        return false;
    }

    // Encode matches
    std::unordered_set<encoded_variable_t> encoded_vars;
    for (auto entry : var_dict_entries) {
        encoded_vars.emplace(encode_var_dict_id(entry->get_id()));
    }

    sub_query.add_imprecise_dict_var(encoded_vars, var_dict_entries);

    return true;
}

template <typename LogTypeDictionaryEntryType, typename VariableDictionaryWriterType>
encoded_variable_t EncodedVariableInterpreter::encode_var(
        std::string_view var,
        LogTypeDictionaryEntryType& logtype_dict_entry,
        VariableDictionaryWriterType& var_dict,
        std::vector<variable_dictionary_id_t>& var_ids
) {
    encoded_variable_t encoded_var{0};
    if (convert_string_to_representable_integer_var(var, encoded_var)) {
        logtype_dict_entry.add_int_var();
    } else if (convert_string_to_representable_float_var(var, encoded_var)) {
        logtype_dict_entry.add_float_var();
    } else {
        // Variable string looks like a dictionary variable, so encode it as so
        encoded_var = encode_var_dict_id(add_dict_var(var, logtype_dict_entry, var_dict, var_ids));
    }
    return encoded_var;
}

template <typename LogTypeDictionaryEntryType, typename VariableDictionaryWriterType>
variable_dictionary_id_t EncodedVariableInterpreter::add_dict_var(
        std::string_view var,
        LogTypeDictionaryEntryType& logtype_dict_entry,
        VariableDictionaryWriterType& var_dict,
        std::vector<variable_dictionary_id_t>& var_ids
) {
    variable_dictionary_id_t id{cVariableDictionaryIdMax};
    var_dict.add_entry(var, id);
    var_ids.push_back(id);

    logtype_dict_entry.add_dictionary_var();

    return id;
}
}  // namespace clp

#endif  // CLP_ENCODEDVARIABLEINTERPRETER_HPP
