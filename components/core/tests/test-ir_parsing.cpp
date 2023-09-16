#include <Catch2/single_include/catch2/catch.hpp>

#include "../src/ir/parsing.hpp"
#include "../src/type_utils.hpp"

using ir::get_bounds_of_next_var;
using std::string;
using std::string_view;
using std::vector;

TEST_CASE("ir::get_bounds_of_next_var", "[ir][get_bounds_of_next_var]") {
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
    str += enum_to_underlying_type(ir::VariablePlaceholder::Integer);
    str += " var123 ";
    begin_pos = 0;
    end_pos = 0;

    REQUIRE(get_bounds_of_next_var(str, begin_pos, end_pos, contains_var_placeholder) == true);
    REQUIRE(contains_var_placeholder);
    REQUIRE("var123" == str.substr(begin_pos, end_pos - begin_pos));
}
