#include <Catch2/single_include/catch2/catch.hpp>

#include "../src/clp/ffi/encoding_methods.hpp"
#include "../src/clp/ir/types.hpp"

using clp::enum_to_underlying_type;
using clp::ffi::decode_float_var;
using clp::ffi::decode_integer_var;
using clp::ffi::decode_message;
using clp::ffi::encode_float_properties;
using clp::ffi::encode_float_string;
using clp::ffi::encode_integer_string;
using clp::ffi::encode_message;
using clp::ffi::wildcard_match_encoded_vars;
using clp::ffi::wildcard_query_matches_any_encoded_var;
using clp::ir::eight_byte_encoded_variable_t;
using clp::ir::four_byte_encoded_variable_t;
using clp::ir::get_bounds_of_next_var;
using clp::ir::VariablePlaceholder;
using std::string;
using std::string_view;
using std::vector;

// Local function prototypes
/**
 * Fills a vector of string views from the given vector of strings
 * @param strings
 * @param string_views
 */
static void
string_views_from_strings(vector<string> const& strings, vector<string_view>& string_views);

TEMPLATE_TEST_CASE(
        "Encoding integers",
        "[ffi][encode-integer]",
        eight_byte_encoded_variable_t,
        four_byte_encoded_variable_t
) {
    // Code below only supports these two types right now
    static_assert(
            (std::is_same_v<TestType, eight_byte_encoded_variable_t>
             || std::is_same_v<TestType, four_byte_encoded_variable_t>)
    );

    string value;
    string decoded_value;
    TestType encoded_var;

    // Test basic conversions
    value = "0";
    REQUIRE(encode_integer_string(value, encoded_var));
    decoded_value = decode_integer_var(encoded_var);
    REQUIRE(decoded_value == value);

    value = "-1";
    REQUIRE(encode_integer_string(value, encoded_var));
    decoded_value = decode_integer_var(encoded_var);
    REQUIRE(decoded_value == value);

    value = "1";
    REQUIRE(encode_integer_string(value, encoded_var));
    decoded_value = decode_integer_var(encoded_var);
    REQUIRE(decoded_value == value);

    // Test edges of representable range
    int64_t min_value;
    int64_t max_value;
    if constexpr (std::is_same_v<TestType, eight_byte_encoded_variable_t>) {
        min_value = INT64_MIN;
        max_value = INT64_MAX;
    } else {  // std::is_same_v<TestType, four_byte_encoded_variable_t>
        min_value = INT32_MIN;
        max_value = INT32_MAX;
    }
    value = std::to_string(min_value);
    REQUIRE(encode_integer_string(value, encoded_var));
    decoded_value = decode_integer_var(encoded_var);
    REQUIRE(decoded_value == value);

    value = std::to_string(max_value);
    REQUIRE(encode_integer_string(value, encoded_var));
    decoded_value = decode_integer_var(encoded_var);
    REQUIRE(decoded_value == value);

    if constexpr (std::is_same_v<TestType, eight_byte_encoded_variable_t>) {
        value = "9223372036854775808";  // INT64_MAX + 1 == 2^63
    } else {  // std::is_same_v<TestType, four_byte_encoded_variable_t>
        value = "2147483648";  // INT32_MAX + 1 == 2^31
    }
    REQUIRE(false == encode_integer_string(value, encoded_var));

    if constexpr (std::is_same_v<TestType, eight_byte_encoded_variable_t>) {
        value = "-9223372036854775809";  // INT64_MIN - 1 == -2^63 - 1
    } else {  // std::is_same_v<TestType, four_byte_encoded_variable_t>
        value = "-2147483649";  // INT32_MIN - 1 == -2^31 - 1
    }
    REQUIRE(false == encode_integer_string(value, encoded_var));

    // Test non-integers
    value = "";
    REQUIRE(!encode_integer_string(value, encoded_var));

    value = "a";
    REQUIRE(!encode_integer_string(value, encoded_var));

    value = "-";
    REQUIRE(!encode_integer_string(value, encoded_var));

    value = "+";
    REQUIRE(!encode_integer_string(value, encoded_var));

    value = "-a";
    REQUIRE(!encode_integer_string(value, encoded_var));

    value = "+a";
    REQUIRE(!encode_integer_string(value, encoded_var));

    value = "--";
    REQUIRE(!encode_integer_string(value, encoded_var));

    value = "++";
    REQUIRE(!encode_integer_string(value, encoded_var));

    // Test unrepresentable values
    value = " 1";
    REQUIRE(!encode_integer_string(value, encoded_var));

    value = "- 1";
    REQUIRE(!encode_integer_string(value, encoded_var));

    value = "1 ";
    REQUIRE(!encode_integer_string(value, encoded_var));

    value = "01";
    REQUIRE(!encode_integer_string(value, encoded_var));

    value = "+1";
    REQUIRE(!encode_integer_string(value, encoded_var));

    value = "1u";
    REQUIRE(!encode_integer_string(value, encoded_var));

    value = "1U";
    REQUIRE(!encode_integer_string(value, encoded_var));

    value = "1l";
    REQUIRE(!encode_integer_string(value, encoded_var));

    value = "1L";
    REQUIRE(!encode_integer_string(value, encoded_var));

    value = "1ll";
    REQUIRE(!encode_integer_string(value, encoded_var));

    value = "1LL";
    REQUIRE(!encode_integer_string(value, encoded_var));

    value = "0.0";
    REQUIRE(!encode_integer_string(value, encoded_var));

    value = "-0";
    REQUIRE(!encode_integer_string(value, encoded_var));
}

TEMPLATE_TEST_CASE(
        "Encoding floats",
        "[ffi][encode-float]",
        eight_byte_encoded_variable_t,
        four_byte_encoded_variable_t
) {
    string value;
    string decoded_value;
    TestType encoded_var;

    // Test basic conversions
    value = "0.0";
    REQUIRE(encode_float_string(value, encoded_var));
    decoded_value = decode_float_var(encoded_var);
    REQUIRE(decoded_value == value);

    value = "-1.0";
    REQUIRE(encode_float_string(value, encoded_var));
    decoded_value = decode_float_var(encoded_var);
    REQUIRE(decoded_value == value);

    value = "1.0";
    REQUIRE(encode_float_string(value, encoded_var));
    decoded_value = decode_float_var(encoded_var);
    REQUIRE(decoded_value == value);

    value = ".1";
    REQUIRE(encode_float_string(value, encoded_var));
    decoded_value = decode_float_var(encoded_var);
    REQUIRE(decoded_value == value);

    value = "-00.00";
    REQUIRE(encode_float_string(value, encoded_var));
    decoded_value = decode_float_var(encoded_var);
    REQUIRE(decoded_value == value);

    // Test edges of representable range
    if constexpr (std::is_same_v<TestType, eight_byte_encoded_variable_t>) {
        value = "-999999999999999.9";
    } else {  // std::is_same_v<TestType, four_byte_encoded_variable_t>
        value = "-3355443.1";
    }
    REQUIRE(encode_float_string(value, encoded_var));
    decoded_value = decode_float_var(encoded_var);
    REQUIRE(decoded_value == value);

    if constexpr (std::is_same_v<TestType, eight_byte_encoded_variable_t>) {
        value = "999999999999999.9";
    } else {  // std::is_same_v<TestType, four_byte_encoded_variable_t>
        value = "3355443.1";
    }
    REQUIRE(encode_float_string(value, encoded_var));
    decoded_value = decode_float_var(encoded_var);
    REQUIRE(decoded_value == value);

    if constexpr (std::is_same_v<TestType, eight_byte_encoded_variable_t>) {
        value = "-.9999999999999999";
    } else {  // std::is_same_v<TestType, four_byte_encoded_variable_t>
        value = "-.33554431";
    }
    REQUIRE(encode_float_string(value, encoded_var));
    decoded_value = decode_float_var(encoded_var);
    REQUIRE(decoded_value == value);

    if constexpr (std::is_same_v<TestType, eight_byte_encoded_variable_t>) {
        value = ".9999999999999999";
    } else {  // std::is_same_v<TestType, four_byte_encoded_variable_t>
        value = ".33554431";
    }
    REQUIRE(encode_float_string(value, encoded_var));
    decoded_value = decode_float_var(encoded_var);
    REQUIRE(decoded_value == value);

    SECTION("Test unrepresentable floats") {
        if constexpr (std::is_same_v<TestType, four_byte_encoded_variable_t>) {
            string const unrepresentable_values = GENERATE(
                    "0.33554431",
                    "-0.33554431",
                    "3.3554432",
                    "-3.3554432",
                    "60.000004",
                    "-60.000004"
            );
            REQUIRE(false == encode_float_string(unrepresentable_values, encoded_var));
        }
    }

    SECTION("Test non-floats") {
        string const non_floating_values = GENERATE(
                "",
                "a",
                "-",
                "+",
                "-a",
                "+a",
                "--",
                "++",
                ".",
                "1.",
                " 1.0",
                "1.0 ",
                "- 1.0",
                "+1.0",
                "1.0f"
                "1.0F",
                "1.0l",
                "1.0L",
                "1.0.0"
        );
        REQUIRE(false == encode_float_string(non_floating_values, encoded_var));
    }
}

TEMPLATE_TEST_CASE(
        "encode_float_properties",
        "[ffi][encode-float]",
        eight_byte_encoded_variable_t,
        four_byte_encoded_variable_t
) {
    // Test all possible combinations of the properties of the encoded float, except for individual
    // values of the 'digits' field, since that takes too long.
    constexpr size_t cMaxDigitsInRepresentableFloatVar
            = std::is_same_v<TestType, four_byte_encoded_variable_t>
                      ? clp::ffi::cMaxDigitsInRepresentableFourByteFloatVar
                      : clp::ffi::cMaxDigitsInRepresentableEightByteFloatVar;
    for (size_t num_digits_in_digits_property = 0;
         num_digits_in_digits_property <= cMaxDigitsInRepresentableFloatVar + 1;
         ++num_digits_in_digits_property)
    {
        // Iterate over the possible values of the `num_digits` property
        for (uint8_t num_digits = 1; num_digits <= cMaxDigitsInRepresentableFloatVar; ++num_digits)
        {
            // Iterate over the possible values of the `decimal_point_pos` property
            for (uint8_t decimal_point_pos = 1;
                 decimal_point_pos <= cMaxDigitsInRepresentableFloatVar;
                 ++decimal_point_pos)
            {
                // Create a value for the `digits` property that has a certain number of digits
                std::conditional_t<
                        std::is_same_v<TestType, four_byte_encoded_variable_t>,
                        uint32_t,
                        uint64_t>
                        digits = 0;
                for (size_t i = 0; i < num_digits_in_digits_property; ++i) {
                    digits = digits * 10 + 9;
                }
                std::conditional_t<
                        std::is_same_v<TestType, four_byte_encoded_variable_t>,
                        uint32_t,
                        uint64_t>
                        cEncodedFloatDigitsBitMask
                        = std::is_same_v<TestType, four_byte_encoded_variable_t>
                                  ? clp::ffi::cFourByteEncodedFloatDigitsBitMask
                                  : clp::ffi::cEightByteEncodedFloatDigitsBitMask;
                // Due to the bitmask, the number of digits encoded may be less than
                // num_digits_in_digits_property
                digits = std::min(cEncodedFloatDigitsBitMask, digits);
                auto num_digits_in_value
                        = std::min(num_digits_in_digits_property, std::to_string(digits).length());

                // Iterate over the possible values for the encoded float's high bits
                uint8_t num_high_bits
                        = std::is_same_v<TestType, four_byte_encoded_variable_t> ? 1 : 2;
                for (size_t high_bits = 0; high_bits < num_high_bits; ++high_bits) {
                    TestType test_encoded_var;
                    if (std::is_same_v<TestType, eight_byte_encoded_variable_t>) {
                        test_encoded_var = encode_float_properties<TestType>(
                                high_bits & 0x2,
                                digits,
                                num_digits,
                                decimal_point_pos
                        );
                        // Since encode_float_properties erases the low bit of high_bits, we need to
                        // add it again manually
                        test_encoded_var
                                = (high_bits << 62) | (((1ULL << 62) - 1) & test_encoded_var);
                    } else {
                        test_encoded_var = encode_float_properties<TestType>(
                                high_bits,
                                digits,
                                num_digits,
                                decimal_point_pos
                        );
                    }

                    INFO("high_bits: " << high_bits);
                    INFO("decimal_point_pos: " << decimal_point_pos);
                    INFO("num_digits: " << num_digits);
                    INFO("num_digits_in_value: " << num_digits_in_value);
                    INFO("digits: " << digits);
                    if (decimal_point_pos <= num_digits && num_digits >= num_digits_in_value
                        && num_digits_in_value <= cMaxDigitsInRepresentableFloatVar)
                    {
                        REQUIRE_NOTHROW(decode_float_var(test_encoded_var));
                    } else {
                        REQUIRE_THROWS_AS(
                                decode_float_var(test_encoded_var),
                                clp::ffi::EncodingException
                        );
                    }
                }
            }
        }
    }
}

TEMPLATE_TEST_CASE(
        "Encoding messages",
        "[ffi][encode-message]",
        eight_byte_encoded_variable_t,
        four_byte_encoded_variable_t
) {
    string message;

    // Test encoding
    string logtype;
    vector<TestType> encoded_vars;
    vector<int32_t> dictionary_var_bounds;
    vector<string> var_strs
            = {"4938",
               std::to_string(INT32_MAX),
               std::to_string(INT64_MAX),
               "0.1",
               "-25.519686",
               "-25.5196868642755",
               "-00.00",
               "bin/python2.7.3",
               "abc123"};
    size_t var_ix = 0;
    message = "here is a string with a small int " + var_strs[var_ix++];
    message += " and a medium int " + var_strs[var_ix++];
    message += " and a very large int " + var_strs[var_ix++];
    message += " and a small float " + var_strs[var_ix++];
    message += " and a medium float " + var_strs[var_ix++];
    message += " and a weird float " + var_strs[var_ix++];
    message += " and a string with numbers " + var_strs[var_ix++];
    message += " and another string with numbers " + var_strs[var_ix++];
    REQUIRE(encode_message(message, logtype, encoded_vars, dictionary_var_bounds));

    // Concatenate all dictionary variables
    size_t all_dictionary_vars_length = 0;
    size_t num_dictionary_vars = 0;
    for (auto current = dictionary_var_bounds.cbegin(); dictionary_var_bounds.cend() != current;) {
        auto begin_pos = *current;
        ++current;
        auto end_pos = *current;
        ++current;
        all_dictionary_vars_length += end_pos - begin_pos;
        ++num_dictionary_vars;
    }
    string all_dictionary_vars;
    all_dictionary_vars.reserve(all_dictionary_vars_length);
    vector<int32_t> dictionary_var_end_offsets;
    dictionary_var_end_offsets.reserve(num_dictionary_vars);
    for (auto current = dictionary_var_bounds.cbegin(); dictionary_var_bounds.cend() != current;) {
        auto begin_pos = *current;
        ++current;
        auto end_pos = *current;
        ++current;
        all_dictionary_vars.append(message.data() + begin_pos, message.data() + end_pos);
        dictionary_var_end_offsets.push_back(all_dictionary_vars.length());
    }

    // Test decoding
    auto decoded_message = decode_message(
            logtype,
            encoded_vars.data(),
            encoded_vars.size(),
            all_dictionary_vars,
            dictionary_var_end_offsets.data(),
            dictionary_var_end_offsets.size()
    );
    REQUIRE(decoded_message == message);

    // Test encoding a message with a variable placeholder after the variables
    message = " test var123 ";
    message += enum_to_underlying_type(VariablePlaceholder::Integer);
    REQUIRE(encode_message(message, logtype, encoded_vars, dictionary_var_bounds));

    // Test encoding a message with a variable placeholder before a variable
    message += " var234";
    REQUIRE(encode_message(message, logtype, encoded_vars, dictionary_var_bounds));
}

TEMPLATE_TEST_CASE(
        "wildcard_query_matches_any_encoded_var",
        "[ffi][wildcard_query_matches_any_encoded_var]",
        eight_byte_encoded_variable_t,
        four_byte_encoded_variable_t
) {
    string message = "Static text, dictVar1, 123, 456.7, dictVar2, 987, 654.3";

    // Encode
    string logtype;
    vector<eight_byte_encoded_variable_t> encoded_vars;
    vector<int32_t> dictionary_var_bounds;
    REQUIRE(encode_message(message, logtype, encoded_vars, dictionary_var_bounds));

    // Test wildcard matches
    REQUIRE(wildcard_query_matches_any_encoded_var<VariablePlaceholder::Integer>(
            "1*3",
            logtype,
            encoded_vars.data(),
            encoded_vars.size()
    ));
    REQUIRE(false
            == wildcard_query_matches_any_encoded_var<VariablePlaceholder::Integer>(
                    "4*7",
                    logtype,
                    encoded_vars.data(),
                    encoded_vars.size()
            ));
    REQUIRE(wildcard_query_matches_any_encoded_var<VariablePlaceholder::Float>(
            "4*7",
            logtype,
            encoded_vars.data(),
            encoded_vars.size()
    ));
    REQUIRE(false
            == wildcard_query_matches_any_encoded_var<VariablePlaceholder::Float>(
                    "1*3",
                    logtype,
                    encoded_vars.data(),
                    encoded_vars.size()
            ));
}

TEMPLATE_TEST_CASE(
        "wildcard_match_encoded_vars",
        "[ffi][wildcard_match_encoded_vars]",
        eight_byte_encoded_variable_t,
        four_byte_encoded_variable_t
) {
    string message = "Static text, dictVar1, 123, 456.7, dictVar2, 987, 654.3";

    // Encode a message
    string logtype;
    vector<TestType> encoded_vars;
    vector<int32_t> dictionary_var_bounds;
    REQUIRE(encode_message(message, logtype, encoded_vars, dictionary_var_bounds));

    string wildcard_var_types;
    vector<string> wildcard_var_queries;
    vector<string_view> wildcard_var_query_views;

    SECTION("Fewer wildcard variables than encoded variables") {
        wildcard_var_queries.emplace_back("*123*");
        wildcard_var_types += enum_to_underlying_type(VariablePlaceholder::Integer);
        wildcard_var_queries.emplace_back("9*7");
        wildcard_var_types += enum_to_underlying_type(VariablePlaceholder::Integer);

        string_views_from_strings(wildcard_var_queries, wildcard_var_query_views);

        REQUIRE(wildcard_match_encoded_vars(
                logtype,
                encoded_vars.data(),
                encoded_vars.size(),
                wildcard_var_types,
                wildcard_var_query_views
        ));
    }

    SECTION("Same number of wildcard variables and encoded variables") {
        wildcard_var_queries.emplace_back("*123*");
        wildcard_var_types += enum_to_underlying_type(VariablePlaceholder::Integer);
        wildcard_var_queries.emplace_back("4*7");
        wildcard_var_types += enum_to_underlying_type(VariablePlaceholder::Float);
        wildcard_var_queries.emplace_back("9*7");
        wildcard_var_types += enum_to_underlying_type(VariablePlaceholder::Integer);
        wildcard_var_queries.emplace_back("*654.3*");
        wildcard_var_types += enum_to_underlying_type(VariablePlaceholder::Float);

        string_views_from_strings(wildcard_var_queries, wildcard_var_query_views);

        REQUIRE(wildcard_match_encoded_vars(
                logtype,
                encoded_vars.data(),
                encoded_vars.size(),
                wildcard_var_types,
                wildcard_var_query_views
        ));
    }

    SECTION("More wildcard variables than encoded variables") {
        wildcard_var_queries.emplace_back("*123*");
        wildcard_var_types += enum_to_underlying_type(VariablePlaceholder::Integer);
        wildcard_var_queries.emplace_back("4*7");
        wildcard_var_types += enum_to_underlying_type(VariablePlaceholder::Float);
        wildcard_var_queries.emplace_back("9*7");
        wildcard_var_types += enum_to_underlying_type(VariablePlaceholder::Integer);
        wildcard_var_queries.emplace_back("*654.3*");
        wildcard_var_types += enum_to_underlying_type(VariablePlaceholder::Float);
        wildcard_var_queries.emplace_back("*123*");
        wildcard_var_types += enum_to_underlying_type(VariablePlaceholder::Integer);

        string_views_from_strings(wildcard_var_queries, wildcard_var_query_views);

        REQUIRE(false
                == wildcard_match_encoded_vars(
                        logtype,
                        encoded_vars.data(),
                        encoded_vars.size(),
                        wildcard_var_types,
                        wildcard_var_query_views
                ));
    }
}

static void
string_views_from_strings(vector<string> const& strings, vector<string_view>& string_views) {
    string_views.reserve(strings.size());
    for (auto const& s : strings) {
        string_views.emplace_back(s);
    }
}
