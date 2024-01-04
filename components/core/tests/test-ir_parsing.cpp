#include <Catch2/single_include/catch2/catch.hpp>

#include "../src/clp/ir/parsing.hpp"
#include "../src/clp/ir/types.hpp"
#include "../src/clp/type_utils.hpp"

using clp::ir::get_bounds_of_next_var;
using std::string;
using std::string_view;
using std::vector;

TEST_CASE("ir::get_bounds_of_next_var", "[ir][get_bounds_of_next_var]") {
    string str;
    size_t begin_pos;
    size_t end_pos;

    // Corner cases
    // Empty string
    str = "";
    begin_pos = 0;
    end_pos = 0;
    REQUIRE(get_bounds_of_next_var(str, begin_pos, end_pos) == false);

    // end_pos past the end of the string
    str = "abc";
    begin_pos = 0;
    end_pos = string::npos;
    REQUIRE(get_bounds_of_next_var(str, begin_pos, end_pos) == false);

    // Non-variables
    str = "/";
    begin_pos = 0;
    end_pos = 0;
    REQUIRE(get_bounds_of_next_var(str, begin_pos, end_pos) == false);
    REQUIRE(str.length() == begin_pos);

    str = "xyz";
    begin_pos = 0;
    end_pos = 0;
    REQUIRE(get_bounds_of_next_var(str, begin_pos, end_pos) == false);
    REQUIRE(str.length() == begin_pos);

    str = "=";
    begin_pos = 0;
    end_pos = 0;
    REQUIRE(get_bounds_of_next_var(str, begin_pos, end_pos) == false);
    REQUIRE(str.length() == begin_pos);

    // Variables
    str = "~=x!abc123;1.2%x:+394/-";
    begin_pos = 0;
    end_pos = 0;

    REQUIRE(get_bounds_of_next_var(str, begin_pos, end_pos) == true);
    REQUIRE("x" == str.substr(begin_pos, end_pos - begin_pos));

    REQUIRE(get_bounds_of_next_var(str, begin_pos, end_pos) == true);
    REQUIRE("abc123" == str.substr(begin_pos, end_pos - begin_pos));

    REQUIRE(get_bounds_of_next_var(str, begin_pos, end_pos) == true);
    REQUIRE("1.2" == str.substr(begin_pos, end_pos - begin_pos));

    REQUIRE(get_bounds_of_next_var(str, begin_pos, end_pos) == true);
    REQUIRE("+394" == str.substr(begin_pos, end_pos - begin_pos));

    REQUIRE(get_bounds_of_next_var(str, begin_pos, end_pos) == false);

    str = " ad ff 95 24 0d ff ";
    begin_pos = 0;
    end_pos = 0;

    REQUIRE(get_bounds_of_next_var(str, begin_pos, end_pos) == true);
    REQUIRE("ad" == str.substr(begin_pos, end_pos - begin_pos));

    REQUIRE(get_bounds_of_next_var(str, begin_pos, end_pos) == true);
    REQUIRE("ff" == str.substr(begin_pos, end_pos - begin_pos));

    REQUIRE(get_bounds_of_next_var(str, begin_pos, end_pos) == true);
    REQUIRE("95" == str.substr(begin_pos, end_pos - begin_pos));

    REQUIRE(get_bounds_of_next_var(str, begin_pos, end_pos) == true);
    REQUIRE("24" == str.substr(begin_pos, end_pos - begin_pos));

    REQUIRE(get_bounds_of_next_var(str, begin_pos, end_pos) == true);
    REQUIRE("0d" == str.substr(begin_pos, end_pos - begin_pos));

    REQUIRE(get_bounds_of_next_var(str, begin_pos, end_pos) == true);
    REQUIRE("ff" == str.substr(begin_pos, end_pos - begin_pos));

    REQUIRE(get_bounds_of_next_var(str, begin_pos, end_pos) == false);
    REQUIRE(str.length() == begin_pos);

    // String containing variable placeholder
    str = " text ";
    str += clp::enum_to_underlying_type(clp::ir::VariablePlaceholder::Integer);
    str += " var123 ";
    begin_pos = 0;
    end_pos = 0;

    REQUIRE(get_bounds_of_next_var(str, begin_pos, end_pos) == true);
    REQUIRE("var123" == str.substr(begin_pos, end_pos - begin_pos));
}
