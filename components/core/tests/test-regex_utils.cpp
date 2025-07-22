#include <string>

#include <catch2/catch.hpp>
#include <regex_utils/ErrorCode.hpp>
#include <regex_utils/regex_translation_utils.hpp>
#include <regex_utils/RegexToWildcardTranslatorConfig.hpp>

using clp::regex_utils::ErrorCode;
using clp::regex_utils::ErrorCodeEnum;
using clp::regex_utils::regex_to_wildcard;
using clp::regex_utils::RegexToWildcardTranslatorConfig;

namespace {
auto test_translation_error(
        std::string const& regex_str,
        ErrorCodeEnum error,
        RegexToWildcardTranslatorConfig const* config = nullptr
) -> void;
auto test_translation_value(
        std::string const& regex_str,
        std::string const& wildcard_str,
        RegexToWildcardTranslatorConfig const* config = nullptr
) -> void;

auto test_translation_error(
        std::string const& regex_str,
        ErrorCodeEnum error,
        RegexToWildcardTranslatorConfig const* config
) -> void {
    if (config != nullptr) {
        REQUIRE((regex_to_wildcard(regex_str, *config).error() == ErrorCode{error}));
    } else {
        REQUIRE((regex_to_wildcard(regex_str).error() == ErrorCode{error}));
    }
}

auto test_translation_value(
        std::string const& regex_str,
        std::string const& wildcard_str,
        RegexToWildcardTranslatorConfig const* config
) -> void {
    if (config != nullptr) {
        REQUIRE((regex_to_wildcard(regex_str, *config).value() == wildcard_str));
    } else {
        REQUIRE((regex_to_wildcard(regex_str).value() == wildcard_str));
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
    test_translation_error(".? xyz .* zyx .", ErrorCodeEnum::UnsupportedQuestionMark);
    test_translation_error(". xyz .** zyx .", ErrorCodeEnum::UntranslatableStar);
    test_translation_error(". xyz .*+ zyx .", ErrorCodeEnum::UntranslatablePlus);
    test_translation_error(". xyz |.* zyx .", ErrorCodeEnum::UnsupportedPipe);
    test_translation_error(". xyz ^.* zyx .", ErrorCodeEnum::IllegalCaret);
    test_translation_error(". xyz $.* zyx .", ErrorCodeEnum::IllegalDollarSign);
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
    test_translation_error("abc\\Qdefghi\\Ejkl", ErrorCodeEnum::IllegalEscapeSequence);
}

TEST_CASE("regex_to_wildcard_charset", "[regex_utils][re2wc][charset]") {
    test_translation_value("x[y]z", "xyz");
    test_translation_value("x[\\^]z", "x^z");
    test_translation_value("x[\\]]z", "x]z");
    test_translation_value("x[-]z", "x-z");
    test_translation_value("x[\\-]z", "x-z");
    test_translation_value("x[\\\\]z", "x\\\\z");
    test_translation_value(R"([a][b][\^][-][\-][\]][\\][c][d])", "ab^--]\\\\cd");

    test_translation_error("x[]y", ErrorCodeEnum::UnsupportedCharsetPattern);
    test_translation_error("x[a-z]y", ErrorCodeEnum::UnsupportedCharsetPattern);
    test_translation_error("x[^^]y", ErrorCodeEnum::UnsupportedCharsetPattern);
    test_translation_error("x[^0-9]y", ErrorCodeEnum::UnsupportedCharsetPattern);
    test_translation_error("[xX][yY]", ErrorCodeEnum::UnsupportedCharsetPattern);
    test_translation_error("ch:[a-zA-Z0-9]", ErrorCodeEnum::UnsupportedCharsetPattern);

    test_translation_error("[\\", ErrorCodeEnum::IncompleteCharsetStructure);
    test_translation_error("[\\\\", ErrorCodeEnum::IncompleteCharsetStructure);
    test_translation_error("[xX", ErrorCodeEnum::IncompleteCharsetStructure);
    test_translation_error("ch:[a-zA-Z0-9", ErrorCodeEnum::IncompleteCharsetStructure);
}

TEST_CASE("regex_to_wildcard_case_insensitive_config", "[regex_utils][re2wc][case_insensitive]") {
    RegexToWildcardTranslatorConfig const config{/*case_insensitive_wildcard=*/true, false};
    test_translation_value("[xX][yY]", "xy", &config);
    test_translation_value("[Yy][Xx]", "yx", &config);
    test_translation_value("[aA][Bb][Cc]", "abc", &config);
    test_translation_value("[aA][Bb][\\^][-][\\]][Cc][dD]", "ab^-]cd", &config);

    test_translation_error("[xX", ErrorCodeEnum::IncompleteCharsetStructure);
    test_translation_error(
            "[aA][Bb][^[-[\\[Cc[dD",
            ErrorCodeEnum::IncompleteCharsetStructure,
            &config
    );
    test_translation_error("ch:[a-zA-Z0-9]", ErrorCodeEnum::UnsupportedCharsetPattern);
    test_translation_error(
            "[aA][Bb][^[-[\\[Cc[dD]",
            ErrorCodeEnum::UnsupportedCharsetPattern,
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

    test_translation_error("xyz$zyx$", ErrorCodeEnum::IllegalDollarSign, &config);
}
