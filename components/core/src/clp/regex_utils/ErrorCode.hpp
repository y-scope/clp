#ifndef CLP_REGEX_UTILS_ERRORCODE_HPP
#define CLP_REGEX_UTILS_ERRORCODE_HPP

#include <cstdint>

#include <ystdlib/error_handling/ErrorCode.hpp>

namespace clp::regex_utils {
/**
 * Enum class for propagating and handling various regex utility errors.
 */
enum class ErrorCodeEnum : uint8_t {
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

using ErrorCode = ystdlib::error_handling::ErrorCode<ErrorCodeEnum>;
}  // namespace clp::regex_utils

YSTDLIB_ERROR_HANDLING_MARK_AS_ERROR_CODE_ENUM(clp::regex_utils::ErrorCodeEnum);

#endif  // CLP_REGEX_UTILS_ERRORCODE_HPP
