#include "regex_utils/ErrorCode.hpp"

#include <string>
#include <string_view>
#include <system_error>

namespace clp::regex_utils {
using std::error_code;

namespace {
using std::error_category;
using std::string;
using std::string_view;

/**
 * Class for giving the error codes more detailed string descriptions.
 */
class ErrorCodeCategory : public error_category {
public:
    /**
     * @return The class of errors.
     */
    [[nodiscard]] auto name() const noexcept -> char const* override;

    /**
     * @param The error code encoded in int.
     * @return The descriptive message for the error.
     */
    [[nodiscard]] auto message(int ev) const -> string override;
};

auto ErrorCodeCategory::name() const noexcept -> char const* {
    return "regex utility";
}

auto ErrorCodeCategory::message(int ev) const -> string {
    switch (static_cast<ErrorCode>(ev)) {
        case ErrorCode::Success:
            return "Success.";

        case ErrorCode::IllegalState:
            return "Unrecognized state.";

        case ErrorCode::UntranslatableStar:
            return "Unable to express regex quantifier `*` in wildcard, which repeats a token for "
                   "zero or more occurences, unless it is combined with a wildcard `.`";

        case ErrorCode::UntranslatablePlus:
            return "Unable to express regex quantifier `+` in wildcard, which repeats a token for "
                   "one or more occurences, unless it is combined with a wildcard `.`";

        case ErrorCode::UnsupportedQuestionMark:
            return "Unable to express regex quantifier `?` in wildcard, which makes the preceding "
                   "token optional, unless the translator supports returning a list of possible "
                   "wildcard translations.";

        case ErrorCode::UnsupportedPipe:
            return "Unable to express regex OR `|` in wildcard, which allows the query string to "
                   "match a single token out of a series of options, unless the translator "
                   "supports returning a list of possible wildcard translations.";

        case ErrorCode::IllegalCaret:
            return "Failed to translate due to start anchor `^` in the middle of the string.";

        case ErrorCode::IllegalDollarSign:
            return "Failed to translate due to end anchor `$` in the middle of the string.";

        case ErrorCode::IllegalEscapeSequence:
            return "Currently only supports escape sequences that are used to suppress special "
                   "meanings of regex metacharacters. Alphanumeric characters are disallowed.";

        case ErrorCode::UnmatchedParenthesis:
            return "Unmatched opening `(` or closing `)`.";

        case ErrorCode::IncompleteCharsetStructure:
            return "Unmatched closing `]` at the end of the string.";

        case ErrorCode::UnsupportedCharsetPattern:
            return "Currently only supports character set that can be reduced to a single "
                   "character.";

        default:
            return "(unrecognized error)";
    }
}

ErrorCodeCategory const cErrorCodeCategoryInstance;
}  // namespace

auto make_error_code(ErrorCode e) -> error_code {
    return {static_cast<int>(e), cErrorCodeCategoryInstance};
}
}  // namespace clp::regex_utils
