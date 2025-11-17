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
            return "Encountered timestamp pattern with invalid syntax.";
        case ErrorCodeEnum::IncompatibleTimestampPattern:
            return "Timestamp pattern does not match the format of the given timestamp.";
        case ErrorCodeEnum::InvalidDate:
            return "Timestamp was parsed successfully, but did not yield a valid date.";
        case ErrorCodeEnum::FormatSpecifierNotImplemented:
            return "Encountered unimplemented format specifier in timestamp pattern.";
        default:
            return "Unknown error code enum.";
    }
}
