#include <chrono>
#include <cstdint>
#include <iostream>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include <boost/foreach.hpp>
#include <boost/range/combine.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <string_utils/constants.hpp>
#include <string_utils/string_utils.hpp>

using clp::string_utils::clean_up_wildcard_search_string;
using clp::string_utils::convert_string_to_int;
using clp::string_utils::cSingleCharWildcard;
using clp::string_utils::cWildcardEscapeChar;
using clp::string_utils::cZeroOrMoreCharsWildcard;
using clp::string_utils::replace_unescaped_char;
using clp::string_utils::unescape_string;
using clp::string_utils::wildcard_match_unsafe;
using clp::string_utils::wildcard_match_unsafe_case_sensitive;
using std::chrono::duration;
using std::chrono::high_resolution_clock;
using std::cout;
using std::string;
using std::vector;

TEST_CASE("to_lower", "[to_lower]") {
    string str{"test123TEST"};
    clp::string_utils::to_lower(str);
    REQUIRE("test123test" == str);
}

TEST_CASE("replace_unescaped_char", "[replace_unescaped_char]") {
    auto check = [](char escape_char,
                    char from_char,
                    char to_char,
                    std::string in,
                    std::string_view expected) {
        replace_unescaped_char(escape_char, from_char, to_char, in);
        REQUIRE(in == expected);
    };

    SECTION("Conventional escape and wildcard characters") {
        auto [str, expected] = GENERATE(
                as<std::pair<string, string>>{},

                // Replacements with no escape chars present
                std::pair{R"(a?b)", R"(a*b)"},
                std::pair{R"(?leading)", R"(*leading)"},
                std::pair{R"(trailing?)", R"(trailing*)"},
                std::pair{R"(multiple??q)", R"(multiple**q)"},

                // Replacements with escape chars present
                std::pair{R"(a\\?b)", R"(a\\*b)"},
                std::pair{R"(\\?abc)", R"(\\*abc)"},
                std::pair{R"(abc\\?)", R"(abc\\*)"},

                // No replacements with escape chars present
                std::pair{R"(a\?b)", R"(a\?b)"},
                std::pair{R"(\?abc)", R"(\?abc)"},
                std::pair{R"(abc\?)", R"(abc\?)"},

                // Mixed
                std::pair{R"(a\\?b a\?b a?b)", R"(a\\*b a\?b a*b)"},
                std::pair{R"(\\?abc \?abc a?b)", R"(\\*abc \?abc a*b)"},
                std::pair{R"(abc\\? abc\? a?b)", R"(abc\\* abc\? a*b)"},

                // Additional edge cases
                std::pair{R"(no change)", R"(no change)"},
                std::pair{R"()", R"()"},
                std::pair{R"(\)", R"(\)"},
                std::pair{R"(\\)", R"(\\)"},
                std::pair{R"(?\)", R"(*\)"}
        );

        check(cWildcardEscapeChar, cSingleCharWildcard, cZeroOrMoreCharsWildcard, str, expected);
    }

    SECTION("Unconventional escape and wildcard characters") {
        check('q', 'w', 'e', R"(aqqwb aqwb awb)", R"(aqqeb aqwb aeb)");
    }
}

TEST_CASE("clean_up_wildcard_search_string", "[clean_up_wildcard_search_string]") {
    string str;

    // No wildcards
    str = "test";
    REQUIRE(clean_up_wildcard_search_string(str) == "test");

    // Only '?'
    str = "?est";
    REQUIRE(clean_up_wildcard_search_string(str) == "?est");

    // Normal case
    str = "***t**\\*s\\?t?**";
    REQUIRE(clean_up_wildcard_search_string(str) == "*t*\\*s\\?t?*");

    // Abnormal cases
    str = "***";
    REQUIRE(clean_up_wildcard_search_string(str) == "*");

    str = "*?*";
    REQUIRE(clean_up_wildcard_search_string(str) == "*?*");

    str = "?";
    REQUIRE(clean_up_wildcard_search_string(str) == "?");

    str = "?";
    REQUIRE(clean_up_wildcard_search_string(str) == "?");

    str = "a\\bc\\";
    REQUIRE(clean_up_wildcard_search_string(str) == "abc");
}

TEST_CASE("unescape_string", "[string_utils][unescape_string]") {
    SECTION("Simple string unescaping") {
        REQUIRE("*?\\" == unescape_string("\\*\\?\\\\"));
        REQUIRE("abcd" == unescape_string("abcd\\"));
    }

    SECTION("Exhaustive string unescaping") {
        std::string unescaped_string;
        std::string escaped_string;
        char c{std::numeric_limits<char>::min()};
        while (true) {
            escaped_string.push_back(cWildcardEscapeChar);
            escaped_string.push_back(c);
            unescaped_string.push_back(c);
            if (c == std::numeric_limits<char>::max()) {
                break;
            }
            ++c;
        }
        REQUIRE(unescape_string(escaped_string) == unescaped_string);
    }
}

SCENARIO("Test case sensitive wild card match in all possible ways", "[wildcard]") {
    std::string tame_string;
    std::string wild_string;

    WHEN("Match is expected if wild card character is \"*\"") {
        GIVEN("Single wild with no suffix char") {
            tame_string = "abcd", wild_string = "a*";
            REQUIRE(wildcard_match_unsafe_case_sensitive(tame_string, wild_string));
        }

        GIVEN("Single wild with no prefix char") {
            tame_string = "abcd", wild_string = "*d";
            REQUIRE(wildcard_match_unsafe_case_sensitive(tame_string, wild_string));
        }

        GIVEN("Single wild on both side & has 1st char as literal") {
            tame_string = "abcd", wild_string = "*a*";
            REQUIRE(wildcard_match_unsafe_case_sensitive(tame_string, wild_string));
        }

        GIVEN("Single wild on both side & has middle char as literal") {
            tame_string = "abcd", wild_string = "*b*";
            REQUIRE(wildcard_match_unsafe_case_sensitive(tame_string, wild_string));
        }

        GIVEN("Single wild on both side & has last char as literal") {
            tame_string = "abcd", wild_string = "*d*";
            REQUIRE(wildcard_match_unsafe_case_sensitive(tame_string, wild_string));
        }

        GIVEN("Single wild only") {
            tame_string = "abcd", wild_string = "*";
            REQUIRE(wildcard_match_unsafe_case_sensitive(tame_string, wild_string));
        }
    }

    WHEN("Match is expected if Wild card character is \"?\"") {
        GIVEN("Single wild in the middle") {
            tame_string = "abcd", wild_string = "a?cd";
            REQUIRE(wildcard_match_unsafe_case_sensitive(tame_string, wild_string));
        }

        GIVEN("Single wild in the beginning") {
            tame_string = "abcd", wild_string = "?bcd";
            REQUIRE(wildcard_match_unsafe_case_sensitive(tame_string, wild_string));
        }

        GIVEN("Single wild at the end") {
            tame_string = "abcd", wild_string = "abc?";
            REQUIRE(wildcard_match_unsafe_case_sensitive(tame_string, wild_string));
        }

        GIVEN("Multiple wild in the middle") {
            tame_string = "abcd", wild_string = "a??d";
            REQUIRE(wildcard_match_unsafe_case_sensitive(tame_string, wild_string));
        }

        GIVEN("Multiple wild in the beginning") {
            tame_string = "abcd", wild_string = "??cd";
            REQUIRE(wildcard_match_unsafe_case_sensitive(tame_string, wild_string));
        }

        GIVEN("Multiple wild in the end") {
            tame_string = "abcd", wild_string = "ab??";
            REQUIRE(wildcard_match_unsafe_case_sensitive(tame_string, wild_string));
        }

        GIVEN("Single wild in the beginning and end") {
            tame_string = "abcd", wild_string = "?bc?";
            REQUIRE(wildcard_match_unsafe_case_sensitive(tame_string, wild_string));
        }

        GIVEN("Multiple wild anywhere") {
            tame_string = "abcdef", wild_string = "a?c?ef";
            REQUIRE(wildcard_match_unsafe_case_sensitive(tame_string, wild_string));
        }

        GIVEN("All wild") {
            tame_string = "abcd", wild_string = "????";
            REQUIRE(wildcard_match_unsafe_case_sensitive(tame_string, wild_string));
        }
    }

    WHEN("Match is expected if wild card character has both \"*\" and \"?\"") {
        GIVEN("Wild begins with \"*?\" pattern with 0 matched char for \"*\"") {
            tame_string = "abcd", wild_string = "*?bcd";
            REQUIRE(wildcard_match_unsafe_case_sensitive(tame_string, wild_string));
        }

        GIVEN("Wild begins with \"?*\" pattern with 0 matched char for \"*\"") {
            tame_string = "abcd", wild_string = "?*bcd";
            REQUIRE(wildcard_match_unsafe_case_sensitive(tame_string, wild_string));
        }

        GIVEN("Wild begins with \"*?\" pattern with 1 matched char for \"*\"") {
            tame_string = "abcd", wild_string = "*?cd";
            REQUIRE(wildcard_match_unsafe_case_sensitive(tame_string, wild_string));
        }

        GIVEN("Wild begins with \"?*\" pattern with 1 matched char for \"*\"") {
            tame_string = "abcd", wild_string = "?*cd";
            REQUIRE(wildcard_match_unsafe_case_sensitive(tame_string, wild_string));
        }

        GIVEN("Wild ends with \"*?\" pattern with 0 matched char for \"*\"") {
            tame_string = "abcd", wild_string = "abc*?";
            REQUIRE(wildcard_match_unsafe_case_sensitive(tame_string, wild_string));
        }

        GIVEN("Wild ends with \"?*\" pattern with 0 matched char for \"*\"") {
            tame_string = "abcd", wild_string = "abc?*";
            REQUIRE(wildcard_match_unsafe_case_sensitive(tame_string, wild_string));
        }

        GIVEN("Wild ends with \"*?\" pattern with 1 matched char for \"*\"") {
            tame_string = "abcd", wild_string = "ab*?";
            REQUIRE(wildcard_match_unsafe_case_sensitive(tame_string, wild_string));
        }

        GIVEN("Wild ends with \"?*\" pattern with 1 matched char for \"*\"") {
            tame_string = "abcd", wild_string = "ab?*";
            REQUIRE(wildcard_match_unsafe_case_sensitive(tame_string, wild_string));
        }

        GIVEN("Wild begins with exactly \"*?\" pattern") {
            tame_string = "abcd", wild_string = "*?";
            REQUIRE(wildcard_match_unsafe_case_sensitive(tame_string, wild_string));
        }

        GIVEN("Wild begins with exactly \"?*\" pattern") {
            tame_string = "abcd", wild_string = "?*";
            REQUIRE(wildcard_match_unsafe_case_sensitive(tame_string, wild_string));
        }
    }

    WHEN("Match unexpected containing wild card character(s)") {
        GIVEN("Missing literal character w/ \"*\"") {
            tame_string = "abcd", wild_string = "ac*";
            REQUIRE(wildcard_match_unsafe_case_sensitive(tame_string, wild_string) == false);
        }

        GIVEN("More literals in wild than tame w/ \"*\" in the middle") {
            tame_string = "abcd", wild_string = "abc*de";
            REQUIRE(wildcard_match_unsafe_case_sensitive(tame_string, wild_string) == false);
        }

        GIVEN("MISSING matching literals in the beginning with \"*\" in the middle") {
            tame_string = "abcd", wild_string = "b**d";
            REQUIRE(wildcard_match_unsafe_case_sensitive(tame_string, wild_string) == false);
        }

        GIVEN("MISSING matching literals in the end with \"*\" in the middle") {
            tame_string = "abcd", wild_string = "a**c";
            REQUIRE(wildcard_match_unsafe_case_sensitive(tame_string, wild_string) == false);
        }

        GIVEN(
                "MISSING matching literals in the beginning with both \"*\" and \"?\" in the middle"
        ) {
            tame_string = "abcd", wild_string = "b*?d";
            REQUIRE(wildcard_match_unsafe_case_sensitive(tame_string, wild_string) == false);
        }

        GIVEN("MISSING matching literals in the beginning with \"?\" at the beginning") {
            tame_string = "abcd", wild_string = "?cd";
            REQUIRE(wildcard_match_unsafe_case_sensitive(tame_string, wild_string) == false);
        }

        GIVEN("MISSING matching literals in the end with both \"?\" at the end") {
            tame_string = "abcd", wild_string = "ab?";
            REQUIRE(wildcard_match_unsafe_case_sensitive(tame_string, wild_string) == false);
        }
    }

    WHEN("Match is expected when escape character(s) are used") {
        GIVEN("Escaping \"*\"") {
            tame_string = "a*cd", wild_string = "a\\*cd";
            REQUIRE(wildcard_match_unsafe_case_sensitive(tame_string, wild_string));
        }

        GIVEN("Escaping \"?\"") {
            tame_string = "a?cd", wild_string = "a\\?cd";
            REQUIRE(wildcard_match_unsafe_case_sensitive(tame_string, wild_string));
        }

        GIVEN("Escaping \"*\" and \"?\"") {
            tame_string = "a?c*e", wild_string = "a\\?c\\*e";
            REQUIRE(wildcard_match_unsafe_case_sensitive(tame_string, wild_string));
        }

        GIVEN("Escaping \"\\\"") {
            tame_string = "a\\cd", wild_string = "a\\\\cd";
            REQUIRE(wildcard_match_unsafe_case_sensitive(tame_string, wild_string));
        }

        GIVEN("Escaping \"?\" when fast forwarding") {
            tame_string = "abc?e", wild_string = "a*\\?e";
            REQUIRE(wildcard_match_unsafe_case_sensitive(tame_string, wild_string));
        }

        GIVEN("Escaping \"*\" when fast forwarding") {
            tame_string = "abc*e", wild_string = "a*\\*e";
            REQUIRE(wildcard_match_unsafe_case_sensitive(tame_string, wild_string));
        }

        GIVEN("Escaping \"\\\" when fast forwarding") {
            tame_string = "abc\\e", wild_string = "a*\\\\e";
            REQUIRE(wildcard_match_unsafe_case_sensitive(tame_string, wild_string));
        }

        GIVEN("Escaping \"?\" when rewinding") {
            tame_string = R"(\ab\ab\c?ef)", wild_string = R"(*ab\\c\?*)";
            REQUIRE(wildcard_match_unsafe_case_sensitive(tame_string, wild_string));
        }

        GIVEN("Escaping \"*\" when rewinding") {
            tame_string = R"(\ab\ab\c*ef)", wild_string = R"(*ab\\c\**)";
            REQUIRE(wildcard_match_unsafe_case_sensitive(tame_string, wild_string));
        }

        GIVEN("Escaping \"\\\" when rewinding") {
            tame_string = R"(\ab\ab\c\ef)", wild_string = R"(*ab\\c\\*)";
            REQUIRE(wildcard_match_unsafe_case_sensitive(tame_string, wild_string));
        }

        GIVEN("Silently ignore unsupported escape sequence \\a") {
            tame_string = "ab?d", wild_string = "\\ab?d";
            REQUIRE(wildcard_match_unsafe_case_sensitive(tame_string, wild_string));
        }
    }

    WHEN("Case wild card match is case insensitive") {
        // This test is meant to supplement the case sensitive wild card implementation
        // The case insensitive implementation is exactly the same as case sensitive except it
        // automatically adjust the inputs to lower case when needed before doing comparison. It is
        // rarely used due to lower performance
        bool const is_case_sensitive{false};
        GIVEN("All lower case tame and all upper case wild") {
            tame_string = "abcde", wild_string = "A?C*";
            REQUIRE(wildcard_match_unsafe(tame_string, wild_string, is_case_sensitive));
        }

        GIVEN("All lower case tame and mixed lower and upper case wild") {
            tame_string = "abcde", wild_string = "A?c*";
            REQUIRE(wildcard_match_unsafe(tame_string, wild_string, is_case_sensitive));

            tame_string = "abcde", wild_string = "a?C*";
            REQUIRE(wildcard_match_unsafe(tame_string, wild_string, is_case_sensitive));
        }
    }

    WHEN("Tested with a bunch of additional test cases found online") {
        GIVEN("Case with repeating character sequences") {
            REQUIRE(wildcard_match_unsafe_case_sensitive("abcccd", "*ccd"));
            REQUIRE(wildcard_match_unsafe_case_sensitive("mississipissippi", "*issip*ss*"));
            REQUIRE_FALSE(wildcard_match_unsafe_case_sensitive("xxxx*zzzzzzzzy*f", "xxxx*zzy*fffff"));
            REQUIRE(wildcard_match_unsafe_case_sensitive("xxxx*zzzzzzzzy*f", "xxx*zzy*f"));
            REQUIRE_FALSE(wildcard_match_unsafe_case_sensitive("xxxxzzzzzzzzyf", "xxxx*zzy*fffff"));
            REQUIRE(wildcard_match_unsafe_case_sensitive("xxxxzzzzzzzzyf", "xxxx*zzy*f"));
            REQUIRE(wildcard_match_unsafe_case_sensitive("xyxyxyzyxyz", "xy*z*xyz"));
            REQUIRE(wildcard_match_unsafe_case_sensitive("mississippi", "*sip*"));
            REQUIRE(wildcard_match_unsafe_case_sensitive("xyxyxyxyz", "xy*xyz"));
            REQUIRE(wildcard_match_unsafe_case_sensitive("mississippi", "mi*sip*"));
            REQUIRE(wildcard_match_unsafe_case_sensitive("ababac", "*abac*"));
            REQUIRE(wildcard_match_unsafe_case_sensitive("ababac", "*abac*"));
            REQUIRE(wildcard_match_unsafe_case_sensitive("aaazz", "a*zz*"));
            REQUIRE_FALSE(wildcard_match_unsafe_case_sensitive("a12b12", "*12*23"));
            REQUIRE_FALSE(wildcard_match_unsafe_case_sensitive("a12b12", "a12b"));
            REQUIRE(wildcard_match_unsafe_case_sensitive("a12b12", "*12*12*"));
        }
        
        GIVEN("Additional cases where the '*' char appears in the tame string") {
            REQUIRE(wildcard_match_unsafe_case_sensitive("*", "*"));
            REQUIRE(wildcard_match_unsafe_case_sensitive("a*abab", "a*b"));
            REQUIRE(wildcard_match_unsafe_case_sensitive("a*r", "a*"));
            REQUIRE_FALSE(wildcard_match_unsafe_case_sensitive("a*ar", "a*aar"));
        }

        GIVEN("More double wildcard scenarios") {
            REQUIRE(wildcard_match_unsafe_case_sensitive("XYXYXYZYXYz", "XY*Z*XYz"));
            REQUIRE(wildcard_match_unsafe_case_sensitive("missisSIPpi", "*SIP*"));
            REQUIRE(wildcard_match_unsafe_case_sensitive("mississipPI", "*issip*PI"));
            REQUIRE(wildcard_match_unsafe_case_sensitive("xyxyxyxyz", "xy*xyz"));
            REQUIRE(wildcard_match_unsafe_case_sensitive("miSsissippi", "mi*sip*"));
            REQUIRE_FALSE(wildcard_match_unsafe_case_sensitive("miSsissippi", "mi*Sip*"));
            REQUIRE(wildcard_match_unsafe_case_sensitive("abAbac", "*Abac*"));
            REQUIRE(wildcard_match_unsafe_case_sensitive("aAazz", "a*zz*"));
            REQUIRE_FALSE(wildcard_match_unsafe_case_sensitive("A12b12", "*12*23"));
            REQUIRE_FALSE(wildcard_match_unsafe_case_sensitive("a12B12", "*12*12*"));
            REQUIRE(wildcard_match_unsafe_case_sensitive("oWn", "*oWn*"));
        }

        GIVEN("Completely tame (no wildcards) cases") {
            REQUIRE(wildcard_match_unsafe_case_sensitive("bLah", "bLah"));
            REQUIRE_FALSE(wildcard_match_unsafe_case_sensitive("bLah", "bLaH"));
        }

        GIVEN("Simple mixed wildcard tests suggested by IBMer Marlin Deckert") {
            REQUIRE(wildcard_match_unsafe_case_sensitive("a", "*?"));
            REQUIRE(wildcard_match_unsafe_case_sensitive("ab", "*?"));
            REQUIRE(wildcard_match_unsafe_case_sensitive("abc", "*?"));
        }

        GIVEN("More mixed wildcard tests including coverage for false positives") {
            REQUIRE_FALSE(wildcard_match_unsafe_case_sensitive("a", "??"));
            REQUIRE(wildcard_match_unsafe_case_sensitive("ab", "?*?"));
            REQUIRE(wildcard_match_unsafe_case_sensitive("ab", "*?*?*"));
            REQUIRE(wildcard_match_unsafe_case_sensitive("abcd", "?b*??"));
            REQUIRE_FALSE(wildcard_match_unsafe_case_sensitive("abcd", "?a*??"));
            REQUIRE(wildcard_match_unsafe_case_sensitive("abcde", "?*b*?*d*?"));
        }

        GIVEN("Single-character-match cases") {
            REQUIRE(wildcard_match_unsafe_case_sensitive("bLah", "bL?h"));
            REQUIRE_FALSE(wildcard_match_unsafe_case_sensitive("bLaaa", "bLa?"));
            REQUIRE(wildcard_match_unsafe_case_sensitive("bLah", "bLa?"));
            REQUIRE_FALSE(wildcard_match_unsafe_case_sensitive("bLaH", "?Lah"));
            REQUIRE(wildcard_match_unsafe_case_sensitive("bLaH", "?LaH"));
        }

        GIVEN("Many-wildcard scenarios") {
            REQUIRE(wildcard_match_unsafe_case_sensitive(
                    "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                    "aaaaaaaaaaaab",
                    "a*a*a*a*a*a*aa*aaa*a*a*b"
            ));
            REQUIRE(wildcard_match_unsafe_case_sensitive(
                    "abababababababababababababababababababaacacacacacacacadaeafagahaiajakalaaaaaaa"
                    "aaaaaaaaaaffafagaagggagaaaaaaaab",
                    "*a*b*ba*ca*a*aa*aaa*fa*ga*b*"
            ));
            REQUIRE_FALSE(wildcard_match_unsafe_case_sensitive(
                    "abababababababababababababababababababaacacacacacacacadaeafagahaiajakalaaaaaaa"
                    "aaaaaaaaaaffafagaagggagaaaaaaaab",
                    "*a*b*ba*ca*a*x*aaa*fa*ga*b*"
            ));
            REQUIRE_FALSE(wildcard_match_unsafe_case_sensitive(
                    "abababababababababababababababababababaacacacacacacacadaeafagahaiajakalaaaaaaa"
                    "aaaaaaaaaaffafagaagggagaaaaaaaab",
                    "*a*b*ba*ca*aaaa*fa*ga*gggg*b*"
            ));
            REQUIRE_FALSE(wildcard_match_unsafe_case_sensitive(
                    "abababababababababababababababababababaacacacacacacacadaeafagahaiajakalaaaaaaa"
                    "aaaaaaaaaaffafagaagggagaaaaaaaab",
                    "*a*b*ba*ca*aaaa*fa*ga*ggg*b*"
            ));
            REQUIRE(wildcard_match_unsafe_case_sensitive("aaabbaabbaab", "*aabbaa*a*"));
            REQUIRE(wildcard_match_unsafe_case_sensitive(
                    "a*a*a*a*a*a*a*a*a*a*a*a*a*a*a*a*a*",
                    "a*a*a*a*a*a*a*a*a*a*a*a*a*a*a*a*a*"
            ));
            REQUIRE(wildcard_match_unsafe_case_sensitive(
                    "aaaaaaaaaaaaaaaaa",
                    "*a*a*a*a*a*a*a*a*a*a*a*a*a*a*a*a*a*"
            ));
            REQUIRE_FALSE(wildcard_match_unsafe_case_sensitive(
                    "aaaaaaaaaaaaaaaa",
                    "*a*a*a*a*a*a*a*a*a*a*a*a*a*a*a*a*a*"
            ));
            REQUIRE_FALSE(wildcard_match_unsafe_case_sensitive(
                    "abc*abcd*abcde*abcdef*abcdefg*abcdefgh*abcdefghi*abcdefghij*abcdefghijk*"
                    "abcdefghijkl*abcdefghijklm*abcdefghijklmn",
                    "abc*abc*abc*abc*abc*abc*abc*abc*abc*abc*abc*abc*abc*abc*abc*abc*abc*"
            ));
            REQUIRE_FALSE(wildcard_match_unsafe_case_sensitive(
                    "abc*abcd*abcde*abcdef*abcdefg*abcdefgh*abcdefghi*abcdefghij*abcdefghijk*"
                    "abcdefghijkl*abcdefghijklm*abcdefghijklmn",
                    "abc*abc*abc*abc*abc*abc*abc*abc*abc*abc*abc*abc*"
            ));
            REQUIRE_FALSE(wildcard_match_unsafe_case_sensitive(
                    "abc*abcd*abcd*abc*abcd",
                    "abc*abc*abc*abc*abc"
            ));
            REQUIRE_FALSE(wildcard_match_unsafe_case_sensitive(
                    "abc*abcd*abcd*abc*abcd*abcd*abc*abcd*abc*abc*abcd",
                    "abc*abc*abc*abc*abc*abc*abc*abc*abc*abc*abcd"
            ));
        }

        GIVEN("A case-insensitive algorithm test") {
            bool const is_case_sensitive{false};
            REQUIRE(wildcard_match_unsafe("mississippi", "*issip*PI", is_case_sensitive));
        }
    }
}

SCENARIO("Test wild card performance", "[wildcard performance]") {
    // This test is to ensure there is no performance regression
    // We use our current implementation vs the next best implementation as a reference
    // If performance becomes slower than our next best implementation, then it is considered a fail

    high_resolution_clock::time_point t1;
    high_resolution_clock::time_point t2;
    string tame_str;
    string wild_str;

    int const n_reps{1'000'000};
    int test_reps{0};
    bool all_passed_current_implementation{true};
    bool const all_passed_next_best_implementation{true};

    /***********************************************************************************************
     * Inputs Begin
     **********************************************************************************************/
    vector<string> tame_vec;
    vector<string> wild_vec;

    // Typical apache log
    tame_vec.emplace_back(
            "64.242.88.10 - - [07/Mar/2004:16:06:51 -0800]"
            " \"GET /twiki/bin/rdiff/TWiki/NewUserTemplate?rev1=1.3&rev2=1.2 HTTP/1.1\" 200 4523"
    );
    wild_vec.emplace_back("*64.242.88.10*Mar/2004*GET*200*");

    /***********************************************************************************************
     * Inputs End
     **********************************************************************************************/

    // Profile current implementation
    test_reps = n_reps;
    t1 = high_resolution_clock::now();
    while (test_reps-- != 0) {
        BOOST_FOREACH (boost::tie(tame_str, wild_str), boost::combine(tame_vec, wild_vec)) {
            all_passed_current_implementation
                    &= wildcard_match_unsafe_case_sensitive(tame_str, wild_str);
        }
    }
    t2 = high_resolution_clock::now();
    duration<double> const time_span_current_implementation = t2 - t1;

    // Profile next best implementation
    test_reps = n_reps;
    t1 = high_resolution_clock::now();
    while (test_reps-- != 0) {
        // Replace this part with slow implementation
        BOOST_FOREACH (boost::tie(tame_str, wild_str), boost::combine(tame_vec, wild_vec)) {
            all_passed_current_implementation
                    &= wildcard_match_unsafe_case_sensitive(tame_str, wild_str);
        }
    }
    t2 = high_resolution_clock::now();
    duration<double> const time_span_next_best_implementation = t2 - t1;
    REQUIRE(all_passed_current_implementation);

    if (all_passed_current_implementation) {
        cout << "Passed performance test in "
             << (duration_cast<std::chrono::milliseconds>(time_span_current_implementation).count())
             << " milliseconds." << '\n';
    } else {
        cout << "Failed performance test in "
             << (duration_cast<std::chrono::milliseconds>(time_span_current_implementation).count())
             << " milliseconds." << '\n';
    }
}

TEST_CASE("convert_string_to_int", "[convert_string_to_int]") {
    int64_t raw_as_int{0};
    string raw;
    int64_t converted{0};

    // Corner cases
    // Empty string
    raw = "";
    REQUIRE(false == convert_string_to_int(raw, converted));

    // Edges of representable range
    raw_as_int = INT64_MAX;
    raw = std::to_string(raw_as_int);
    REQUIRE(convert_string_to_int(raw, converted));
    REQUIRE(raw_as_int == converted);

    raw_as_int = INT64_MIN;
    raw = std::to_string(raw_as_int);
    REQUIRE(convert_string_to_int(raw, converted));
    REQUIRE(raw_as_int == converted);

    raw = "9223372036854775808";  // INT64_MAX + 1 == 2^63
    REQUIRE(false == convert_string_to_int(raw, converted));

    raw = "-9223372036854775809";  // INT64_MIN - 1 == -2^63 - 1
    REQUIRE(false == convert_string_to_int(raw, converted));

    // Non-integers
    raw = "abc";
    REQUIRE(false == convert_string_to_int(raw, converted));

    raw = "90a";
    REQUIRE(false == convert_string_to_int(raw, converted));

    raw = "0.5";
    REQUIRE(false == convert_string_to_int(raw, converted));

    // Non-decimal integers
    raw = "0x5A";
    REQUIRE(false == convert_string_to_int(raw, converted));

    // Integers
    raw = "98340";
    REQUIRE(convert_string_to_int(raw, converted));
    REQUIRE(98'340 == converted);
}
