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
    using eight_byte_encoded_variable_t = int64_t;

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

    static constexpr char cTooFewDictionaryVarsErrorMessage[] =
            "There are fewer dictionary variables than dictionary variable delimiters in the "
            "logtype.";
    static constexpr char cTooFewEncodedVarsErrorMessage[] =
            "There are fewer encoded variables than encoded variable delimiters in the logtype.";

    /**
     * @param c
     * @return Whether the character is a variable placeholder
     */
    bool is_variable_placeholder (char c);

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
}

#include "encoding_methods.tpp"

#endif // FFI_ENCODING_METHODS_HPP
