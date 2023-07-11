// C libaries
#include <sys/stat.h>
#include <unistd.h>

// C++ libraries
#include <boost/foreach.hpp>
#include <boost/range/combine.hpp>
#include <chrono>
#include <ctime>
#include <iostream>
#include <ratio>

// Catch2
#include <Catch2/single_include/catch2/catch.hpp>

// Project headers
#include "../src/Utils.hpp"

using namespace std;

TEST_CASE("create_directory_structure", "[create_directory_structure]") {
    struct stat s = {};
    string path;

    path = "a/b/c";
    REQUIRE(ErrorCode_Success == create_directory_structure(path, 0700));
    REQUIRE(stat(path.c_str(), &s) == 0);

    path = "d/e/f/";
    REQUIRE(ErrorCode_Success == create_directory_structure(path, 0700));
    REQUIRE(stat(path.c_str(), &s) == 0);

    path = "/tmp/5807";
    REQUIRE(ErrorCode_Success == create_directory_structure(path, 0700));
    REQUIRE(stat(path.c_str(), &s) == 0);

    REQUIRE(0 == rmdir("a/b/c"));
    REQUIRE(0 == rmdir("a/b"));
    REQUIRE(0 == rmdir("a"));

    REQUIRE(0 == rmdir("d/e/f"));
    REQUIRE(0 == rmdir("d/e"));
    REQUIRE(0 == rmdir("d"));

    REQUIRE(0 == rmdir("/tmp/5807"));
}

TEST_CASE("get_parent_directory_path", "[get_parent_directory_path]") {
    // Corner cases
    // Anything without a slash should return "."
    REQUIRE(get_parent_directory_path(".") == ".");
    REQUIRE(get_parent_directory_path("..") == ".");
    REQUIRE(get_parent_directory_path("abc") == ".");
    // A single slash, at the beginning, should always return "/"
    REQUIRE(get_parent_directory_path("/") == "/");
    REQUIRE(get_parent_directory_path("/.") == "/");
    REQUIRE(get_parent_directory_path("/..") == "/");
    REQUIRE(get_parent_directory_path("/abc") == "/");

    // Normal cases
    REQUIRE(get_parent_directory_path("//abc/./def//../def/.///") == "/abc");
}

TEST_CASE("get_unambiguous_path", "[get_unambiguous_path]") {
    // Base cases (should not modify anything)
    REQUIRE(get_unambiguous_path("/") == "/");
    REQUIRE(get_unambiguous_path("abc") == "abc");
    REQUIRE(get_unambiguous_path("/abc") == "/abc");
    REQUIRE(get_unambiguous_path("/abc/def") == "/abc/def");

    // Corner cases
    REQUIRE(get_unambiguous_path(".").empty());
    REQUIRE(get_unambiguous_path("..").empty());
    REQUIRE(get_unambiguous_path("////") == "/");
    REQUIRE(get_unambiguous_path("/./.././//../") == "/");
    REQUIRE(get_unambiguous_path("./.././//../").empty());
    REQUIRE(get_unambiguous_path("/abc/def/.././../") == "/");
    REQUIRE(get_unambiguous_path("abc/def/.././../").empty());

    // Normal cases
    REQUIRE(get_unambiguous_path("/abc///def/../ghi/./") == "/abc/ghi");
    REQUIRE(get_unambiguous_path("abc///def/../ghi/./") == "abc/ghi");
    REQUIRE(get_unambiguous_path("../abc///def/../ghi/./") == "abc/ghi");
}

TEST_CASE("get_bounds_of_next_var", "[get_bounds_of_next_var]") {
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
    REQUIRE("+394/-" == str.substr(begin_pos, end_pos - begin_pos));

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
}
