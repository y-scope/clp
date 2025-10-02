#ifndef CLP_S_TIMESTAMPPARSERERRORCODE_HPP
#define CLP_S_TIMESTAMPPARSERERRORCODE_HPP

#include <cstdint>

#include <ystdlib/error_handling/ErrorCode.hpp>

namespace clp_s {
enum class TimestampParserErrorCodeEnum : uint8_t {
    InvalidTimestampPattern = 1,
    IncompatibleTimestampPattern,
    InvalidDate,
    FormatSpecifierNotImplemented
};

using TimestampParserErrorCode = ystdlib::error_handling::ErrorCode<TimestampParserErrorCodeEnum>;
}  // namespace clp_s

YSTDLIB_ERROR_HANDLING_MARK_AS_ERROR_CODE_ENUM(clp_s::TimestampParserErrorCodeEnum);

#endif  // CLP_S_TIMESTAMPPARSERERRORCODE_HPP
