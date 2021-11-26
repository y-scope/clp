// C libraries
#include <unistd.h>

// Catch2
#include "../submodules/Catch2/single_include/catch2/catch.hpp"

// Project headers
#include "../src/EncodedVariableInterpreter.hpp"
#include "../src/streaming_archive/Constants.hpp"

using std::string;
using std::to_string;
using std::vector;

TEST_CASE("EncodedVariableInterpreter", "[EncodedVariableInterpreter]") {
    SECTION("Test range of variable dictionary IDs") {
        // Ensure range of variable dictionary IDs goes from begin to end and is not empty
        REQUIRE(EncodedVariableInterpreter::get_var_dict_id_range_begin() < EncodedVariableInterpreter::get_var_dict_id_range_end());
    }

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
        encoded_variable_t representable_int_range_begin = EncodedVariableInterpreter::get_var_dict_id_range_begin() - 1;
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

        value = to_string(EncodedVariableInterpreter::get_var_dict_id_range_begin());
        REQUIRE(!EncodedVariableInterpreter::convert_string_to_representable_integer_var(value, encoded_var));

        value = to_string(EncodedVariableInterpreter::get_var_dict_id_range_end());
        REQUIRE(!EncodedVariableInterpreter::convert_string_to_representable_integer_var(value, encoded_var));
    }

    SECTION("Test convert_string_to_representable_double_var") {
        string value;
        encoded_variable_t encoded_var;
        double var_as_double;

        // Test basic conversions
        value = "0.0";
        REQUIRE(EncodedVariableInterpreter::convert_string_to_representable_double_var(value, encoded_var));
        EncodedVariableInterpreter::convert_encoded_double_to_string(encoded_var, value);
        REQUIRE("0.0" == value);

        value = "-1.0";
        REQUIRE(EncodedVariableInterpreter::convert_string_to_representable_double_var(value, encoded_var));
        EncodedVariableInterpreter::convert_encoded_double_to_string(encoded_var, value);
        REQUIRE("-1.0" == value);

        value = "1.0";
        REQUIRE(EncodedVariableInterpreter::convert_string_to_representable_double_var(value, encoded_var));
        EncodedVariableInterpreter::convert_encoded_double_to_string(encoded_var, value);
        REQUIRE("1.0" == value);

        value = ".1";
        REQUIRE(EncodedVariableInterpreter::convert_string_to_representable_double_var(value, encoded_var));
        EncodedVariableInterpreter::convert_encoded_double_to_string(encoded_var, value);
        REQUIRE(".1" == value);

        value = "-00.00";
        REQUIRE(EncodedVariableInterpreter::convert_string_to_representable_double_var(value, encoded_var));
        EncodedVariableInterpreter::convert_encoded_double_to_string(encoded_var, value);
        REQUIRE("-00.00" == value);

        // Test edges of representable range
        value = "-999999999999999.9";
        REQUIRE(EncodedVariableInterpreter::convert_string_to_representable_double_var(value, encoded_var));
        EncodedVariableInterpreter::convert_encoded_double_to_string(encoded_var, value);
        REQUIRE("-999999999999999.9" == value);

        value = "-.9999999999999999";
        REQUIRE(EncodedVariableInterpreter::convert_string_to_representable_double_var(value, encoded_var));
        EncodedVariableInterpreter::convert_encoded_double_to_string(encoded_var, value);
        REQUIRE("-.9999999999999999" == value);

        value = ".9999999999999999";
        REQUIRE(EncodedVariableInterpreter::convert_string_to_representable_double_var(value, encoded_var));
        EncodedVariableInterpreter::convert_encoded_double_to_string(encoded_var, value);
        REQUIRE(".9999999999999999" == value);

        // Test non-doubles
        value = "";
        REQUIRE(!EncodedVariableInterpreter::convert_string_to_representable_double_var(value, encoded_var));

        value = "a";
        REQUIRE(!EncodedVariableInterpreter::convert_string_to_representable_double_var(value, encoded_var));

        value = "-";
        REQUIRE(!EncodedVariableInterpreter::convert_string_to_representable_double_var(value, encoded_var));

        value = "+";
        REQUIRE(!EncodedVariableInterpreter::convert_string_to_representable_double_var(value, encoded_var));

        value = "-a";
        REQUIRE(!EncodedVariableInterpreter::convert_string_to_representable_double_var(value, encoded_var));

        value = "+a";
        REQUIRE(!EncodedVariableInterpreter::convert_string_to_representable_double_var(value, encoded_var));

        value = "--";
        REQUIRE(!EncodedVariableInterpreter::convert_string_to_representable_double_var(value, encoded_var));

        value = "++";
        REQUIRE(!EncodedVariableInterpreter::convert_string_to_representable_double_var(value, encoded_var));

        // Test unrepresentable values
        value = ".";
        REQUIRE(!EncodedVariableInterpreter::convert_string_to_representable_double_var(value, encoded_var));

        value = "1.";
        REQUIRE(!EncodedVariableInterpreter::convert_string_to_representable_double_var(value, encoded_var));

        value = " 1.0";
        REQUIRE(!EncodedVariableInterpreter::convert_string_to_representable_double_var(value, encoded_var));

        value = "- 1.0";
        REQUIRE(!EncodedVariableInterpreter::convert_string_to_representable_double_var(value, encoded_var));

        value = "1.0 ";
        REQUIRE(!EncodedVariableInterpreter::convert_string_to_representable_double_var(value, encoded_var));

        value = "+1.0";
        REQUIRE(!EncodedVariableInterpreter::convert_string_to_representable_double_var(value, encoded_var));

        value = "1.0f";
        REQUIRE(!EncodedVariableInterpreter::convert_string_to_representable_double_var(value, encoded_var));

        value = "1.0F";
        REQUIRE(!EncodedVariableInterpreter::convert_string_to_representable_double_var(value, encoded_var));

        value = "1.0l";
        REQUIRE(!EncodedVariableInterpreter::convert_string_to_representable_double_var(value, encoded_var));

        value = "1.0L";
        REQUIRE(!EncodedVariableInterpreter::convert_string_to_representable_double_var(value, encoded_var));

        value = to_string(EncodedVariableInterpreter::get_var_dict_id_range_begin());
        REQUIRE(!EncodedVariableInterpreter::convert_string_to_representable_double_var(value, encoded_var));

        value = to_string(EncodedVariableInterpreter::get_var_dict_id_range_end());
        REQUIRE(!EncodedVariableInterpreter::convert_string_to_representable_double_var(value, encoded_var));
    }

    SECTION("Test encoding and decoding") {
        string msg;

        const char cVarDictPath[] = "var.dict";
        const char cVarSegmentIndexPath[] = "var.segindex";

        // Open writer
        VariableDictionaryWriter var_dict_writer;
        var_dict_writer.open(cVarDictPath, cVarSegmentIndexPath,
                             EncodedVariableInterpreter::get_var_dict_id_range_end() - EncodedVariableInterpreter::get_var_dict_id_range_begin());

        // Test encoding
        vector<encoded_variable_t> encoded_vars;
        vector<variable_dictionary_id_t> added_var_ids;
        vector<string> var_strs = {"4938", to_string(EncodedVariableInterpreter::get_var_dict_id_range_begin()), "-25.5196868642755", "-00.00"};
        msg = "here is a string with a small int " + var_strs[0] + " and a very large int " + var_strs[1] + " and a double " + var_strs[2] +
              " and a weird double " + var_strs[3];
        LogTypeDictionaryEntry logtype_dict_entry;
        EncodedVariableInterpreter::encode_and_add_to_dictionary(msg, logtype_dict_entry, var_dict_writer, encoded_vars, added_var_ids);
        var_dict_writer.close();

        // Test added_var_ids is correctly populated
        size_t encoded_var_id_count = 0;
        for(const auto& var : encoded_vars) {
            if(EncodedVariableInterpreter::is_var_dict_id(var)){
                REQUIRE(added_var_ids.size() > encoded_var_id_count);
                REQUIRE(EncodedVariableInterpreter::decode_var_dict_id(var) == added_var_ids[encoded_var_id_count]);
            }
        }

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