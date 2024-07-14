#include <regex_utils/ErrorCode.hpp>
#include <regex_utils/regex_utils.hpp>
#include <regex_utils/RegexToWildcardTranslatorConfig.hpp>

#include <Catch2/single_include/catch2/catch.hpp>

using clp::regex_utils::regex_has_end_anchor;
using clp::regex_utils::regex_has_start_anchor;
using clp::regex_utils::regex_to_wildcard;
using clp::regex_utils::regex_trim_line_anchors;

TEST_CASE("regex_to_wildcard", "[regex_utils][regex_to_wildcard]") {
    // Test empty string
    REQUIRE(regex_to_wildcard("").value().empty());

    // Test simple wildcard translations
    REQUIRE((regex_to_wildcard("^xyz$").value() == "xyz"));
    REQUIRE((regex_to_wildcard("xyz").value() == "xyz"));
    REQUIRE((regex_to_wildcard(". xyz .* zyx .").value() == "? xyz * zyx ?"));
    REQUIRE((regex_to_wildcard(". xyz .+ zyx .*").value() == "? xyz ?* zyx *"));

    // Test unescaped meta characters
    REQUIRE((regex_to_wildcard(".? xyz .* zyx .").error() == clp::regex_utils::ErrorCode::Question)
    );
    REQUIRE((regex_to_wildcard(". xyz .** zyx .").error() == clp::regex_utils::ErrorCode::Star));
    REQUIRE((regex_to_wildcard(". xyz .*+ zyx .").error() == clp::regex_utils::ErrorCode::Plus));
    REQUIRE((regex_to_wildcard(". xyz |.* zyx .").error() == clp::regex_utils::ErrorCode::Pipe));
    REQUIRE((regex_to_wildcard(". xyz ^.* zyx .").error() == clp::regex_utils::ErrorCode::Caret));

    // Test properly escaped meta characters
    REQUIRE(
            (regex_to_wildcard("\\^\\$\\.\\*\\{\\}\\[\\]\\(\\)\\+\\|\\?\\<\\>\\-\\_\\/\\=\\!\\\\")
                     .value()
             == "^$.\\*{}[]()+|\\?<>-_/=!\\\\")
    );
    REQUIRE(
            (regex_to_wildcard("abc\\Qdefghi\\Ejkl").error()
             == clp::regex_utils::ErrorCode::DisallowedEscapeSequence)
    );

    // Test quantifiers
    REQUIRE((regex_to_wildcard("abc{3}").value() == "abccc"));
    REQUIRE((regex_to_wildcard("abc{4}").value() == "abcccc"));
    REQUIRE((regex_to_wildcard("abc{0}").value() == "ab"));
    REQUIRE((regex_to_wildcard("abc.{4}").value() == "abc????"));
    REQUIRE((regex_to_wildcard("abc\\[{4}").value() == "abc[[[["));
    REQUIRE((regex_to_wildcard("abc\\^{4}").value() == "abc^^^^"));
    REQUIRE((regex_to_wildcard("abc\\*{4}").value() == "abc\\*\\*\\*\\*"));
    REQUIRE((regex_to_wildcard("abc\\?{4}").value() == "abc\\?\\?\\?\\?"));
    REQUIRE((regex_to_wildcard("abc{123").value() == "abc{123"));
    REQUIRE((regex_to_wildcard("abc{123,456").value() == "abc{123,456"));
    REQUIRE((regex_to_wildcard("abc{00123\\*").value() == "abc{00123\\*"));
    REQUIRE((regex_to_wildcard("abc{3,4{{{{3}").value() == "abc{3,4{{{{{"));
    REQUIRE((regex_to_wildcard("abc{3,4{3,4{3,{3}").value() == "abc{3,4{3,4{3,,,"));
    REQUIRE((regex_to_wildcard("abc{3,4{3,4{3,4{3}").value() == "abc{3,4{3,4{3,444"));
    REQUIRE((regex_to_wildcard("abc{3,4{3,4{3,4.*").value() == "abc{3,4{3,4{3,4*"));
    REQUIRE((regex_to_wildcard("abc{3,4{3,4{3,4\\[a-z]").value() == "abc{3,4{3,4{3,4[a-z]"));
    REQUIRE((regex_to_wildcard("abc{3,4{3,4{3,4\\*{4}").value() == "abc{3,4{3,4{3,4\\*\\*\\*\\*"));

    REQUIRE(
            (regex_to_wildcard("abc{-3}").error()
             == clp::regex_utils::ErrorCode::UnsupportedQuantifier)
    );
    REQUIRE(
            (regex_to_wildcard("abc{3,4}").error()
             == clp::regex_utils::ErrorCode::UnsupportedQuantifier)
    );

    REQUIRE((
            regex_to_wildcard("{3}abc").error() == clp::regex_utils::ErrorCode::TokenUnquantifiable
    ));
    REQUIRE(
            (regex_to_wildcard("abc{3}{3}").error()
             == clp::regex_utils::ErrorCode::TokenUnquantifiable)
    );
    REQUIRE(
            (regex_to_wildcard("abc.*{3}").error()
             == clp::regex_utils::ErrorCode::TokenUnquantifiable)
    );
    REQUIRE(
            (regex_to_wildcard("abc.+{3}").error()
             == clp::regex_utils::ErrorCode::TokenUnquantifiable)
    );

    // Test grouping and quantifiers
    REQUIRE((regex_to_wildcard("(xyz)").value() == "xyz"));
    REQUIRE((regex_to_wildcard("abc (xyz) def").value() == "abc xyz def"));
    REQUIRE((regex_to_wildcard("abc () def").value() == "abc  def"));
    REQUIRE(
            (regex_to_wildcard("abc (. xyz .+ zyx .*){2} def").value()
             == "abc ? xyz ?* zyx *? xyz ?* zyx * def")
    );
    REQUIRE(
            (regex_to_wildcard("abc (.{3} xyz .+ zyx .*){2} def").value()
             == "abc ??? xyz ?* zyx *??? xyz ?* zyx * def")
    );
    REQUIRE((regex_to_wildcard("abc (\\)){2} def").value() == "abc )) def"));
    REQUIRE((regex_to_wildcard("abc (\\)\\*){2} def").value() == "abc )\\*)\\* def"));
    REQUIRE((
            regex_to_wildcard("abc (x(\\*){3}z){2} def").value() == "abc x\\*\\*\\*zx\\*\\*\\*z def"
    ));

    REQUIRE(
            (regex_to_wildcard("abc (. xyz .+ zyx .*{2} def").error()
             == clp::regex_utils::ErrorCode::UnmatchedParenthesis)
    );
    REQUIRE(
            (regex_to_wildcard("abc (x(\\*{3}z){2} def").error()
             == clp::regex_utils::ErrorCode::UnmatchedParenthesis)
    );
    REQUIRE(
            (regex_to_wildcard("abc (x(\\*){3}z{2} def").error()
             == clp::regex_utils::ErrorCode::UnmatchedParenthesis)
    );
    REQUIRE(
            (regex_to_wildcard("abc x(\\*){3}z){2} def").error()
             == clp::regex_utils::ErrorCode::UnmatchedParenthesis)
    );
    REQUIRE(
            (regex_to_wildcard("abc (x\\*){3}z){2} def").error()
             == clp::regex_utils::ErrorCode::UnmatchedParenthesis)
    );
    REQUIRE((
            regex_to_wildcard("abc (abc | def){2} def").error() == clp::regex_utils::ErrorCode::Pipe
    ));
    REQUIRE(
            (regex_to_wildcard("abc (* xyz .+ zyx .*){2} def").error()
             == clp::regex_utils::ErrorCode::Star)
    );
    REQUIRE(
            (regex_to_wildcard("abc (+ xyz .+ zyx .*){2} def").error()
             == clp::regex_utils::ErrorCode::Plus)
    );
    REQUIRE(
            (regex_to_wildcard("abc (.{3}{3} xyz .+ zyx .*){2} def").error()
             == clp::regex_utils::ErrorCode::TokenUnquantifiable)
    );
    REQUIRE(
            (regex_to_wildcard("abc (. xyz .+{3} zyx .*){2} def").error()
             == clp::regex_utils::ErrorCode::TokenUnquantifiable)
    );

    // Test charset and quantifiers
    REQUIRE((regex_to_wildcard("x[y]z").value() == "xyz"));
    REQUIRE((regex_to_wildcard("x[y]{2}z").value() == "xyyz"));
    REQUIRE((regex_to_wildcard("x[+]{2}z").value() == "x++z"));
    REQUIRE((regex_to_wildcard("x[-]{2}z").value() == "x--z"));
    REQUIRE((regex_to_wildcard("x[|]{2}z").value() == "x||z"));
    REQUIRE((regex_to_wildcard("x[\\-]{2}z").value() == "x--z"));
    REQUIRE((regex_to_wildcard("x[\\^]{2}z").value() == "x^^z"));
    REQUIRE((regex_to_wildcard("x[\\]]{2}z").value() == "x]]z"));
    REQUIRE((regex_to_wildcard("x[*]{2}z").value() == "x\\*\\*z"));
    REQUIRE((regex_to_wildcard("x[?]{2}z").value() == "x\\?\\?z"));
    REQUIRE((regex_to_wildcard("x[\\\\]{2}z").value() == "x\\\\\\\\z"));

    REQUIRE((regex_to_wildcard("abc (x[*]{2}z){2} def").value() == "abc x\\*\\*zx\\*\\*z def"));
    REQUIRE((regex_to_wildcard("abc (x[\\]]{2}z){2} def").value() == "abc x]]zx]]z def"));

    REQUIRE(
            (regex_to_wildcard("x[aA").error()
             == clp::regex_utils::ErrorCode::IncompleteCharsetStructure)
    );
    REQUIRE((
            regex_to_wildcard("x[]{2}z").error() == clp::regex_utils::ErrorCode::UnsupportedCharsets
    ));
    REQUIRE(
            (regex_to_wildcard("x[^]{2}z").error()
             == clp::regex_utils::ErrorCode::UnsupportedCharsets)
    );
    REQUIRE(
            (regex_to_wildcard("x[\\]{2}z").error()
             == clp::regex_utils::ErrorCode::UnsupportedCharsets)
    );

    // Need to set case-insensitive wildcard config for the following to work
    REQUIRE((regex_to_wildcard("[aA]").error() == clp::regex_utils::ErrorCode::UnsupportedCharsets)
    );
    REQUIRE((regex_to_wildcard("[Aa]").error() == clp::regex_utils::ErrorCode::UnsupportedCharsets)
    );
    REQUIRE(
            (regex_to_wildcard("[Ee][Xx][Cc][Ee][Pp][Tt][Ii][Oo][Nn]").error()
             == clp::regex_utils::ErrorCode::UnsupportedCharsets)
    );
    REQUIRE(
            (regex_to_wildcard("[eE][Xx][cC][eE][pP][Tt][iI][Oo]{2}[Nn]").error()
             == clp::regex_utils::ErrorCode::UnsupportedCharsets)
    );
}

TEST_CASE(
        "regex_to_wildcard_case_insensitive_wildcard",
        "[regex_utils][regex_to_wildcard][case_insensitive_wildcard]"
) {
    clp::regex_utils::RegexToWildcardTranslatorConfig config;
    config.set_case_insensitive_wildcard(true);

    REQUIRE((regex_to_wildcard("[aA]", config).value() == "a"));
    REQUIRE((regex_to_wildcard("[Aa]", config).value() == "a"));
    REQUIRE((regex_to_wildcard("[Aa][pP]{2}[Ll][eE]", config).value() == "apple"));
    REQUIRE((
            regex_to_wildcard("[Ee][Xx][Cc][Ee][Pp][Tt][Ii][Oo][Nn]", config).value() == "exception"
    ));
    REQUIRE(
            (regex_to_wildcard("[eE][Xx][cC][eE][pP][Tt][iI][Oo]{2}[Nn]", config).value()
             == "exceptioon")
    );

    REQUIRE(
            (regex_to_wildcard("[eE][Xx][cC][eE][pP][Tk][iI][Oo][Nn]", config).error()
             == clp::regex_utils::ErrorCode::UnsupportedCharsets)
    );

    // The other test cases should not be affected
    REQUIRE((regex_to_wildcard("x[y]z", config).value() == "xyz"));
    REQUIRE((regex_to_wildcard("x[y]{2}z", config).value() == "xyyz"));
    REQUIRE((regex_to_wildcard("x[+]{2}z", config).value() == "x++z"));
    REQUIRE((regex_to_wildcard("x[-]{2}z", config).value() == "x--z"));
    REQUIRE((regex_to_wildcard("x[|]{2}z", config).value() == "x||z"));
    REQUIRE((regex_to_wildcard("x[\\-]{2}z", config).value() == "x--z"));
    REQUIRE((regex_to_wildcard("x[\\^]{2}z", config).value() == "x^^z"));
    REQUIRE((regex_to_wildcard("x[\\]]{2}z", config).value() == "x]]z"));
    REQUIRE((regex_to_wildcard("x[*]{2}z", config).value() == "x\\*\\*z"));
    REQUIRE((regex_to_wildcard("x[?]{2}z", config).value() == "x\\?\\?z"));
    REQUIRE((regex_to_wildcard("x[\\\\]{2}z", config).value() == "x\\\\\\\\z"));

    REQUIRE((
            regex_to_wildcard("abc (x[*]{2}z){2} def", config).value() == "abc x\\*\\*zx\\*\\*z def"
    ));
    REQUIRE((regex_to_wildcard("abc (x[\\]]{2}z){2} def", config).value() == "abc x]]zx]]z def"));

    REQUIRE(
            (regex_to_wildcard("x[]{2}z", config).error()
             == clp::regex_utils::ErrorCode::UnsupportedCharsets)
    );
    REQUIRE(
            (regex_to_wildcard("x[^]{2}z", config).error()
             == clp::regex_utils::ErrorCode::UnsupportedCharsets)
    );
    REQUIRE(
            (regex_to_wildcard("x[\\]{2}z", config).error()
             == clp::regex_utils::ErrorCode::UnsupportedCharsets)
    );
}

TEST_CASE("regex_to_wildcard_anchor_config", "[regex_utils][regex_to_wildcard][anchor_config]") {
    // Test anchors and prefix/suffix wildcards
    clp::regex_utils::RegexToWildcardTranslatorConfig config;
    config.set_add_prefix_suffix_wildcards(true);
    REQUIRE(((regex_to_wildcard("^", config).value() == "*")));
    REQUIRE((regex_to_wildcard("$", config).value() == "*"));
    REQUIRE((regex_to_wildcard("^xyz$", config).value() == "xyz"));
    REQUIRE((regex_to_wildcard("xyz", config).value() == "*xyz*"));

    // Test in groups
    REQUIRE((regex_to_wildcard("xyz(. xyz .* zyx .)zyx", config).value() == "*xyz? xyz * zyx ?zyx*")
    );
    REQUIRE(
            (regex_to_wildcard("xyz(^. xyz .* zyx .)zyx", config).error()
             == clp::regex_utils::ErrorCode::Caret)
    );
    REQUIRE(
            (regex_to_wildcard("xyz(. xyz .* zyx .$)zyx", config).error()
             == clp::regex_utils::ErrorCode::Dollar)
    );
}

TEST_CASE("regex_trim_line_anchors", "[regex_utils][regex_trim_line_anchors]") {
    REQUIRE(regex_trim_line_anchors("").empty());
    REQUIRE((regex_trim_line_anchors("^^^hello$$$") == "^hello$"));
    REQUIRE((regex_trim_line_anchors("^^\\^hello$$$") == "^\\^hello$"));
    REQUIRE((regex_trim_line_anchors("^^^hello\\$$$") == "^hello\\$$"));
    REQUIRE((regex_trim_line_anchors("^^\\^hello\\$$$") == "^\\^hello\\$$"));
    REQUIRE((regex_trim_line_anchors("^^^hello\\\\\\\\\\\\\\$$$") == "^hello\\\\\\\\\\\\\\$$"));
    REQUIRE((regex_trim_line_anchors("^^^\\\\goodbye\\\\\\\\$$$") == "^\\\\goodbye\\\\\\\\$"));
}

TEST_CASE("regex_has_start_anchor", "[regex_utils][regex_has_start_anchor]") {
    REQUIRE_FALSE(regex_has_start_anchor(""));
    REQUIRE(regex_has_start_anchor("^hello$"));
    REQUIRE_FALSE(regex_has_start_anchor("\\^hello$"));
    REQUIRE(regex_has_start_anchor("^hello\\$"));
    REQUIRE_FALSE(regex_has_start_anchor("\\^hello\\$"));
    REQUIRE(regex_has_start_anchor("^hello\\\\\\\\\\\\\\$"));
    REQUIRE(regex_has_start_anchor("^\\\\goodbye\\\\\\\\\\\\$"));
}

TEST_CASE("regex_has_end_anchor", "[regex_utils][regex_has_end_anchor]") {
    REQUIRE_FALSE(regex_has_end_anchor(""));
    REQUIRE(regex_has_end_anchor("^hello$"));
    REQUIRE(regex_has_end_anchor("\\^hello$"));
    REQUIRE_FALSE(regex_has_end_anchor("^hello\\$"));
    REQUIRE_FALSE(regex_has_end_anchor("\\^hello\\$"));
    REQUIRE_FALSE(regex_has_end_anchor("^hello\\\\\\\\\\\\\\$"));
    REQUIRE(regex_has_end_anchor("^\\\\goodbye\\\\\\\\\\\\$"));
    REQUIRE(regex_has_end_anchor("\\\\\\\\\\\\$"));
    REQUIRE_FALSE(regex_has_end_anchor("\\\\\\\\\\\\\\$"));
}
