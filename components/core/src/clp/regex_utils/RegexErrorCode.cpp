#include "RegexErrorCode.hpp"

#include <string>

using clp::regex_utils::RegexErrorCategory;
using clp::regex_utils::RegexErrorCodeEnum;

template <>
auto RegexErrorCategory::name() const noexcept -> char const* {
    return "clp::regex_utils::RegexErrorCode";
}

template <>
auto RegexErrorCategory::message(RegexErrorCodeEnum error_enum) const -> std::string {
    switch (error_enum) {
        case RegexErrorCodeEnum::Success:
            return "Success.";

        case RegexErrorCodeEnum::IllegalState:
            return "Unrecognized state.";

        case RegexErrorCodeEnum::UntranslatableStar:
            return "Unable to express regex quantifier `*` in wildcard, which repeats a token for "
                   "zero or more occurences, unless it is combined with a wildcard `.`";

        case RegexErrorCodeEnum::UntranslatablePlus:
            return "Unable to express regex quantifier `+` in wildcard, which repeats a token for "
                   "one or more occurences, unless it is combined with a wildcard `.`";

        case RegexErrorCodeEnum::UnsupportedQuestionMark:
            return "Unable to express regex quantifier `?` in wildcard, which makes the preceding "
                   "token optional, unless the translator supports returning a list of possible "
                   "wildcard translations.";

        case RegexErrorCodeEnum::UnsupportedPipe:
            return "Unable to express regex OR `|` in wildcard, which allows the query string to "
                   "match a single token out of a series of options, unless the translator "
                   "supports returning a list of possible wildcard translations.";

        case RegexErrorCodeEnum::IllegalCaret:
            return "Failed to translate due to start anchor `^` in the middle of the string.";

        case RegexErrorCodeEnum::IllegalDollarSign:
            return "Failed to translate due to end anchor `$` in the middle of the string.";

        case RegexErrorCodeEnum::IllegalEscapeSequence:
            return "Currently only supports escape sequences that are used to suppress special "
                   "meanings of regex metacharacters. Alphanumeric characters are disallowed.";

        case RegexErrorCodeEnum::UnmatchedParenthesis:
            return "Unmatched opening `(` or closing `)`.";

        case RegexErrorCodeEnum::IncompleteCharsetStructure:
            return "Unmatched closing `]` at the end of the string.";

        case RegexErrorCodeEnum::UnsupportedCharsetPattern:
            return "Currently only supports character set that can be reduced to a single "
                   "character.";
        default:
            return "Unknown error code enum.";
    }
}
