#include <sys/stat.h>
#include <unistd.h>

#include <chrono>
#include <ctime>
#include <iostream>
#include <ratio>

#include <boost/foreach.hpp>
#include <boost/range/combine.hpp>
#include <Catch2/single_include/catch2/catch.hpp>

#include "../src/clp/Utils.hpp"

using clp::create_directory_structure;
using clp::ErrorCode_Success;
using std::string;

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
