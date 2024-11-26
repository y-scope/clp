#include <Catch2/single_include/catch2/catch.hpp>
#include <regex_utils/ErrorCode.hpp>
#include <regex_utils/regex_translation_utils.hpp>
#include <regex_utils/RegexToWildcardTranslatorConfig.hpp>

using clp::regex_utils::ErrorCode;
using clp::regex_utils::regex_to_wildcard;
using clp::regex_utils::RegexToWildcardTranslatorConfig;

TEST_CASE("regex_to_wildcard_simple_translations", "[regex_utils][re2wc][simple_translations]") {
    REQUIRE(regex_to_wildcard("").value().empty());

    REQUIRE((regex_to_wildcard("xyz").value() == "xyz"));
    REQUIRE((regex_to_wildcard(". xyz .* zyx .").value() == "? xyz * zyx ?"));
    REQUIRE((regex_to_wildcard(". xyz .+ zyx .*").value() == "? xyz ?* zyx *"));
}

TEST_CASE("regex_to_wildcard_unescaped_metachar", "[regex_utils][re2wc][unescaped_metachar]") {
    REQUIRE((regex_to_wildcard(".? xyz .* zyx .").error() == ErrorCode::UnsupportedQuestionMark));
    REQUIRE((regex_to_wildcard(". xyz .** zyx .").error() == ErrorCode::UntranslatableStar));
    REQUIRE((regex_to_wildcard(". xyz .*+ zyx .").error() == ErrorCode::UntranslatablePlus));
    REQUIRE((regex_to_wildcard(". xyz |.* zyx .").error() == ErrorCode::UnsupportedPipe));
    REQUIRE((regex_to_wildcard(". xyz ^.* zyx .").error() == ErrorCode::IllegalCaret));
    REQUIRE((regex_to_wildcard(". xyz $.* zyx .").error() == ErrorCode::IllegalDollarSign));
}

TEST_CASE("regex_to_wildcard_escaped_metachar", "[regex_utils][re2wc][escaped_metachar]") {
    // Escape backslash is superfluous for the following set of characters
    REQUIRE((regex_to_wildcard("<>-_/=!").value() == "<>-_/=!"));
    REQUIRE((regex_to_wildcard("\\<\\>\\-\\_\\/\\=\\!").value() == "<>-_/=!"));
    // Test the full escape sequences set
    REQUIRE(
            (regex_to_wildcard("\\*\\+\\?\\|\\^\\$\\.\\{\\}\\[\\]\\(\\)\\<\\>\\-\\_\\/\\=\\!\\\\")
                     .value()
             == "\\*+\\?|^$.{}[]()<>-_/=!\\\\")
    );
    // Test unsupported escape sequences
    REQUIRE(
            (regex_to_wildcard("abc\\Qdefghi\\Ejkl").error()
             == clp::regex_utils::ErrorCode::IllegalEscapeSequence)
    );
}

TEST_CASE("regex_to_wildcard_charset", "[regex_utils][re2wc][charset]") {
    REQUIRE((regex_to_wildcard("x[y]z").value() == "xyz"));
    REQUIRE((regex_to_wildcard("x[\\^]z").value() == "x^z"));
    REQUIRE((regex_to_wildcard("x[\\]]z").value() == "x]z"));
    REQUIRE((regex_to_wildcard("x[-]z").value() == "x-z"));
    REQUIRE((regex_to_wildcard("x[\\-]z").value() == "x-z"));
    REQUIRE((regex_to_wildcard("x[\\\\]z").value() == "x\\\\z"));
    REQUIRE((regex_to_wildcard("[a][b][\\^][-][\\-][\\]][\\\\][c][d]").value() == "ab^--]\\\\cd"));

    REQUIRE((regex_to_wildcard("x[]y").error() == ErrorCode::UnsupportedCharsetPattern));
    REQUIRE((regex_to_wildcard("x[a-z]y").error() == ErrorCode::UnsupportedCharsetPattern));
    REQUIRE((regex_to_wildcard("x[^^]y").error() == ErrorCode::UnsupportedCharsetPattern));
    REQUIRE((regex_to_wildcard("x[^0-9]y").error() == ErrorCode::UnsupportedCharsetPattern));
    REQUIRE((regex_to_wildcard("[xX][yY]").error() == ErrorCode::UnsupportedCharsetPattern));
    REQUIRE((regex_to_wildcard("ch:[a-zA-Z0-9]").error() == ErrorCode::UnsupportedCharsetPattern));

    REQUIRE((regex_to_wildcard("[\\").error() == ErrorCode::IncompleteCharsetStructure));
    REQUIRE((regex_to_wildcard("[\\\\").error() == ErrorCode::IncompleteCharsetStructure));
    REQUIRE((regex_to_wildcard("[xX").error() == ErrorCode::IncompleteCharsetStructure));
    REQUIRE((regex_to_wildcard("ch:[a-zA-Z0-9").error() == ErrorCode::IncompleteCharsetStructure));
}

TEST_CASE("regex_to_wildcard_case_insensitive_config", "[regex_utils][re2wc][case_insensitive]") {
    RegexToWildcardTranslatorConfig const config{/*case_insensitive_wildcard=*/true, false};
    REQUIRE((regex_to_wildcard("[xX][yY]", config).value() == "xy"));
    REQUIRE((regex_to_wildcard("[Yy][Xx]", config).value() == "yx"));
    REQUIRE((regex_to_wildcard("[aA][Bb][Cc]", config).value() == "abc"));
    REQUIRE((regex_to_wildcard("[aA][Bb][\\^][-][\\]][Cc][dD]", config).value() == "ab^-]cd"));

    REQUIRE((regex_to_wildcard("[xX").error() == ErrorCode::IncompleteCharsetStructure));
    REQUIRE(
            (regex_to_wildcard("[aA][Bb][^[-[\\[Cc[dD", config).error()
             == ErrorCode::IncompleteCharsetStructure)
    );
    REQUIRE((regex_to_wildcard("ch:[a-zA-Z0-9]").error() == ErrorCode::UnsupportedCharsetPattern));
    REQUIRE(
            (regex_to_wildcard("[aA][Bb][^[-[\\[Cc[dD]", config).error()
             == ErrorCode::UnsupportedCharsetPattern)
    );
}

TEST_CASE("regex_to_wildcard_anchor_config", "[regex_utils][re2wc][anchor_config]") {
    // Test anchors and prefix/suffix wildcards
    RegexToWildcardTranslatorConfig const config{false, /*add_prefix_suffix_wildcards=*/true};
    REQUIRE(((regex_to_wildcard("^", config).value() == "*")));
    REQUIRE((regex_to_wildcard("$", config).value() == "*"));
    REQUIRE((regex_to_wildcard("^xyz$", config).value() == "xyz"));
    REQUIRE((regex_to_wildcard("xyz", config).value() == "*xyz*"));
    REQUIRE((regex_to_wildcard("xyz$$", config).value() == "*xyz"));

    REQUIRE((regex_to_wildcard("xyz$zyx$", config).error() == ErrorCode::IllegalDollarSign));
}
