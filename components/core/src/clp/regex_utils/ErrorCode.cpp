#include "ErrorCode.hpp"

#include <string>

#include <ystdlib/error_handling/ErrorCode.hpp>

using clp::regex_utils::ErrorCodeEnum;
using ErrorCategory = ystdlib::error_handling::ErrorCategory<ErrorCodeEnum>;

template <>
auto ErrorCategory::name() const noexcept -> char const* {
    return "clp::regex_utils::ErrorCode";
}

template <>
auto ErrorCategory::message(ErrorCodeEnum error_enum) const -> std::string {
    switch (error_enum) {
        case ErrorCodeEnum::Success:
            return "Success.";

        case ErrorCodeEnum::IllegalState:
            return "Unrecognized state.";

        case ErrorCodeEnum::UntranslatableStar:
            return "Unable to express regex quantifier `*` in wildcard, which repeats a token for "
                   "zero or more occurrences, unless it is combined with a wildcard `.`";

        case ErrorCodeEnum::UntranslatablePlus:
            return "Unable to express regex quantifier `+` in wildcard, which repeats a token for "
                   "one or more occurrences, unless it is combined with a wildcard `.`";

        case ErrorCodeEnum::UnsupportedQuestionMark:
            return "Unable to express regex quantifier `?` in wildcard, which makes the preceding "
                   "token optional, unless the translator supports returning a list of possible "
                   "wildcard translations.";

        case ErrorCodeEnum::UnsupportedPipe:
            return "Unable to express regex OR `|` in wildcard, which allows the query string to "
                   "match a single token out of a series of options, unless the translator "
                   "supports returning a list of possible wildcard translations.";

        case ErrorCodeEnum::IllegalCaret:
            return "Failed to translate due to start anchor `^` in the middle of the string.";

        case ErrorCodeEnum::IllegalDollarSign:
            return "Failed to translate due to end anchor `$` in the middle of the string.";

        case ErrorCodeEnum::IllegalEscapeSequence:
            return "Currently only supports escape sequences that are used to suppress special "
                   "meanings of regex metacharacters. Alphanumeric characters are disallowed.";

        case ErrorCodeEnum::UnmatchedParenthesis:
            return "Unmatched opening `(` or closing `)`.";

        case ErrorCodeEnum::IncompleteCharsetStructure:
            return "Unmatched closing `]` at the end of the string.";

        case ErrorCodeEnum::UnsupportedCharsetPattern:
            return "Currently only supports character set that can be reduced to a single "
                   "character.";
        default:
            return "Unknown error code enum.";
    }
}
