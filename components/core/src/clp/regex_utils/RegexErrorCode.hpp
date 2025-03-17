#ifndef CLP_REGEX_UTILS_REGEXERRORCODE_HPP
#define CLP_REGEX_UTILS_REGEXERRORCODE_HPP

#include <cstdint>

#include <ystdlib/error_handling/ErrorCode.hpp>

namespace clp::regex_utils {
/**
 * Enum class for propagating and handling various regex utility errors.
 */
enum class RegexErrorCodeEnum : uint8_t {
    Success = 0,
    IllegalState,
    UntranslatableStar,
    UntranslatablePlus,
    UnsupportedQuestionMark,
    UnsupportedPipe,
    IllegalCaret,
    IllegalDollarSign,
    IllegalEscapeSequence,
    UnmatchedParenthesis,
    IncompleteCharsetStructure,
    UnsupportedCharsetPattern,
};

using RegexErrorCode = ystdlib::error_handling::ErrorCode<RegexErrorCodeEnum>;
using RegexErrorCategory = ystdlib::error_handling::ErrorCategory<RegexErrorCodeEnum>;
}  // namespace clp::regex_utils

YSTDLIB_ERROR_HANDLING_MARK_AS_ERROR_CODE_ENUM(clp::regex_utils::RegexErrorCodeEnum);

#endif  // CLP_REGEX_UTILS_REGEXERRORCODE_HPP
