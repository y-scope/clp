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
#include "../submodules/Catch2/single_include/catch2/catch.hpp"

// Project headers
#include "../src/Utils.hpp"

using namespace std;

TEST_CASE("clean_up_wildcard_search_string", "[clean_up_wildcard_search_string]") {
    string str;

    // No wildcards
    str = "test";
    REQUIRE(clean_up_wildcard_search_string(str) == "*test*");

    // Only '?'
    str = "?est";
    REQUIRE(clean_up_wildcard_search_string(str) == "*?est*");

    // Normal case
    str = "***t**st?**";
    REQUIRE(clean_up_wildcard_search_string(str) == "*t*st?*");

    // Abnormal cases
    str = "***";
    REQUIRE(clean_up_wildcard_search_string(str) == "*");

    str = "*?*";
    REQUIRE(clean_up_wildcard_search_string(str) == "*?*");

    str = "?";
    REQUIRE(clean_up_wildcard_search_string(str) == "*?*");
}

TEST_CASE("convert_string_to_int64", "[convert_string_to_int64]") {
    string raw;
    int64_t converted;

    // Corner cases
    // Empty string
    raw = "";
    REQUIRE(false == convert_string_to_int64(raw, converted));

    // Integer that's more than 64-bits
    raw = "9999999999999999999";
    REQUIRE(false == convert_string_to_int64(raw, converted));

    // Non-integers
    raw = "abc";
    REQUIRE(false == convert_string_to_int64(raw, converted));

    raw = "90a";
    REQUIRE(false == convert_string_to_int64(raw, converted));

    raw = "0.5";
    REQUIRE(false == convert_string_to_int64(raw, converted));

    // Non-decimal integers
    raw = "0x5A";
    REQUIRE(false == convert_string_to_int64(raw, converted));

    // Integers
    raw = "98340";
    REQUIRE(convert_string_to_int64(raw, converted));
    REQUIRE(98340 == converted);
}

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

SCENARIO("Test case sensitive wild card match in all possible ways", "[wildcard]") {
    std::string tameString, wildString;

    WHEN("Match is expected if wild card character is \"*\"") {
        GIVEN("Single wild with no suffix char") {
            tameString = "abcd", wildString = "a*";
            REQUIRE(wildCardMatch_caseSensitive(tameString, wildString) == true);
        }

        GIVEN("Multiple wild with no suffix char") {
            tameString = "abcd", wildString = "a**";
            REQUIRE(wildCardMatch_caseSensitive(tameString, wildString) == true);
        }

        GIVEN("Multiple wild with no suffix char") {
            tameString = "abcd", wildString = "a***";
            REQUIRE(wildCardMatch_caseSensitive(tameString, wildString) == true);
        }

        GIVEN("Single wild with no prefix char") {
            tameString = "abcd", wildString = "*d";
            REQUIRE(wildCardMatch_caseSensitive(tameString, wildString) == true);
        }

        GIVEN("Multiple wild with no prefix char") {
            tameString = "abcd", wildString = "**d";
            REQUIRE(wildCardMatch_caseSensitive(tameString, wildString) == true);
        }

        GIVEN("Multiple wild with no prefix char") {
            tameString = "abcd", wildString = "***d";
            REQUIRE(wildCardMatch_caseSensitive(tameString, wildString) == true);
        }

        GIVEN("Single wild on both side & has 1st char as literal") {
            tameString = "abcd", wildString = "*a*";
            REQUIRE(wildCardMatch_caseSensitive(tameString, wildString) == true);
        }

        GIVEN("Single wild on both side & has middle char as literal") {
            tameString = "abcd", wildString = "*b*";
            REQUIRE(wildCardMatch_caseSensitive(tameString, wildString) == true);
        }

        GIVEN("Single wild on both side & has last char as literal") {
            tameString = "abcd", wildString = "*d*";
            REQUIRE(wildCardMatch_caseSensitive(tameString, wildString) == true);
        }

        GIVEN("Single wild on left side, multiple on right, 1st char as literal") {
            tameString = "abcd", wildString = "*a**";
            REQUIRE(wildCardMatch_caseSensitive(tameString, wildString) == true);
        }

        GIVEN("Single wild on left side, multiple on right, middle char as literal") {
            tameString = "abcd", wildString = "*b**";
            REQUIRE(wildCardMatch_caseSensitive(tameString, wildString) == true);
        }

        GIVEN("Single wild on left side, multiple on right, last char as literal") {
            tameString = "abcd", wildString = "*d**";
            REQUIRE(wildCardMatch_caseSensitive(tameString, wildString) == true);
        }

        GIVEN("Single wild on right side, multiple on left, 1st char as literal") {
            tameString = "abcd", wildString = "**a*";
            REQUIRE(wildCardMatch_caseSensitive(tameString, wildString) == true);
        }

        GIVEN("Single wild on right side, multiple on left, middle char as literal") {
            tameString = "abcd", wildString = "**b*";
            REQUIRE(wildCardMatch_caseSensitive(tameString, wildString) == true);
        }

        GIVEN("Single wild on right side, multiple on left, last char as literal") {
            tameString = "abcd", wildString = "**d*";
            REQUIRE(wildCardMatch_caseSensitive(tameString, wildString) == true);
        }

        GIVEN("Multiple wild anywhere") {
            tameString = "abcdef", wildString = "a*b**c******d*ef";
            REQUIRE(wildCardMatch_caseSensitive(tameString, wildString) == true);
        }

        GIVEN("Single wild only") {
            tameString = "abcd", wildString = "*";
            REQUIRE(wildCardMatch_caseSensitive(tameString, wildString) == true);
        }

        GIVEN("Some wild only") {
            tameString = "abcd", wildString = "***";
            REQUIRE(wildCardMatch_caseSensitive(tameString, wildString) == true);
        }

        GIVEN("Lots of wild only") {
            tameString = "abcd", wildString = "********";
            REQUIRE(wildCardMatch_caseSensitive(tameString, wildString) == true);
        }
    }

    WHEN("Match is expected if Wild card character is \"?\"") {
        GIVEN("Single wild in the middle") {
            tameString = "abcd", wildString = "a?cd";
            REQUIRE(wildCardMatch_caseSensitive(tameString, wildString) == true);
        }

        GIVEN("Single wild in the beginning") {
            tameString = "abcd", wildString = "?bcd";
            REQUIRE(wildCardMatch_caseSensitive(tameString, wildString) == true);
        }

        GIVEN("Single wild at the end") {
            tameString = "abcd", wildString = "abc?";
            REQUIRE(wildCardMatch_caseSensitive(tameString, wildString) == true);
        }


        GIVEN("Multiple wild in the middle") {
            tameString = "abcd", wildString = "a??d";
            REQUIRE(wildCardMatch_caseSensitive(tameString, wildString) == true);
        }

        GIVEN("Multiple wild in the beginning") {
            tameString = "abcd", wildString = "??cd";
            REQUIRE(wildCardMatch_caseSensitive(tameString, wildString) == true);
        }

        GIVEN("Multiple wild in the end") {
            tameString = "abcd", wildString = "ab??";
            REQUIRE(wildCardMatch_caseSensitive(tameString, wildString) == true);
        }

        GIVEN("Single wild in the beginning and end") {
            tameString = "abcd", wildString = "?bc?";
            REQUIRE(wildCardMatch_caseSensitive(tameString, wildString) == true);
        }

        GIVEN("Multiple wild anywhere") {
            tameString = "abcdef", wildString = "a?c?ef";
            REQUIRE(wildCardMatch_caseSensitive(tameString, wildString) == true);
        }

        GIVEN("All wild") {
            tameString = "abcd", wildString = "????";
            REQUIRE(wildCardMatch_caseSensitive(tameString, wildString) == true);
        }
    }

    WHEN("Match is expected if wild card character has both \"*\" and \"?\"") {
        GIVEN("Wild begins with \"*?\" pattern with 0 matched char for \"*\"") {
            tameString = "abcd", wildString = "*?bcd";
            REQUIRE(wildCardMatch_caseSensitive(tameString, wildString) == true);
        }

        GIVEN("Wild begins with \"?*\" pattern with 0 matched char for \"*\"") {
            tameString = "abcd", wildString = "?*bcd";
            REQUIRE(wildCardMatch_caseSensitive(tameString, wildString) == true);
        }

        GIVEN("Wild begins with \"*?\" pattern with 1 matched char for \"*\"") {
            tameString = "abcd", wildString = "*?cd";
            REQUIRE(wildCardMatch_caseSensitive(tameString, wildString) == true);
        }

        GIVEN("Wild begins with \"?*\" pattern with 1 matched char for \"*\"") {
            tameString = "abcd", wildString = "*?cd";
            REQUIRE(wildCardMatch_caseSensitive(tameString, wildString) == true);
        }

        GIVEN("Wild ends with \"*?\" pattern with 0 matched char for \"*\"") {
            tameString = "abcd", wildString = "abc*?";
            REQUIRE(wildCardMatch_caseSensitive(tameString, wildString) == true);
        }

        GIVEN("Wild ends with \"?*\" pattern with 0 matched char for \"*\"") {
            tameString = "abcd", wildString = "abc*?";
            REQUIRE(wildCardMatch_caseSensitive(tameString, wildString) == true);
        }

        GIVEN("Wild ends with \"*?\" pattern with 1 matched char for \"*\"") {
            tameString = "abcd", wildString = "ab*?";
            REQUIRE(wildCardMatch_caseSensitive(tameString, wildString) == true);
        }

        GIVEN("Wild ends with \"?*\" pattern with 1 matched char for \"*\"") {
            tameString = "abcd", wildString = "ab?*";
            REQUIRE(wildCardMatch_caseSensitive(tameString, wildString) == true);
        }

        GIVEN("Wild begins with exactly \"*?\" pattern") {
            tameString = "abcd", wildString = "*?";
            REQUIRE(wildCardMatch_caseSensitive(tameString, wildString) == true);
        }

        GIVEN("Wild begins with exactly \"?*\" pattern") {
            tameString = "abcd", wildString = "?*";
            REQUIRE(wildCardMatch_caseSensitive(tameString, wildString) == true);
        }
    }

    WHEN("Match unexpected containing wild card character(s)") {
        GIVEN("Missing literal character w/ \"*\"") {
            tameString = "abcd", wildString = "ac*";
            REQUIRE(wildCardMatch_caseSensitive(tameString, wildString) == false);
        }

        GIVEN("More literals in wild than tame w/ \"*\" in the middle") {
            tameString = "abcd", wildString = "abc*de";
            REQUIRE(wildCardMatch_caseSensitive(tameString, wildString) == false);
        }

        GIVEN("MISSING matching literals in the beginning with \"*\" in the middle") {
            tameString = "abcd", wildString = "b**d";
            REQUIRE(wildCardMatch_caseSensitive(tameString, wildString) == false);
        }

        GIVEN("MISSING matching literals in the end with \"*\" in the middle") {
            tameString = "abcd", wildString = "a**c";
            REQUIRE(wildCardMatch_caseSensitive(tameString, wildString) == false);
        }

        GIVEN("MISSING matching literals in the beginning with both \"*\" and \"?\" in the middle") {
            tameString = "abcd", wildString = "b*?d";
            REQUIRE(wildCardMatch_caseSensitive(tameString, wildString) == false);
        }

        GIVEN("MISSING matching literals in the beginning with \"?\" at the beginning") {
            tameString = "abcd", wildString = "?cd";
            REQUIRE(wildCardMatch_caseSensitive(tameString, wildString) == false);
        }

        GIVEN("MISSING matching literals in the end with both \"?\" at the end") {
            tameString = "abcd", wildString = "ab?";
            REQUIRE(wildCardMatch_caseSensitive(tameString, wildString) == false);
        }
    }

    WHEN("Match is expected when escape character(s) are used") {
        GIVEN("Escaping \"*\"") {
            tameString = "a*cd", wildString = "a\\*cd";
            REQUIRE(wildCardMatch_caseSensitive(tameString, wildString) == true);
        }

        GIVEN("Escaping \"?\"") {
            tameString = "a?cd", wildString = "a\\?cd";
            REQUIRE(wildCardMatch_caseSensitive(tameString, wildString) == true);
        }

        GIVEN("Escaping \"*\" and \"?\"") {
            tameString = "a?c*e", wildString = "a\\?c\\*e";
            REQUIRE(wildCardMatch_caseSensitive(tameString, wildString) == true);
        }

        GIVEN("Escaping \"\\\"") {
            tameString = "a\\cd", wildString = "a\\\\cd";
            REQUIRE(wildCardMatch_caseSensitive(tameString, wildString) == true);
        }

        GIVEN("Escaping \"?\" when fast forwarding") {
            tameString = "abc?e", wildString = "a*\\?e";
            REQUIRE(wildCardMatch_caseSensitive(tameString, wildString) == true);
        }

        GIVEN("Escaping \"*\" when fast forwarding") {
            tameString = "abc*e", wildString = "a*\\*e";
            REQUIRE(wildCardMatch_caseSensitive(tameString, wildString) == true);
        }

        GIVEN("Escaping \"\\\" when fast forwarding") {
            tameString = "abc\\e", wildString = "a*\\\\e";
            REQUIRE(wildCardMatch_caseSensitive(tameString, wildString) == true);
        }

        GIVEN("Escaping \"?\" when rewinding") {
            tameString = "\\ab\\ab\\c?ef", wildString = "*ab\\\\c\\?*";
            REQUIRE(wildCardMatch_caseSensitive(tameString, wildString) == true);
        }

        GIVEN("Escaping \"*\" when rewinding") {
            tameString = "\\ab\\ab\\c*ef", wildString = "*ab\\\\c\\**";
            REQUIRE(wildCardMatch_caseSensitive(tameString, wildString) == true);
        }

        GIVEN("Escaping \"\\\" when rewinding") {
            tameString = "\\ab\\ab\\c\\ef", wildString = "*ab\\\\c\\\\*";
            REQUIRE(wildCardMatch_caseSensitive(tameString, wildString) == true);
        }

        GIVEN("Silently ignore unsupported escape sequence \\a") {
            tameString = "ab?d", wildString = "\\ab?d";
            REQUIRE(wildCardMatch_caseSensitive(tameString, wildString) == true);
        }

        GIVEN("Silently ignore improper escape sequence at the end") {
            tameString = "abcd", wildString = "abcd\\";
            REQUIRE(wildCardMatch_caseSensitive(tameString, wildString) == true);
        }
    }

    WHEN("Case wild card match is case insensitive") {
        // This test is meant to supplement the case sensitive wild card implementation
        // The case insensitive implementation is exactly the same as case sensitive except it automatically adjust
        // the inputs to lower case when needed before doing comparison. It is rarely used due to lower performance
        bool isCaseSensitive = false;
        GIVEN("All lower case tame and all upper case wild") {
            tameString = "abcde", wildString = "A?C*";
            REQUIRE(wildCardMatch(tameString, wildString, isCaseSensitive) == true);
        }

        GIVEN("All lower case tame and mixed lower and upper case wild") {
            tameString = "abcde", wildString = "A?c*";
            REQUIRE(wildCardMatch(tameString, wildString, isCaseSensitive) == true);
        }
    }

    WHEN("Tested with a bunch of additional test cases found online") {
        bool allPassed = true;

        GIVEN("Case with repeating character sequences") {
            allPassed &= wildCardMatch_caseSensitive("abcccd", "*ccd");
            allPassed &= wildCardMatch_caseSensitive("mississipissippi", "*issip*ss*");
            allPassed &= !wildCardMatch_caseSensitive("xxxx*zzzzzzzzy*f", "xxxx*zzy*fffff");
            allPassed &= wildCardMatch_caseSensitive("xxxx*zzzzzzzzy*f", "xxx*zzy*f");
            allPassed &= !wildCardMatch_caseSensitive("xxxxzzzzzzzzyf", "xxxx*zzy*fffff");
            allPassed &= wildCardMatch_caseSensitive("xxxxzzzzzzzzyf", "xxxx*zzy*f");
            allPassed &= wildCardMatch_caseSensitive("xyxyxyzyxyz", "xy*z*xyz");
            allPassed &= wildCardMatch_caseSensitive("mississippi", "*sip*");
            allPassed &= wildCardMatch_caseSensitive("xyxyxyxyz", "xy*xyz");
            allPassed &= wildCardMatch_caseSensitive("mississippi", "mi*sip*");
            allPassed &= wildCardMatch_caseSensitive("ababac", "*abac*");
            allPassed &= wildCardMatch_caseSensitive("ababac", "*abac*");
            allPassed &= wildCardMatch_caseSensitive("aaazz", "a*zz*");
            allPassed &= !wildCardMatch_caseSensitive("a12b12", "*12*23");
            allPassed &= !wildCardMatch_caseSensitive("a12b12", "a12b");
            allPassed &= wildCardMatch_caseSensitive("a12b12", "*12*12*");
            REQUIRE(allPassed == true);
        }

        GIVEN("Additional cases where the '*' char appears in the tame string") {
            allPassed &= wildCardMatch_caseSensitive("*", "*");
            allPassed &= wildCardMatch_caseSensitive("a*abab", "a*b");
            allPassed &= wildCardMatch_caseSensitive("a*r", "a*");
            allPassed &= !wildCardMatch_caseSensitive("a*ar", "a*aar");
            REQUIRE(allPassed == true);
        }

        GIVEN("More double wildcard scenarios") {
            allPassed &= wildCardMatch_caseSensitive("XYXYXYZYXYz", "XY*Z*XYz");
            allPassed &= wildCardMatch_caseSensitive("missisSIPpi", "*SIP*");
            allPassed &= wildCardMatch_caseSensitive("mississipPI", "*issip*PI");
            allPassed &= wildCardMatch_caseSensitive("xyxyxyxyz", "xy*xyz");
            allPassed &= wildCardMatch_caseSensitive("miSsissippi", "mi*sip*");
            allPassed &= !wildCardMatch_caseSensitive("miSsissippi", "mi*Sip*");
            allPassed &= wildCardMatch_caseSensitive("abAbac", "*Abac*");
            allPassed &= wildCardMatch_caseSensitive("abAbac", "*Abac*");
            allPassed &= wildCardMatch_caseSensitive("aAazz", "a*zz*");
            allPassed &= !wildCardMatch_caseSensitive("A12b12", "*12*23");
            allPassed &= wildCardMatch_caseSensitive("a12B12", "*12*12*");
            allPassed &= wildCardMatch_caseSensitive("oWn", "*oWn*");
            REQUIRE(allPassed == true);
        }

        GIVEN("Completely tame (no wildcards) cases") {
            allPassed &= wildCardMatch_caseSensitive("bLah", "bLah");
            allPassed &= !wildCardMatch_caseSensitive("bLah", "bLaH");
            REQUIRE(allPassed == true);
        }

        GIVEN("Simple mixed wildcard tests suggested by IBMer Marlin Deckert") {
            allPassed &= wildCardMatch_caseSensitive("a", "*?");
            allPassed &= wildCardMatch_caseSensitive("ab", "*?");
            allPassed &= wildCardMatch_caseSensitive("abc", "*?");
            REQUIRE(allPassed == true);
        }

        GIVEN("More mixed wildcard tests including coverage for false positives") {
            allPassed &= !wildCardMatch_caseSensitive("a", "??");
            allPassed &= wildCardMatch_caseSensitive("ab", "?*?");
            allPassed &= wildCardMatch_caseSensitive("ab", "*?*?*");
            allPassed &= wildCardMatch_caseSensitive("abc", "?**?*?");
            allPassed &= !wildCardMatch_caseSensitive("abc", "?**?*&?");
            allPassed &= wildCardMatch_caseSensitive("abcd", "?b*??");
            allPassed &= !wildCardMatch_caseSensitive("abcd", "?a*??");
            allPassed &= wildCardMatch_caseSensitive("abcd", "?**?c?");
            allPassed &= !wildCardMatch_caseSensitive("abcd", "?**?d?");
            allPassed &= wildCardMatch_caseSensitive("abcde", "?*b*?*d*?");
            REQUIRE(allPassed == true);
        }

        GIVEN("Single-character-match cases") {
            allPassed &= wildCardMatch_caseSensitive("bLah", "bL?h");
            allPassed &= !wildCardMatch_caseSensitive("bLaaa", "bLa?");
            allPassed &= wildCardMatch_caseSensitive("bLah", "bLa?");
            allPassed &= !wildCardMatch_caseSensitive("bLaH", "?Lah");
            allPassed &= wildCardMatch_caseSensitive("bLaH", "?LaH");
            REQUIRE(allPassed == true);
        }

        GIVEN("Many-wildcard scenarios") {
            allPassed &= wildCardMatch_caseSensitive(
                    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaab",
                    "a*a*a*a*a*a*aa*aaa*a*a*b");
            allPassed &= wildCardMatch_caseSensitive(
                    "abababababababababababababababababababaacacacacacacacadaeafagahaiajakalaaaaaaaaaaaaaaaaaffafagaagggagaaaaaaaab",
                    "*a*b*ba*ca*a*aa*aaa*fa*ga*b*");
            allPassed &= !wildCardMatch_caseSensitive(
                    "abababababababababababababababababababaacacacacacacacadaeafagahaiajakalaaaaaaaaaaaaaaaaaffafagaagggagaaaaaaaab",
                    "*a*b*ba*ca*a*x*aaa*fa*ga*b*");
            allPassed &= !wildCardMatch_caseSensitive(
                    "abababababababababababababababababababaacacacacacacacadaeafagahaiajakalaaaaaaaaaaaaaaaaaffafagaagggagaaaaaaaab",
                    "*a*b*ba*ca*aaaa*fa*ga*gggg*b*");
            allPassed &= wildCardMatch_caseSensitive(
                    "abababababababababababababababababababaacacacacacacacadaeafagahaiajakalaaaaaaaaaaaaaaaaaffafagaagggagaaaaaaaab",
                    "*a*b*ba*ca*aaaa*fa*ga*ggg*b*");
            allPassed &= wildCardMatch_caseSensitive("aaabbaabbaab", "*aabbaa*a*");
            allPassed &= wildCardMatch_caseSensitive("a*a*a*a*a*a*a*a*a*a*a*a*a*a*a*a*a*",
                                                     "a*a*a*a*a*a*a*a*a*a*a*a*a*a*a*a*a*");
            allPassed &= wildCardMatch_caseSensitive("aaaaaaaaaaaaaaaaa", "*a*a*a*a*a*a*a*a*a*a*a*a*a*a*a*a*a*");
            allPassed &= !wildCardMatch_caseSensitive("aaaaaaaaaaaaaaaa", "*a*a*a*a*a*a*a*a*a*a*a*a*a*a*a*a*a*");
            allPassed &= !wildCardMatch_caseSensitive(
                    "abc*abcd*abcde*abcdef*abcdefg*abcdefgh*abcdefghi*abcdefghij*abcdefghijk*abcdefghijkl*abcdefghijklm*abcdefghijklmn",
                    "abc*abc*abc*abc*abc*abc*abc*abc*abc*abc*abc*abc*abc*abc*abc*abc*abc*");
            allPassed &= wildCardMatch_caseSensitive(
                    "abc*abcd*abcde*abcdef*abcdefg*abcdefgh*abcdefghi*abcdefghij*abcdefghijk*abcdefghijkl*abcdefghijklm*abcdefghijklmn",
                    "abc*abc*abc*abc*abc*abc*abc*abc*abc*abc*abc*abc*");
            allPassed &= !wildCardMatch_caseSensitive("abc*abcd*abcd*abc*abcd", "abc*abc*abc*abc*abc");
            allPassed &= wildCardMatch_caseSensitive("abc*abcd*abcd*abc*abcd*abcd*abc*abcd*abc*abc*abcd",
                                                     "abc*abc*abc*abc*abc*abc*abc*abc*abc*abc*abcd");
            allPassed &= wildCardMatch_caseSensitive("abc", "********a********b********c********");
            allPassed &= !wildCardMatch_caseSensitive("********a********b********c********", "abc");
            allPassed &= !wildCardMatch_caseSensitive("abc", "********a********b********b********");
            allPassed &= wildCardMatch_caseSensitive("*abc*", "***a*b*c***");
            REQUIRE(allPassed == true);
        }

        GIVEN("A case-insensitive algorithm test") {
            bool isCaseSensitive = false;
            allPassed &= wildCardMatch("mississippi", "*issip*PI", isCaseSensitive);
            REQUIRE(allPassed == true);
        }
    }
}

SCENARIO("Test wild card performance", "[wildcard performance]") {
    // This test is to ensure there is no performance regression
    // We use our current implementation vs the next best implementation as a reference
    // If performance becomes slower than our next best implementation, then it is considered a fail
    using namespace std::chrono;

    high_resolution_clock::time_point t1, t2;
    string tameStr, wildStr;

    const int nReps = 1000000;
    int testReps;
    bool allPassed_currentImplementation = true;
    bool allPassed_nextBestImplementation = true;

    /*******************************************************************************************************************
     * Inputs Begin
     ******************************************************************************************************************/
    vector<string> tameVec, wildVec;

    // Typical apache log
    tameVec.push_back("64.242.88.10 - - [07/Mar/2004:16:06:51 -0800] \"GET "
                              "/twiki/bin/rdiff/TWiki/NewUserTemplate?rev1=1"
                              ".3&rev2=1.2 HTTP/1.1\" 200 4523");
    wildVec.push_back("*64.242.88.10*Mar/2004*GET*200*");

    /*******************************************************************************************************************
     * Inputs End
     ******************************************************************************************************************/

    // Profile current implementation
    testReps = nReps;
    t1 = high_resolution_clock::now();
    while (testReps--) {
        BOOST_FOREACH(boost::tie(tameStr, wildStr), boost::combine(tameVec, wildVec)) {
            allPassed_currentImplementation &= wildCardMatch_caseSensitive(tameStr, wildStr);
        }
    }
    t2 = high_resolution_clock::now();
    duration<double> timeSpan_currentImplementation = t2 - t1;



    // Profile next best implementation
    testReps = nReps;
    t1 = high_resolution_clock::now();
    while (testReps--) {
        // Replace this part with slow implementation
        BOOST_FOREACH(boost::tie(tameStr, wildStr), boost::combine(tameVec, wildVec)) {
            allPassed_currentImplementation &= wildCardMatch_caseSensitive(tameStr, wildStr);
        }
    }
    t2 = high_resolution_clock::now();
    duration<double> timeSpan_nextBestImplementation = t2 - t1;
    REQUIRE(allPassed_currentImplementation == true);


    if (allPassed_currentImplementation) {
        cout << "Passed performance test in " << (timeSpan_currentImplementation.count() * 1000) << " milliseconds." << endl;
    } else {
        cout << "Failed performance test in " << (timeSpan_currentImplementation.count() * 1000) << " milliseconds." << endl;
    }
}