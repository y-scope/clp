#include "TimestampParserErrorCode.hpp"

#include <string>

#include <ystdlib/error_handling/ErrorCode.hpp>

namespace {
using clp_s::TimestampParserErrorCodeEnum;
using ErrorCategory = ystdlib::error_handling::ErrorCategory<TimestampParserErrorCodeEnum>;
}  // namespace

template <>
auto ErrorCategory::name() const noexcept -> char const* {
    return "clp_s::TimestampParserErrorCode";
}

template <>
auto ErrorCategory::message(TimestampParserErrorCodeEnum error_enum) const -> std::string {
    switch (error_enum) {
        case TimestampParserErrorCodeEnum::InvalidTimestampPattern:
            return "Encountered timestamp pattern with invalid syntax.";
        case TimestampParserErrorCodeEnum::IncompatibleTimestampPattern:
            return "Timestamp pattern does not match the format of the given timestamp.";
        case TimestampParserErrorCodeEnum::InvalidDate:
        return "Timestamp was parsed successfully, but did not yield a valid "
               "date." case TimestampParserErrorCodeEnum::FormatSpecifierNotImplemented:
            return "Encountered unimplemented format specifier in timestamp pattern." default
                    : return "Unknown error code enum.";
    }
}
