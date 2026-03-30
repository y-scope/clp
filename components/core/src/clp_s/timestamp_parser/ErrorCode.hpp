#ifndef CLP_S_TIMESTAMP_PARSER_ERRORCODE_HPP
#define CLP_S_TIMESTAMP_PARSER_ERRORCODE_HPP

#include <cstdint>

#include <ystdlib/error_handling/ErrorCode.hpp>

namespace clp_s::timestamp_parser {
enum class ErrorCodeEnum : uint8_t {
    InvalidTimestampPattern = 1,
    IncompatibleTimestampPattern,
    InvalidDate,
    InvalidTimezoneOffset,
    InvalidEscapeSequence,
    InvalidCharacter
};

using ErrorCode = ystdlib::error_handling::ErrorCode<ErrorCodeEnum>;
}  // namespace clp_s::timestamp_parser

YSTDLIB_ERROR_HANDLING_MARK_AS_ERROR_CODE_ENUM(clp_s::timestamp_parser::ErrorCodeEnum);

#endif  // CLP_S_TIMESTAMP_PARSER_ERRORCODE_HPP
