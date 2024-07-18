#include "regex_utils/ErrorCode.hpp"

#include <string>
#include <string_view>
#include <system_error>

using std::error_category;
using std::error_code;
using std::string;
using std::string_view;

namespace clp::regex_utils {
namespace {
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

        case ErrorCode::UnmatchedParenthesis:
            return "Unmatched opening `(` or closing `)`.";

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
