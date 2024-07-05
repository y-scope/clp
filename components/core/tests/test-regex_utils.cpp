#include <iostream>
#include <regex_utils/regex_utils.hpp>

#include <Catch2/single_include/catch2/catch.hpp>

using clp::regex_utils::regexToWildcard;
using clp::regex_utils::regexTrimLineAnchors;
using clp::regex_utils::regexHasStartAnchor;
using clp::regex_utils::regexHasEndAnchor;

TEST_CASE("regexToWildcard", "[regex_utils][regexToWildcard]") {
    // Test empty string
    REQUIRE(regexToWildcard("") == "");

    // Test anchors
    REQUIRE(regexToWildcard("^") == "*");
    REQUIRE(regexToWildcard("$") == "*");
    REQUIRE(regexToWildcard("xyz") == "*xyz*");

    // Test simple wildcard translations
    REQUIRE(regexToWildcard("^xyz$") == "xyz");
    REQUIRE(regexToWildcard("^. xyz .* zyx .$") == "? xyz * zyx ?");
    REQUIRE(regexToWildcard("^. xyz .* zyx .*$") == "? xyz * zyx *");

    // Test unescaped meta characters
    REQUIRE_THROWS_AS(regexToWildcard("^. xyz ^.* zyx .$"), std::invalid_argument);
    REQUIRE_THROWS_AS(regexToWildcard("^. xyz |.* zyx .$"), std::runtime_error);
    REQUIRE_THROWS_AS(regexToWildcard("^. xyz ?.* zyx .$"), std::invalid_argument);
    REQUIRE_THROWS_AS(regexToWildcard("^. xyz .** zyx .$"), std::invalid_argument);
    REQUIRE_THROWS_AS(regexToWildcard("^. xyz .*+ zyx .$"), std::invalid_argument);
}

TEST_CASE("regexTrimLineAnchors", "[regex_utils][regexTrimLineAnchors]") {
    REQUIRE(regexTrimLineAnchors("") == "");
    REQUIRE(regexTrimLineAnchors("^^^hello$$$") == "^hello$");
    REQUIRE(regexTrimLineAnchors("^^\\^hello$$$") == "^\\^hello$");
    REQUIRE(regexTrimLineAnchors("^^^hello\\$$$") == "^hello\\$$");
    REQUIRE(regexTrimLineAnchors("^^\\^hello\\$$$") == "^\\^hello\\$$");
    REQUIRE(regexTrimLineAnchors("^^^hello\\\\\\\\\\\\\\$$$") == "^hello\\\\\\\\\\\\\\$$");
    REQUIRE(regexTrimLineAnchors("^^^\\\\goodbye\\\\\\\\\\\\$$$") == "^\\\\goodbye\\\\\\\\\\\\$");
}

TEST_CASE("regexHasStartAnchor", "[regex_utils][regexHasStartAnchor]") {
    REQUIRE_FALSE(regexHasStartAnchor(""));
    REQUIRE(regexHasStartAnchor("^hello$"));
    REQUIRE_FALSE(regexHasStartAnchor("\\^hello$"));
    REQUIRE(regexHasStartAnchor("^hello\\$"));
    REQUIRE_FALSE(regexHasStartAnchor("\\^hello\\$"));
    REQUIRE(regexHasStartAnchor("^hello\\\\\\\\\\\\\\$"));
    REQUIRE(regexHasStartAnchor("^\\\\goodbye\\\\\\\\\\\\$"));
}

TEST_CASE("regexHasEndAnchor", "[regex_utils][regexHasEndAnchor]") {
    REQUIRE_FALSE(regexHasEndAnchor(""));
    REQUIRE(regexHasEndAnchor("^hello$"));
    REQUIRE(regexHasEndAnchor("\\^hello$"));
    REQUIRE_FALSE(regexHasEndAnchor("^hello\\$"));
    REQUIRE_FALSE(regexHasEndAnchor("\\^hello\\$"));
    REQUIRE_FALSE(regexHasEndAnchor("^hello\\\\\\\\\\\\\\$"));
    REQUIRE(regexHasEndAnchor("^\\\\goodbye\\\\\\\\\\\\$"));
    REQUIRE(regexHasEndAnchor("\\\\\\\\\\\\$"));
    REQUIRE_FALSE(regexHasEndAnchor("\\\\\\\\\\\\\\$"));
}

