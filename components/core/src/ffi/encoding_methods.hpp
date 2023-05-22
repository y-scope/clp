#ifndef FFI_ENCODING_METHODS_HPP
#define FFI_ENCODING_METHODS_HPP

// C++ standard libraries
#include <string>
#include <vector>

// Project headers
#include "../TraceableException.hpp"

// TODO Some of the methods in this file are mostly duplicated from code that
//  exists elsewhere in the repo. They should be consolidated in a future
//  commit.
namespace ffi {
    // Types
    using epoch_time_ms_t = int64_t;

    using eight_byte_encoded_variable_t = int64_t;
    using four_byte_encoded_variable_t = int32_t;

    enum class VariablePlaceholder : char {
        Integer = 0x11,
        Dictionary = 0x12,
        Float = 0x13,
    };

    class EncodingException : public TraceableException {
    public:
        // Constructors
        EncodingException (ErrorCode error_code, const char* const filename, int line_number,
                           std::string message) :
                TraceableException(error_code, filename, line_number),
                m_message(std::move(message)) {}

        // Methods
        [[nodiscard]] const char* what() const noexcept override {
            return m_message.c_str();
        }

    private:
        std::string m_message;
    };

    // Constants
    /*
     * These constants can be used by callers to store the version of the
     * schemas and encoding methods they're using. At some point, we may update
     * and/or add built-in schemas/encoding methods. So callers must store the
     * versions they used for encoding to ensure that they can choose the same
     * versions for decoding.
     *
     * We use versions which look like package names in anticipation of users
     * writing their own custom schemas and encoding methods.
     */
    static constexpr char cVariableEncodingMethodsVersion[] =
            "com.yscope.clp.VariableEncodingMethodsV1";
    static constexpr char cVariablesSchemaVersion[] = "com.yscope.clp.VariablesSchemaV2";

    constexpr char cVariablePlaceholderEscapeCharacter = '\\';

    static constexpr char cTooFewDictionaryVarsErrorMessage[] =
            "There are fewer dictionary variables than dictionary variable delimiters in the "
            "logtype.";
    static constexpr char cTooFewEncodedVarsErrorMessage[] =
            "There are fewer encoded variables than encoded variable delimiters in the logtype.";
    static constexpr char cUnexpectedEscapeCharacterMessage[] =
            "Unexpected escape character without escaped value at the end of the logtype.";

    constexpr size_t cMaxDigitsInRepresentableEightByteFloatVar = 16;
    constexpr size_t cMaxDigitsInRepresentableFourByteFloatVar = 8;
    constexpr uint64_t cEightByteEncodedFloatDigitsBitMask = (1ULL << 54) - 1;
    constexpr uint32_t cFourByteEncodedFloatDigitsBitMask = (1UL << 25) - 1;

    /**
     * Checks if the given character is a delimiter
     * We treat everything *except* the following quoted characters as a
     * delimiter: "+-.0-9A-Z\_a-z"
     * @param c
     * @return Whether c is a delimiter
     */
    bool is_delim (signed char c);

    /**
     * @param c
     * @return Whether the character is a variable placeholder
     */
    bool is_variable_placeholder (char c);

    /**
     * @param str
     * @return Whether the given string could be a multi-digit hex value
     */
    bool could_be_multi_digit_hex_value (std::string_view str);

    /**
     * @param value
     * @return Whether the given value is a variable according to the schemas
     * specified in ffi::get_bounds_of_next_var
     */
    bool is_var (std::string_view value);

    /**
     * Gets the bounds of the next variable in the given string
     * A variable is a token (word between two delimiters) that matches one of
     * these schemas:
     * - ".*[0-9].*"
     * - "=(.*[a-zA-Z].*)" (the variable is within the capturing group)
     * - "[a-fA-F0-9]{2,}"
     * @param str String to search within
     * @param begin_pos Begin position of last variable, changes to begin
     * position of next variable
     * @param end_pos End position of last variable, changes to end position of
     * next variable
     * @param contains_var_placeholder Whether the string already contains a
     * variable placeholder (for efficiency, this is only set to true, so the
     * caller must reset it to false if necessary)
     * @return true if a variable was found, false otherwise
     */
    bool get_bounds_of_next_var (std::string_view str, size_t& begin_pos, size_t& end_pos,
                                 bool& contains_var_placeholder);

    /**
     * Encodes the given string into a representable float variable if
     * possible
     * @tparam encoded_variable_t Type of the encoded variable
     * @param str
     * @param encoded_var
     * @return true on success, false otherwise
     */
    template <typename encoded_variable_t>
    bool encode_float_string (std::string_view str, encoded_variable_t& encoded_var);

    /**
     * Encodes a float value with the given properties into an encoded variable
     * @tparam encoded_variable_t Type of the encoded variable
     * @param is_negative
     * @param digits The digits of the float, ignoring the decimal, as an
     * integer
     * @param num_digits The number of digits in \p digits
     * @param decimal_point_pos The position of the decimal point from the right
     * of the value
     * @return The encoded variable
     */
    template <typename encoded_variable_t>
    encoded_variable_t encode_float_properties (
            bool is_negative,
            std::conditional_t<std::is_same_v<encoded_variable_t, four_byte_encoded_variable_t>,
                    uint32_t, uint64_t> digits,
            size_t num_digits,
            size_t decimal_point_pos
    );

    /**
     * Decodes the given encoded float variable into a string
     * @tparam encoded_variable_t Type of the encoded variable
     * @param encoded_var
     * @return The decoded value as a string
     */
    template <typename encoded_variable_t>
    std::string decode_float_var (encoded_variable_t encoded_var);

    /**
     * Encodes the given string into a representable integer variable if
     * possible
     * @tparam encoded_variable_t Type of the encoded variable
     * @param str
     * @param encoded_var
     * @return true if successfully converted, false otherwise
     */
    template <typename encoded_variable_t>
    bool encode_integer_string (std::string_view str, encoded_variable_t& encoded_var);
    /**
     * Decodes the given encoded integer variable into a string
     * @tparam encoded_variable_t Type of the encoded variable
     * @param encoded_var
     * @return The decoded value as a string
     */
    template <typename encoded_variable_t>
    std::string decode_integer_var (encoded_variable_t encoded_var);

    /**
     * Encodes the given message and calls the given methods to handle specific
     * components of the message.
     * @tparam encoded_variable_t Type of the encoded variable
     * @tparam ConstantHandler Method to handle constants. Signature:
     * (std::string_view constant, bool constant_contains_variable_placeholder,
     * std::string& logtype) -> bool
     * @tparam FinalConstantHandler Method to handle the constant after the last
     * variable. Signature: (std::string_view constant, std::string& logtype) -> bool
     * @tparam EncodedVariableHandler Method to handle encoded variables.
     * Signature: (encoded_variable_t) -> void
     * @tparam DictionaryVariableHandler Method to handle dictionary variables.
     * Signature: (std::string_view message, size_t begin_pos, size_t end_pos) -> bool
     * @param message
     * @param logtype
     * @param constant_handler
     * @param final_constant_handler
     * @param encoded_variable_handler
     * @param dictionary_variable_handler
     * @return true on success, false otherwise
     */
    template <typename encoded_variable_t, typename ConstantHandler, typename FinalConstantHandler,
            typename EncodedVariableHandler, typename DictionaryVariableHandler>
    bool encode_message_generically (std::string_view message, std::string& logtype,
                                     ConstantHandler constant_handler,
                                     FinalConstantHandler final_constant_handler,
                                     EncodedVariableHandler encoded_variable_handler,
                                     DictionaryVariableHandler dictionary_variable_handler);

    /**
     * Encodes the given message. The simplistic interface is to make it
     * efficient to transfer data between the caller language and this native
     * code.
     * @tparam encoded_variable_t Type of the encoded variable
     * @param message
     * @param logtype
     * @param encoded_vars
     * @param dictionary_var_bounds A one-dimensional array containing the
     * bounds (begin_pos followed by end_pos) of each dictionary variable in the
     * message
     * @return false if the message contains variable placeholders, true
     * otherwise
     */
    template <typename encoded_variable_t>
    bool encode_message (std::string_view message, std::string& logtype,
                         std::vector<encoded_variable_t>& encoded_vars,
                         std::vector<int32_t>& dictionary_var_bounds);

    /**
     * Decodes the message from the given logtype, encoded variables, and
     * dictionary variables. The simplistic interface is to make it efficient
     * to transfer data between the caller language and this native code.
     * @tparam encoded_variable_t Type of the encoded variable
     * @param logtype
     * @param encoded_vars
     * @param encoded_vars_length
     * @param all_dictionary_vars The message's dictionary variables, stored
     * back-to-back in a single byte-array
     * @param dictionary_var_end_offsets The end-offset of each dictionary
     * variable in ``all_dictionary_vars``
     * @param dictionary_var_end_offsets_length
     * @return The decoded message
     */
    template <typename encoded_variable_t>
    std::string decode_message (
            std::string_view logtype,
            encoded_variable_t* encoded_vars,
            size_t encoded_vars_length,
            std::string_view all_dictionary_vars,
            const int32_t* dictionary_var_end_offsets,
            size_t dictionary_var_end_offsets_length
    );

    /**
     * Checks if any encoded variable matches the given wildcard query
     * NOTE: This method checks for *either* matching integer encoded variables
     * or matching float encoded variables, based on the variable placeholder
     * template parameter.
     * @tparam var_placeholder Placeholder for the type of encoded variables
     * that should be checked for matches
     * @tparam encoded_variable_t Type of the encoded variable
     * @param wildcard_query
     * @param logtype
     * @param encoded_vars
     * @param encoded_vars_length
     * @return true if a match was found, false otherwise
     */
    template <VariablePlaceholder var_placeholder, typename encoded_variable_t>
    bool wildcard_query_matches_any_encoded_var (
            std::string_view wildcard_query,
            std::string_view logtype,
            encoded_variable_t* encoded_vars,
            int encoded_vars_length
    );

    /**
     * Checks whether the given wildcard strings match the given encoded
     * variables (from a message). Specifically, let {w in W} be the set of
     * wildcard strings and {e in E} be the set of encoded variables. This
     * method will return true only if:
     * (1) Each unique `w` matches a unique `e`.
     * (2) When (1) is true, the order of elements in both W and E is unchanged.
     * NOTE: Instead of taking an array of objects, this method takes arrays of
     * object-members (the result of serializing the objects) so that it can be
     * called without unnecessarily reconstructing the objects.
     * @tparam encoded_variable_t Type of the encoded variable
     * @param logtype The message's logtype
     * @param encoded_vars The message's encoded variables
     * @param encoded_vars_length The number of encoded variables in \p
     * encoded_vars
     * @param wildcard_var_placeholders String of variable placeholders, where
     * each one indicates how the corresponding wildcard string should be
     * interpreted.
     * @param wildcard_var_queries The wildcard strings to compare with the
     * encoded variables. Callers must ensure each wildcard string contains
     * no redundant wildcards (e.g. "**") nor unnecessary escape characters
     * (e.g. "\").
     * @return Whether the wildcard strings match the encoded variables
     */
    template <typename encoded_variable_t>
    bool wildcard_match_encoded_vars (
            std::string_view logtype,
            encoded_variable_t* encoded_vars,
            size_t encoded_vars_length,
            std::string_view wildcard_var_placeholders,
            const std::vector<std::string_view>& wildcard_var_queries
    );
}

#include "encoding_methods.tpp"

#endif // FFI_ENCODING_METHODS_HPP
