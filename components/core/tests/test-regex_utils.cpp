#include <string>

#include <Catch2/single_include/catch2/catch.hpp>
#include <regex_utils/regex_translation_utils.hpp>
#include <regex_utils/RegexErrorCode.hpp>
#include <regex_utils/RegexToWildcardTranslatorConfig.hpp>

using clp::regex_utils::regex_to_wildcard;
using clp::regex_utils::RegexErrorCode;
using clp::regex_utils::RegexErrorCodeEnum;
using clp::regex_utils::RegexToWildcardTranslatorConfig;

namespace {
auto test_translation_error(
        std::string const& wildcard_str,
        RegexErrorCodeEnum error,
        RegexToWildcardTranslatorConfig const* config = nullptr
) -> void;
auto test_translation_value(
        std::string const& wildcard_str,
        std::string const& regex_str,
        RegexToWildcardTranslatorConfig const* config = nullptr
) -> void;

auto test_translation_error(
        std::string const& wildcard_str,
        RegexErrorCodeEnum error,
        RegexToWildcardTranslatorConfig const* config
) -> void {
    if (config != nullptr) {
        REQUIRE((regex_to_wildcard(wildcard_str, *config).error() == RegexErrorCode{error}));
    } else {
        REQUIRE((regex_to_wildcard(wildcard_str).error() == RegexErrorCode{error}));
    }
}

auto test_translation_value(
        std::string const& wildcard_str,
        std::string const& regex_str,
        RegexToWildcardTranslatorConfig const* config
) -> void {
    if (config != nullptr) {
        REQUIRE((regex_to_wildcard(wildcard_str, *config).value() == regex_str));
    } else {
        REQUIRE((regex_to_wildcard(wildcard_str).value() == regex_str));
    }
}
}  // namespace

TEST_CASE("regex_to_wildcard_simple_translations", "[regex_utils][re2wc][simple_translations]") {
    REQUIRE(regex_to_wildcard("").value().empty());

    REQUIRE((regex_to_wildcard("xyz").value() == "xyz"));
    REQUIRE((regex_to_wildcard(". xyz .* zyx .").value() == "? xyz * zyx ?"));
    REQUIRE((regex_to_wildcard(". xyz .+ zyx .*").value() == "? xyz ?* zyx *"));
}

TEST_CASE("regex_to_wildcard_unescaped_metachar", "[regex_utils][re2wc][unescaped_metachar]") {
    test_translation_error(".? xyz .* zyx .", RegexErrorCodeEnum::UnsupportedQuestionMark);
    test_translation_error(". xyz .** zyx .", RegexErrorCodeEnum::UntranslatableStar);
    test_translation_error(". xyz .*+ zyx .", RegexErrorCodeEnum::UntranslatablePlus);
    test_translation_error(". xyz |.* zyx .", RegexErrorCodeEnum::UnsupportedPipe);
    test_translation_error(". xyz ^.* zyx .", RegexErrorCodeEnum::IllegalCaret);
    test_translation_error(". xyz $.* zyx .", RegexErrorCodeEnum::IllegalDollarSign);
}

TEST_CASE("regex_to_wildcard_escaped_metachar", "[regex_utils][re2wc][escaped_metachar]") {
    // Escape backslash is superfluous for the following set of characters
    test_translation_value("<>-_/=!", "<>-_/=!");
    test_translation_value(R"(\<\>\-\_\/\=\!)", "<>-_/=!");
    // Test the full escape sequences set
    test_translation_value(
            R"(\*\+\?\|\^\$\.\{\}\[\]\(\)\<\>\-\_\/\=\!\\)",
            R"(\*+\?|^$.{}[]()<>-_/=!\\)"
    );
    // Test unsupported escape sequences
    test_translation_error("abc\\Qdefghi\\Ejkl", RegexErrorCodeEnum::IllegalEscapeSequence);
}

TEST_CASE("regex_to_wildcard_charset", "[regex_utils][re2wc][charset]") {
    test_translation_value("x[y]z", "xyz");
    test_translation_value("x[\\^]z", "x^z");
    test_translation_value("x[\\]]z", "x]z");
    test_translation_value("x[-]z", "x-z");
    test_translation_value("x[\\-]z", "x-z");
    test_translation_value("x[\\\\]z", "x\\\\z");
    test_translation_value(R"([a][b][\^][-][\-][\]][\\][c][d])", "ab^--]\\\\cd");

    test_translation_error("x[]y", RegexErrorCodeEnum::UnsupportedCharsetPattern);
    test_translation_error("x[a-z]y", RegexErrorCodeEnum::UnsupportedCharsetPattern);
    test_translation_error("x[^^]y", RegexErrorCodeEnum::UnsupportedCharsetPattern);
    test_translation_error("x[^0-9]y", RegexErrorCodeEnum::UnsupportedCharsetPattern);
    test_translation_error("[xX][yY]", RegexErrorCodeEnum::UnsupportedCharsetPattern);
    test_translation_error("ch:[a-zA-Z0-9]", RegexErrorCodeEnum::UnsupportedCharsetPattern);

    test_translation_error("[\\", RegexErrorCodeEnum::IncompleteCharsetStructure);
    test_translation_error("[\\\\", RegexErrorCodeEnum::IncompleteCharsetStructure);
    test_translation_error("[xX", RegexErrorCodeEnum::IncompleteCharsetStructure);
    test_translation_error("ch:[a-zA-Z0-9", RegexErrorCodeEnum::IncompleteCharsetStructure);
}

TEST_CASE("regex_to_wildcard_case_insensitive_config", "[regex_utils][re2wc][case_insensitive]") {
    RegexToWildcardTranslatorConfig const config{/*case_insensitive_wildcard=*/true, false};
    test_translation_value("[xX][yY]", "xy", &config);
    test_translation_value("[Yy][Xx]", "yx", &config);
    test_translation_value("[aA][Bb][Cc]", "abc", &config);
    test_translation_value("[aA][Bb][\\^][-][\\]][Cc][dD]", "ab^-]cd", &config);

    test_translation_error("[xX", RegexErrorCodeEnum::IncompleteCharsetStructure);
    test_translation_error(
            "[aA][Bb][^[-[\\[Cc[dD",
            RegexErrorCodeEnum::IncompleteCharsetStructure,
            &config
    );
    test_translation_error("ch:[a-zA-Z0-9]", RegexErrorCodeEnum::UnsupportedCharsetPattern);
    test_translation_error(
            "[aA][Bb][^[-[\\[Cc[dD]",
            RegexErrorCodeEnum::UnsupportedCharsetPattern,
            &config
    );
}

TEST_CASE("regex_to_wildcard_anchor_config", "[regex_utils][re2wc][anchor_config]") {
    // Test anchors and prefix/suffix wildcards
    RegexToWildcardTranslatorConfig const config{false, /*add_prefix_suffix_wildcards=*/true};
    test_translation_value("^", "*", &config);
    test_translation_value("$", "*", &config);
    test_translation_value("^xyz$", "xyz", &config);
    test_translation_value("xyz", "*xyz*", &config);
    test_translation_value("xyz$$", "*xyz", &config);

    test_translation_error("xyz$zyx$", RegexErrorCodeEnum::IllegalDollarSign, &config);
}
