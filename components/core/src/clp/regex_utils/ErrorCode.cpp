#include "regex_utils/ErrorCode.hpp"

#include <string>
#include <string_view>
#include <system_error>

using std::error_category;
using std::error_code;
using std::string;
using std::string_view;

namespace clp::regex_utils {

/**
 * Class for giving the error codes more detailed string descriptions.
 * This class does not need to be seen outside the std error code wrapper implementation.
 */
class ErrorCodeCategory : public error_category {
public:
    /**
     * @return The class of errors.
     */
    [[nodiscard]] char const* name() const noexcept override;

    /**
     * @param The error code encoded in int.
     * @return The descriptive message for the error.
     */
    [[nodiscard]] string message(int ev) const override;
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

        case ErrorCode::Star:
            return "Failed to translate due to metachar `*` (zero or more occurences).";

        case ErrorCode::Plus:
            return "Failed to translate due to metachar `+` (one or more occurences).";

        case ErrorCode::Question:
            return "Currently does not support returning a list of wildcard translations. The "
                   "metachar `?` (lazy match) may be supported in the future.";

        case ErrorCode::Pipe:
            return "Currently does not support returning a list of wildcard translations. The "
                   "regex OR condition feature may be supported in the future.";

        case ErrorCode::Caret:
            return "Failed to translate due to start anchor `^` in the middle of the string.";

        case ErrorCode::Dollar:
            return "Failed to translate due to end anchor `$` in the middle of the string.";

        case ErrorCode::DisallowedEscapeSequence:
            return "Disallowed escape sequence.";

        case ErrorCode::UnmatchedParenthesis:
            return "Unmatched opening `(` or closing `)`.";

        case ErrorCode::UnsupportedCharsets:
            return "Currently only supports case-insensitive single-char charset (i.e. [aA] [bB]).";

        case ErrorCode::IncompleteCharsetStructure:
            return "Unmatched closing `]` at the end of the string.";

        case ErrorCode::UnsupportedQuantifier:
            return "Currently only supports exact positive number of repetitions in regex syntax.";

        case ErrorCode::TokenUnquantifiable:
            return "The preceding token is not quantifiable.";

        default:
            return "(unrecognized error)";
    }
}

ErrorCodeCategory const cTheErrorCodeCategory{};

auto make_error_code(ErrorCode e) -> error_code {
    return {static_cast<int>(e), cTheErrorCodeCategory};
}

}  // namespace clp::regex_utils
