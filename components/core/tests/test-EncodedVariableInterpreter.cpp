// C libraries
#include <unistd.h>

// Catch2
#include <Catch2/single_include/catch2/catch.hpp>

// Project headers
#include "../src/EncodedVariableInterpreter.hpp"
#include "../src/streaming_archive/Constants.hpp"

using std::string;
using std::to_string;
using std::vector;

TEST_CASE("EncodedVariableInterpreter", "[EncodedVariableInterpreter]") {
    SECTION("Test convert_string_to_representable_integer_var") {
        string value;
        encoded_variable_t encoded_var;

        // Test basic conversions
        value = "0";
        REQUIRE(EncodedVariableInterpreter::convert_string_to_representable_integer_var(value, encoded_var));
        REQUIRE(0 == encoded_var);

        value = "-1";
        REQUIRE(EncodedVariableInterpreter::convert_string_to_representable_integer_var(value, encoded_var));
        REQUIRE(-1 == encoded_var);

        value = "1";
        REQUIRE(EncodedVariableInterpreter::convert_string_to_representable_integer_var(value, encoded_var));
        REQUIRE(1 == encoded_var);

        // Test edges of representable range
        encoded_variable_t representable_int_range_begin = INT64_MAX;
        value = to_string(representable_int_range_begin);
        REQUIRE(EncodedVariableInterpreter::convert_string_to_representable_integer_var(value, encoded_var));
        REQUIRE(representable_int_range_begin == encoded_var);

        encoded_variable_t representable_int_range_end = INT64_MIN;
        value = to_string(representable_int_range_end);
        REQUIRE(EncodedVariableInterpreter::convert_string_to_representable_integer_var(value, encoded_var));
        REQUIRE(representable_int_range_end == encoded_var);

        // Test non-integers
        value = "";
        REQUIRE(!EncodedVariableInterpreter::convert_string_to_representable_integer_var(value, encoded_var));

        value = "a";
        REQUIRE(!EncodedVariableInterpreter::convert_string_to_representable_integer_var(value, encoded_var));

        value = "-";
        REQUIRE(!EncodedVariableInterpreter::convert_string_to_representable_integer_var(value, encoded_var));

        value = "+";
        REQUIRE(!EncodedVariableInterpreter::convert_string_to_representable_integer_var(value, encoded_var));

        value = "-a";
        REQUIRE(!EncodedVariableInterpreter::convert_string_to_representable_integer_var(value, encoded_var));

        value = "+a";
        REQUIRE(!EncodedVariableInterpreter::convert_string_to_representable_integer_var(value, encoded_var));

        value = "--";
        REQUIRE(!EncodedVariableInterpreter::convert_string_to_representable_integer_var(value, encoded_var));

        value = "++";
        REQUIRE(!EncodedVariableInterpreter::convert_string_to_representable_integer_var(value, encoded_var));

        // Test unrepresentable values
        value = " 1";
        REQUIRE(!EncodedVariableInterpreter::convert_string_to_representable_integer_var(value, encoded_var));

        value = "- 1";
        REQUIRE(!EncodedVariableInterpreter::convert_string_to_representable_integer_var(value, encoded_var));

        value = "1 ";
        REQUIRE(!EncodedVariableInterpreter::convert_string_to_representable_integer_var(value, encoded_var));

        value = "01";
        REQUIRE(!EncodedVariableInterpreter::convert_string_to_representable_integer_var(value, encoded_var));

        value = "+1";
        REQUIRE(!EncodedVariableInterpreter::convert_string_to_representable_integer_var(value, encoded_var));

        value = "1u";
        REQUIRE(!EncodedVariableInterpreter::convert_string_to_representable_integer_var(value, encoded_var));

        value = "1U";
        REQUIRE(!EncodedVariableInterpreter::convert_string_to_representable_integer_var(value, encoded_var));

        value = "1l";
        REQUIRE(!EncodedVariableInterpreter::convert_string_to_representable_integer_var(value, encoded_var));

        value = "1L";
        REQUIRE(!EncodedVariableInterpreter::convert_string_to_representable_integer_var(value, encoded_var));

        value = "1ll";
        REQUIRE(!EncodedVariableInterpreter::convert_string_to_representable_integer_var(value, encoded_var));

        value = "1LL";
        REQUIRE(!EncodedVariableInterpreter::convert_string_to_representable_integer_var(value, encoded_var));

        value = "0.0";
        REQUIRE(!EncodedVariableInterpreter::convert_string_to_representable_integer_var(value, encoded_var));

        value = "-0";
        REQUIRE(!EncodedVariableInterpreter::convert_string_to_representable_integer_var(value, encoded_var));
    }

    SECTION("Test convert_string_to_representable_float_var") {
        string value;
        encoded_variable_t encoded_var;
        double var_as_double;

        // Test basic conversions
        value = "0.0";
        REQUIRE(EncodedVariableInterpreter::convert_string_to_representable_float_var(value, encoded_var));
        EncodedVariableInterpreter::convert_encoded_float_to_string(encoded_var, value);
        REQUIRE("0.0" == value);

        value = "-1.0";
        REQUIRE(EncodedVariableInterpreter::convert_string_to_representable_float_var(value, encoded_var));
        EncodedVariableInterpreter::convert_encoded_float_to_string(encoded_var, value);
        REQUIRE("-1.0" == value);

        value = "1.0";
        REQUIRE(EncodedVariableInterpreter::convert_string_to_representable_float_var(value, encoded_var));
        EncodedVariableInterpreter::convert_encoded_float_to_string(encoded_var, value);
        REQUIRE("1.0" == value);

        value = ".1";
        REQUIRE(EncodedVariableInterpreter::convert_string_to_representable_float_var(value, encoded_var));
        EncodedVariableInterpreter::convert_encoded_float_to_string(encoded_var, value);
        REQUIRE(".1" == value);

        value = "-00.00";
        REQUIRE(EncodedVariableInterpreter::convert_string_to_representable_float_var(value, encoded_var));
        EncodedVariableInterpreter::convert_encoded_float_to_string(encoded_var, value);
        REQUIRE("-00.00" == value);

        // Test edges of representable range
        value = "-999999999999999.9";
        REQUIRE(EncodedVariableInterpreter::convert_string_to_representable_float_var(value, encoded_var));
        EncodedVariableInterpreter::convert_encoded_float_to_string(encoded_var, value);
        REQUIRE("-999999999999999.9" == value);

        value = "-.9999999999999999";
        REQUIRE(EncodedVariableInterpreter::convert_string_to_representable_float_var(value, encoded_var));
        EncodedVariableInterpreter::convert_encoded_float_to_string(encoded_var, value);
        REQUIRE("-.9999999999999999" == value);

        value = ".9999999999999999";
        REQUIRE(EncodedVariableInterpreter::convert_string_to_representable_float_var(value, encoded_var));
        EncodedVariableInterpreter::convert_encoded_float_to_string(encoded_var, value);
        REQUIRE(".9999999999999999" == value);

        // Test non-doubles
        value = "";
        REQUIRE(!EncodedVariableInterpreter::convert_string_to_representable_float_var(value, encoded_var));

        value = "a";
        REQUIRE(!EncodedVariableInterpreter::convert_string_to_representable_float_var(value, encoded_var));

        value = "-";
        REQUIRE(!EncodedVariableInterpreter::convert_string_to_representable_float_var(value, encoded_var));

        value = "+";
        REQUIRE(!EncodedVariableInterpreter::convert_string_to_representable_float_var(value, encoded_var));

        value = "-a";
        REQUIRE(!EncodedVariableInterpreter::convert_string_to_representable_float_var(value, encoded_var));

        value = "+a";
        REQUIRE(!EncodedVariableInterpreter::convert_string_to_representable_float_var(value, encoded_var));

        value = "--";
        REQUIRE(!EncodedVariableInterpreter::convert_string_to_representable_float_var(value, encoded_var));

        value = "++";
        REQUIRE(!EncodedVariableInterpreter::convert_string_to_representable_float_var(value, encoded_var));

        // Test unrepresentable values
        value = ".";
        REQUIRE(!EncodedVariableInterpreter::convert_string_to_representable_float_var(value, encoded_var));

        value = "1.";
        REQUIRE(!EncodedVariableInterpreter::convert_string_to_representable_float_var(value, encoded_var));

        value = " 1.0";
        REQUIRE(!EncodedVariableInterpreter::convert_string_to_representable_float_var(value, encoded_var));

        value = "- 1.0";
        REQUIRE(!EncodedVariableInterpreter::convert_string_to_representable_float_var(value, encoded_var));

        value = "1.0 ";
        REQUIRE(!EncodedVariableInterpreter::convert_string_to_representable_float_var(value, encoded_var));

        value = "+1.0";
        REQUIRE(!EncodedVariableInterpreter::convert_string_to_representable_float_var(value, encoded_var));

        value = "1.0f";
        REQUIRE(!EncodedVariableInterpreter::convert_string_to_representable_float_var(value, encoded_var));

        value = "1.0F";
        REQUIRE(!EncodedVariableInterpreter::convert_string_to_representable_float_var(value, encoded_var));

        value = "1.0l";
        REQUIRE(!EncodedVariableInterpreter::convert_string_to_representable_float_var(value, encoded_var));

        value = "1.0L";
        REQUIRE(!EncodedVariableInterpreter::convert_string_to_representable_float_var(value, encoded_var));

        value = to_string(UINT64_MAX);
        REQUIRE(!EncodedVariableInterpreter::convert_string_to_representable_float_var(value, encoded_var));
    }

    SECTION("Test encoding and decoding") {
        string msg;

        const char cVarDictPath[] = "var.dict";
        const char cVarSegmentIndexPath[] = "var.segindex";

        // Open writer
        VariableDictionaryWriter var_dict_writer;
        var_dict_writer.open(cVarDictPath, cVarSegmentIndexPath, cVariableDictionaryIdMax);

        // Test encoding
        vector<encoded_variable_t> encoded_vars;
        vector<variable_dictionary_id_t> var_ids;

        string large_val_str = to_string(cVariableDictionaryIdMax) + "0";
        vector<string> var_strs = {"4938", large_val_str, "-25.5196868642755",
                                   "-00.00", "bin/python2.7.3"};
        msg = "here is a string with a small int " + var_strs[0] +
                " and a very large int " + var_strs[1] +
                " and a double " + var_strs[2] +
                " and a weird double " + var_strs[3] +
                " and a str with numbers " + var_strs[4];

        LogTypeDictionaryEntry logtype_dict_entry;
        EncodedVariableInterpreter::encode_and_add_to_dictionary(msg, logtype_dict_entry,
                                                                 var_dict_writer, encoded_vars,
                                                                 var_ids);
        var_dict_writer.close();

        // Test var_ids is correctly populated
        size_t encoded_var_id_ix = 0;
        LogTypeDictionaryEntry::VarDelim var_placeholder;
        for (auto var_ix = 0; var_ix < logtype_dict_entry.get_num_vars(); var_ix++) {
            std::ignore = logtype_dict_entry.get_var_info(var_ix, var_placeholder);
            if (LogTypeDictionaryEntry::VarDelim::Dictionary == var_placeholder) {
                auto var = encoded_vars[var_ix];
                REQUIRE(var_ids.size() > encoded_var_id_ix);
                REQUIRE(EncodedVariableInterpreter::decode_var_dict_id(var) ==
                        var_ids[encoded_var_id_ix]);
                encoded_var_id_ix++;
            }
        }
        REQUIRE(var_ids.size() == encoded_var_id_ix);

        // Open reader
        VariableDictionaryReader var_dict_reader;
        var_dict_reader.open(cVarDictPath, cVarSegmentIndexPath);
        var_dict_reader.read_new_entries();

        // Test searching
        string search_logtype = "here is a string with a small int ";
        SubQuery sub_query;
        REQUIRE(EncodedVariableInterpreter::encode_and_search_dictionary(var_strs[0], var_dict_reader, false, search_logtype, sub_query));
        search_logtype += " and a very large int ";
        REQUIRE(EncodedVariableInterpreter::encode_and_search_dictionary(var_strs[1], var_dict_reader, false, search_logtype, sub_query));
        search_logtype += " and a double ";
        REQUIRE(EncodedVariableInterpreter::encode_and_search_dictionary(var_strs[2], var_dict_reader, false, search_logtype, sub_query));
        search_logtype += " and a weird double ";
        REQUIRE(EncodedVariableInterpreter::encode_and_search_dictionary(var_strs[3], var_dict_reader, false, search_logtype, sub_query));
        search_logtype += " and a str with numbers ";
        REQUIRE(EncodedVariableInterpreter::encode_and_search_dictionary(var_strs[4], var_dict_reader, false, search_logtype, sub_query));
        auto& vars = sub_query.get_vars();
        REQUIRE(vars.size() == encoded_vars.size());
        for (size_t i = 0; i < vars.size(); ++i) {
            REQUIRE(vars[i].matches(encoded_vars[i]));
        }

        // Test search for unknown variable
        REQUIRE(!EncodedVariableInterpreter::encode_and_search_dictionary("abc123", var_dict_reader, false, search_logtype, sub_query));

        REQUIRE(logtype_dict_entry.get_value() == search_logtype);

        // Test decoding
        string decompressed_msg;
        REQUIRE(EncodedVariableInterpreter::decode_variables_into_message(logtype_dict_entry, var_dict_reader, encoded_vars, decompressed_msg));
        REQUIRE(msg == decompressed_msg);

        var_dict_reader.close();

        // Clean-up
        int retval = unlink(cVarDictPath);
        REQUIRE(0 == retval);
        retval = unlink(cVarSegmentIndexPath);
        REQUIRE(0 == retval);
    }
}
