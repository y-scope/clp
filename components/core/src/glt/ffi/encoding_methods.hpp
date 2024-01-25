#ifndef GLT_FFI_ENCODING_METHODS_HPP
#define GLT_FFI_ENCODING_METHODS_HPP

#include <string>
#include <vector>

#include "../ir/parsing.hpp"
#include "../ir/types.hpp"
#include "../TraceableException.hpp"

// TODO Some of the methods in this file are mostly duplicated from code that exists elsewhere in
//  the repo. They should be consolidated in a future commit.
namespace glt::ffi {
class EncodingException : public TraceableException {
public:
    // Constructors
    EncodingException(
            ErrorCode error_code,
            char const* const filename,
            int line_number,
            std::string message
    )
            : TraceableException(error_code, filename, line_number),
              m_message(std::move(message)) {}

    // Methods
    [[nodiscard]] char const* what() const noexcept override { return m_message.c_str(); }

private:
    std::string m_message;
};

// Constants
/*
 * These constants can be used by callers to store the version of the schemas and encoding methods
 * they're using. At some point, we may update and/or add built-in schemas/encoding methods. So
 * callers must store the versions they used for encoding to ensure that they can choose the same
 * versions for decoding.
 *
 * We use versions which look like package names in anticipation of users writing their own custom
 * schemas and encoding methods.
 */
static constexpr char cVariableEncodingMethodsVersion[]
        = "com.yscope.clp.VariableEncodingMethodsV1";
static constexpr char cVariablesSchemaVersion[] = "com.yscope.clp.VariablesSchemaV2";

static constexpr char cTooFewDictionaryVarsErrorMessage[]
        = "There are fewer dictionary variables than dictionary variable placeholders in the "
          "logtype.";
static constexpr char cTooFewEncodedVarsErrorMessage[]
        = "There are fewer encoded variables than encoded variable placeholders in the logtype.";
static constexpr char cUnexpectedEscapeCharacterMessage[]
        = "Unexpected escape character without escaped value at the end of the logtype.";

constexpr size_t cMaxDigitsInRepresentableEightByteFloatVar = 16;
constexpr size_t cMaxDigitsInRepresentableFourByteFloatVar = 8;
constexpr uint64_t cEightByteEncodedFloatDigitsBitMask = (1ULL << 54) - 1;
constexpr uint32_t cFourByteEncodedFloatDigitsBitMask = (1UL << 25) - 1;

/**
 * Encodes the given string into a representable float variable if possible
 * @tparam encoded_variable_t Type of the encoded variable
 * @param str
 * @param encoded_var
 * @return true on success, false otherwise
 */
template <typename encoded_variable_t>
bool encode_float_string(std::string_view str, encoded_variable_t& encoded_var);

/**
 * Encodes the given four-byte encoded float using the eight-byte encoding
 * @param four_byte_encoded_var
 * @return The float using the eight-byte encoding
 */
ir::eight_byte_encoded_variable_t encode_four_byte_float_as_eight_byte(
        ir::four_byte_encoded_variable_t four_byte_encoded_var
);

/**
 * Encodes a float value with the given properties into an encoded variable.
 * NOTE: It's the caller's responsibility to validate that the input is a representable float.
 * @tparam encoded_variable_t Type of the encoded variable
 * @param is_negative
 * @param digits The digits of the float, ignoring the decimal, as an integer
 * @param num_digits The number of digits in \p digits
 * @param decimal_point_pos The position of the decimal point from the right of the value
 * @return The encoded variable
 */
template <typename encoded_variable_t>
encoded_variable_t encode_float_properties(
        bool is_negative,
        std::conditional_t<
                std::is_same_v<encoded_variable_t, ir::four_byte_encoded_variable_t>,
                uint32_t,
                uint64_t> digits,
        size_t num_digits,
        size_t decimal_point_pos
);

/**
 * Decodes an encoded float variable into its properties
 * @tparam encoded_variable_t Type of the encoded variable
 * @param encoded_var
 * @param is_negative Returns whether the float is negative
 * @param digits Returns the digits of the float, ignoring the decimal, as an integer
 * @param num_digits Returns the number of digits in \p digits
 * @param decimal_point_pos Returns the position of the decimal point from the right of the value
 */
template <typename encoded_variable_t>
void decode_float_properties(
        encoded_variable_t encoded_var,
        bool& is_negative,
        std::conditional_t<
                std::is_same_v<encoded_variable_t, ir::four_byte_encoded_variable_t>,
                uint32_t,
                uint64_t>& digits,
        uint8_t& num_digits,
        uint8_t& decimal_point_pos
);

/**
 * Decodes the given encoded float variable into a string
 * @tparam encoded_variable_t Type of the encoded variable
 * @param encoded_var
 * @return The decoded value as a string
 */
template <typename encoded_variable_t>
std::string decode_float_var(encoded_variable_t encoded_var);

/**
 * Encodes the given string into a representable integer variable if possible
 * @tparam encoded_variable_t Type of the encoded variable
 * @param str
 * @param encoded_var
 * @return true if successfully converted, false otherwise
 */
template <typename encoded_variable_t>
bool encode_integer_string(std::string_view str, encoded_variable_t& encoded_var);

/**
 * Encodes the given four-byte encoded integer using the eight-byte encoding
 * @param four_byte_encoded_var
 * @return The integer using the eight-byte encoding
 */
ir::eight_byte_encoded_variable_t encode_four_byte_integer_as_eight_byte(
        ir::four_byte_encoded_variable_t four_byte_encoded_var
);

/**
 * Decodes the given encoded integer variable into a string
 * @tparam encoded_variable_t Type of the encoded variable
 * @param encoded_var
 * @return The decoded value as a string
 */
template <typename encoded_variable_t>
std::string decode_integer_var(encoded_variable_t encoded_var);

/**
 * Encodes the given message and calls the given methods to handle specific components of the
 * message.
 * @tparam encoded_variable_t Type of the encoded variable
 * @tparam ConstantHandler Method to handle constants. Signature:
 * (std::string_view constant, std::string& logtype) -> void
 * @tparam EncodedVariableHandler Method to handle encoded variables. Signature:
 * (encoded_variable_t) -> void
 * @tparam DictionaryVariableHandler Method to handle dictionary variables. Signature:
 * (std::string_view message, size_t begin_pos, size_t end_pos) -> bool
 * @param message
 * @param logtype
 * @param constant_handler
 * @param encoded_variable_handler
 * @param dictionary_variable_handler
 * @return true on success, false otherwise
 */
template <
        typename encoded_variable_t,
        typename ConstantHandler,
        typename EncodedVariableHandler,
        typename DictionaryVariableHandler>
bool encode_message_generically(
        std::string_view message,
        std::string& logtype,
        ConstantHandler constant_handler,
        EncodedVariableHandler encoded_variable_handler,
        DictionaryVariableHandler dictionary_variable_handler
);

/**
 * Encodes the given message. The simplistic interface is to make it efficient to transfer data
 * between the caller language and this native code.
 * @tparam encoded_variable_t Type of the encoded variable
 * @param message
 * @param logtype
 * @param encoded_vars
 * @param dictionary_var_bounds A one-dimensional array containing the bounds (begin_pos followed by
 * end_pos) of each dictionary variable in the message
 * @return false if the message contains variable placeholders, true otherwise
 */
template <typename encoded_variable_t>
bool encode_message(
        std::string_view message,
        std::string& logtype,
        std::vector<encoded_variable_t>& encoded_vars,
        std::vector<int32_t>& dictionary_var_bounds
);

/**
 * Decodes the message from the given logtype, encoded variables, and dictionary variables. The
 * simplistic interface is to make it efficient to transfer data between the caller language and
 * this native code.
 * @tparam encoded_variable_t Type of the encoded variable
 * @param logtype
 * @param encoded_vars
 * @param encoded_vars_length
 * @param all_dictionary_vars The message's dictionary variables, stored back-to-back in a single
 * byte-array
 * @param dictionary_var_end_offsets The end-offset of each dictionary variable in
 * ``all_dictionary_vars``
 * @param dictionary_var_end_offsets_length
 * @return The decoded message
 */
template <typename encoded_variable_t>
std::string decode_message(
        std::string_view logtype,
        encoded_variable_t* encoded_vars,
        size_t encoded_vars_length,
        std::string_view all_dictionary_vars,
        int32_t const* dictionary_var_end_offsets,
        size_t dictionary_var_end_offsets_length
);

/**
 * Checks if any encoded variable matches the given wildcard query
 * NOTE: This method checks for *either* matching integer encoded variables or matching float
 * encoded variables, based on the variable placeholder template parameter.
 * @tparam var_placeholder Placeholder for the type of encoded variables that should be checked for
 * matches
 * @tparam encoded_variable_t Type of the encoded variable
 * @param wildcard_query
 * @param logtype
 * @param encoded_vars
 * @param encoded_vars_length
 * @return true if a match was found, false otherwise
 */
template <ir::VariablePlaceholder var_placeholder, typename encoded_variable_t>
bool wildcard_query_matches_any_encoded_var(
        std::string_view wildcard_query,
        std::string_view logtype,
        encoded_variable_t* encoded_vars,
        size_t encoded_vars_length
);

/**
 * Checks whether the given wildcard strings match the given encoded variables (from a message).
 * Specifically, let {w in W} be the set of wildcard strings and {e in E} be the set of encoded
 * variables. This method will return true only if:
 * (1) Each unique `w` matches a unique `e`.
 * (2) When (1) is true, the order of elements in both W and E is unchanged.
 * NOTE: Instead of taking an array of objects, this method takes arrays of object-members (the
 * result of serializing the objects) so that it can be called without unnecessarily reconstructing
 * the objects.
 * @tparam encoded_variable_t Type of the encoded variable
 * @param logtype The message's logtype
 * @param encoded_vars The message's encoded variables
 * @param encoded_vars_length The number of encoded variables in \p encoded_vars
 * @param wildcard_var_placeholders String of variable placeholders, where each one indicates how
 * the corresponding wildcard string should be interpreted.
 * @param wildcard_var_queries The wildcard strings to compare with the encoded variables. Callers
 * must ensure each wildcard string contains no redundant wildcards (e.g. "**") nor unnecessary
 * escape characters (e.g. "\").
 * @return Whether the wildcard strings match the encoded variables
 */
template <typename encoded_variable_t>
bool wildcard_match_encoded_vars(
        std::string_view logtype,
        encoded_variable_t* encoded_vars,
        size_t encoded_vars_length,
        std::string_view wildcard_var_placeholders,
        std::vector<std::string_view> const& wildcard_var_queries
);
}  // namespace glt::ffi

#include "encoding_methods.inc"

#endif  // GLT_FFI_ENCODING_METHODS_HPP
