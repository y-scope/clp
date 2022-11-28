// Catch2
#include "../submodules/Catch2/single_include/catch2/catch.hpp"

// Project headers
#include "../src/ffi/encoding_methods.hpp"

using ffi::decode_float_var;
using ffi::decode_integer_var;
using ffi::decode_message;
using ffi::encode_float_string;
using ffi::encode_integer_string;
using ffi::encode_message;
using ffi::get_bounds_of_next_var;
using ffi::VariablePlaceholder;
using ffi::wildcard_query_matches_any_encoded_var;
using std::string;
using std::vector;

TEST_CASE("ffi::get_bounds_of_next_var", "[ffi::get_bounds_of_next_var]") {
    string str;
    size_t begin_pos;
    size_t end_pos;
    bool contains_var_placeholder = false;

    // Corner cases
    // Empty string
    str = "";
    begin_pos = 0;
    end_pos = 0;
    REQUIRE(get_bounds_of_next_var(str, begin_pos, end_pos, contains_var_placeholder) == false);
    REQUIRE(false == contains_var_placeholder);

    // end_pos past the end of the string
    str = "abc";
    begin_pos = 0;
    end_pos = string::npos;
    REQUIRE(get_bounds_of_next_var(str, begin_pos, end_pos, contains_var_placeholder) == false);
    REQUIRE(false == contains_var_placeholder);

    // Non-variables
    str = "/";
    begin_pos = 0;
    end_pos = 0;
    REQUIRE(get_bounds_of_next_var(str, begin_pos, end_pos, contains_var_placeholder) == false);
    REQUIRE(str.length() == begin_pos);
    REQUIRE(false == contains_var_placeholder);

    str = "xyz";
    begin_pos = 0;
    end_pos = 0;
    REQUIRE(get_bounds_of_next_var(str, begin_pos, end_pos, contains_var_placeholder) == false);
    REQUIRE(str.length() == begin_pos);
    REQUIRE(false == contains_var_placeholder);

    str = "=";
    begin_pos = 0;
    end_pos = 0;
    REQUIRE(get_bounds_of_next_var(str, begin_pos, end_pos, contains_var_placeholder) == false);
    REQUIRE(str.length() == begin_pos);
    REQUIRE(false == contains_var_placeholder);

    // Variables
    str = "~=x!abc123;1.2%x:+394/-";
    begin_pos = 0;
    end_pos = 0;

    REQUIRE(get_bounds_of_next_var(str, begin_pos, end_pos, contains_var_placeholder) == true);
    REQUIRE("x" == str.substr(begin_pos, end_pos - begin_pos));
    REQUIRE(false == contains_var_placeholder);

    REQUIRE(get_bounds_of_next_var(str, begin_pos, end_pos, contains_var_placeholder) == true);
    REQUIRE("abc123" == str.substr(begin_pos, end_pos - begin_pos));
    REQUIRE(false == contains_var_placeholder);

    REQUIRE(get_bounds_of_next_var(str, begin_pos, end_pos, contains_var_placeholder) == true);
    REQUIRE("1.2" == str.substr(begin_pos, end_pos - begin_pos));
    REQUIRE(false == contains_var_placeholder);

    REQUIRE(get_bounds_of_next_var(str, begin_pos, end_pos, contains_var_placeholder) == true);
    REQUIRE("+394" == str.substr(begin_pos, end_pos - begin_pos));
    REQUIRE(false == contains_var_placeholder);

    REQUIRE(get_bounds_of_next_var(str, begin_pos, end_pos, contains_var_placeholder) == false);
    REQUIRE(false == contains_var_placeholder);

    str = " ad ff 95 24 0d ff ";
    begin_pos = 0;
    end_pos = 0;

    REQUIRE(get_bounds_of_next_var(str, begin_pos, end_pos, contains_var_placeholder) == true);
    REQUIRE("ad" == str.substr(begin_pos, end_pos - begin_pos));
    REQUIRE(false == contains_var_placeholder);

    REQUIRE(get_bounds_of_next_var(str, begin_pos, end_pos, contains_var_placeholder) == true);
    REQUIRE("ff" == str.substr(begin_pos, end_pos - begin_pos));
    REQUIRE(false == contains_var_placeholder);

    REQUIRE(get_bounds_of_next_var(str, begin_pos, end_pos, contains_var_placeholder) == true);
    REQUIRE("95" == str.substr(begin_pos, end_pos - begin_pos));
    REQUIRE(false == contains_var_placeholder);

    REQUIRE(get_bounds_of_next_var(str, begin_pos, end_pos, contains_var_placeholder) == true);
    REQUIRE("24" == str.substr(begin_pos, end_pos - begin_pos));
    REQUIRE(false == contains_var_placeholder);

    REQUIRE(get_bounds_of_next_var(str, begin_pos, end_pos, contains_var_placeholder) == true);
    REQUIRE("0d" == str.substr(begin_pos, end_pos - begin_pos));
    REQUIRE(false == contains_var_placeholder);

    REQUIRE(get_bounds_of_next_var(str, begin_pos, end_pos, contains_var_placeholder) == true);
    REQUIRE("ff" == str.substr(begin_pos, end_pos - begin_pos));
    REQUIRE(false == contains_var_placeholder);

    REQUIRE(get_bounds_of_next_var(str, begin_pos, end_pos, contains_var_placeholder) == false);
    REQUIRE(str.length() == begin_pos);
    REQUIRE(false == contains_var_placeholder);

    // String containing variable placeholder
    str = " text ";
    str += enum_to_underlying_type(VariablePlaceholder::Integer);
    str += " var123 ";
    begin_pos = 0;
    end_pos = 0;

    REQUIRE(get_bounds_of_next_var(str, begin_pos, end_pos, contains_var_placeholder) == true);
    REQUIRE(contains_var_placeholder);
    REQUIRE("var123" == str.substr(begin_pos, end_pos - begin_pos));
}

TEST_CASE("Encoding integers", "[encode-integer]") {
    string value;
    string decoded_value;
    encoded_variable_t encoded_var;

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
    value = std::to_string(INT64_MAX);
    REQUIRE(encode_integer_string(value, encoded_var));
    decoded_value = decode_integer_var(encoded_var);
    REQUIRE(decoded_value == value);

    value = std::to_string(INT64_MIN);
    REQUIRE(encode_integer_string(value, encoded_var));
    decoded_value = decode_integer_var(encoded_var);
    REQUIRE(decoded_value == value);

    value = "9223372036854775808";  // INT64_MAX + 1 == 2^63
    REQUIRE(false == encode_integer_string(value, encoded_var));

    value = "-9223372036854775809";  // INT64_MIN - 1 == -2^63 - 1
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

TEST_CASE("Encoding floats", "[encode-float]") {
    string value;
    string decoded_value;
    encoded_variable_t encoded_var;

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
    value = "-999999999999999.9";
    REQUIRE(encode_float_string(value, encoded_var));
    decoded_value = decode_float_var(encoded_var);
    REQUIRE(decoded_value == value);

    value = "-.9999999999999999";
    REQUIRE(encode_float_string(value, encoded_var));
    decoded_value = decode_float_var(encoded_var);
    REQUIRE(decoded_value == value);

    value = ".9999999999999999";
    REQUIRE(encode_float_string(value, encoded_var));
    decoded_value = decode_float_var(encoded_var);
    REQUIRE(decoded_value == value);

    // Test non-doubles
    value = "";
    REQUIRE(!encode_float_string(value, encoded_var));

    value = "a";
    REQUIRE(!encode_float_string(value, encoded_var));

    value = "-";
    REQUIRE(!encode_float_string(value, encoded_var));

    value = "+";
    REQUIRE(!encode_float_string(value, encoded_var));

    value = "-a";
    REQUIRE(!encode_float_string(value, encoded_var));

    value = "+a";
    REQUIRE(!encode_float_string(value, encoded_var));

    value = "--";
    REQUIRE(!encode_float_string(value, encoded_var));

    value = "++";
    REQUIRE(!encode_float_string(value, encoded_var));

    // Test unrepresentable values
    value = ".";
    REQUIRE(!encode_float_string(value, encoded_var));

    value = "1.";
    REQUIRE(!encode_float_string(value, encoded_var));

    value = " 1.0";
    REQUIRE(!encode_float_string(value, encoded_var));

    value = "- 1.0";
    REQUIRE(!encode_float_string(value, encoded_var));

    value = "1.0 ";
    REQUIRE(!encode_float_string(value, encoded_var));

    value = "+1.0";
    REQUIRE(!encode_float_string(value, encoded_var));

    value = "1.0f";
    REQUIRE(!encode_float_string(value, encoded_var));

    value = "1.0F";
    REQUIRE(!encode_float_string(value, encoded_var));

    value = "1.0l";
    REQUIRE(!encode_float_string(value, encoded_var));

    value = "1.0L";
    REQUIRE(!encode_float_string(value, encoded_var));
}

TEST_CASE("Encoding messages", "[encode-message]") {
    string message;

    // Test encoding
    string logtype;
    vector<encoded_variable_t> encoded_vars;
    vector<int32_t> dictionary_var_bounds;
    vector<string> var_strs = {"4938", std::to_string(INT64_MAX), "-25.5196868642755", "-00.00",
                               "bin/python2.7.3"};
    message = "here is a string with a small int " + var_strs[0] + " and a very large int " +
              var_strs[1] + " and a double " + var_strs[2] + " and a weird double " + var_strs[3] +
              " and a str with numbers " + var_strs[4];
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
        dictionary_var_end_offsets.push_back(end_pos);
    }

    // Test decoding
    auto decoded_message = decode_message(logtype, encoded_vars.data(), encoded_vars.size(),
                                          all_dictionary_vars, dictionary_var_end_offsets.data(),
                                          dictionary_var_end_offsets.size());
    REQUIRE(decoded_message == message);


    // Test encoding a message with a variable placeholder after the variables
    message = " test var123 ";
    message += enum_to_underlying_type(VariablePlaceholder::Integer);
    REQUIRE(false == encode_message(message, logtype, encoded_vars, dictionary_var_bounds));

    // Test encoding a message with a variable placeholder before a variable
    message += " var234";
    REQUIRE(false == encode_message(message, logtype, encoded_vars, dictionary_var_bounds));
}

TEST_CASE("ffi::wildcard_query_matches_any_encoded_var",
          "[ffi::wildcard_query_matches_any_encoded_var]")
{
    string message = "Static text, dictVar1, 123, 456.7, dictVar2, 987, 654.3";

    // Encode
    string logtype;
    vector<encoded_variable_t> encoded_vars;
    vector<int32_t> dictionary_var_bounds;
    REQUIRE(encode_message(message, logtype, encoded_vars, dictionary_var_bounds));

    // Test wildcard matches
    REQUIRE(wildcard_query_matches_any_encoded_var<VariablePlaceholder::Integer>(
            "1*3", logtype, encoded_vars.data(), encoded_vars.size()));
    REQUIRE(false == wildcard_query_matches_any_encoded_var<VariablePlaceholder::Integer>(
            "4*7", logtype, encoded_vars.data(), encoded_vars.size()));
    REQUIRE(wildcard_query_matches_any_encoded_var<VariablePlaceholder::Float>(
            "4*7", logtype, encoded_vars.data(), encoded_vars.size()));
    REQUIRE(false == wildcard_query_matches_any_encoded_var<VariablePlaceholder::Float>(
            "1*3", logtype, encoded_vars.data(), encoded_vars.size()));
}
