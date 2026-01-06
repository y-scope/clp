#include "ErrorCode.hpp"

#include <string>

#include <ystdlib/error_handling/ErrorCode.hpp>

namespace {
using clp_s::timestamp_parser::ErrorCodeEnum;
using ErrorCategory = ystdlib::error_handling::ErrorCategory<ErrorCodeEnum>;
}  // namespace

template <>
auto ErrorCategory::name() const noexcept -> char const* {
    return "clp_s::timestamp_parser::ErrorCode";
}

template <>
auto ErrorCategory::message(ErrorCodeEnum error_enum) const -> std::string {
    switch (error_enum) {
        case ErrorCodeEnum::InvalidTimestampPattern:
            return "Encountered timestamp pattern with invalid syntax";
        case ErrorCodeEnum::IncompatibleTimestampPattern:
            return "Timestamp pattern does not match the format of the given timestamp";
        case ErrorCodeEnum::InvalidDate:
            return "Timestamp was parsed successfully, but did not yield a valid date";
        case ErrorCodeEnum::InvalidTimezoneOffset:
            return "Encountered invalid data when expecting a timezone offset";
        case ErrorCodeEnum::InvalidEscapeSequence:
            return "Timestamp pattern contains an unsupported escape sequence";
        case ErrorCodeEnum::InvalidCharacter:
            return "Timestamp pattern contains an unsupported character";
        default:
            return "Unknown error code enum";
    }
}
