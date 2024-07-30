#ifndef CLP_REGEX_UTILS_ERRORCODE_HPP
#define CLP_REGEX_UTILS_ERRORCODE_HPP

#include <cstdint>
#include <system_error>
#include <type_traits>

namespace clp::regex_utils {
/**
 * Enum class for propagating and handling various regex utility errors.
 * More detailed descriptions can be found in ErrorCode.cpp.
 */
enum class ErrorCode : uint8_t {
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

/**
 * Wrapper function to turn a regular enum class into an std::error_code.
 *
 * @param An error code enum.
 * @return The corresponding std::error_code type variable.
 */
[[nodiscard]] auto make_error_code(ErrorCode ec) -> std::error_code;
}  // namespace clp::regex_utils

namespace std {
template <>
struct is_error_code_enum<clp::regex_utils::ErrorCode> : true_type {};
}  // namespace std

#endif  // CLP_REGEX_UTILS_ERRORCODE_HPP
