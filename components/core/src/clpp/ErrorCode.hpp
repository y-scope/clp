#ifndef CLPP_ERRORCODE_HPP
#define CLPP_ERRORCODE_HPP

#include <cstdint>

#include <ystdlib/error_handling/ErrorCode.hpp>

namespace clpp {
enum class ClppErrorCodeEnum : uint8_t {
    BadParam,
    Corrupt,
    Failure,
    OutOfBounds,
    Unsupported,
    UnsupportedDecomposition
};

using ClppErrorCode = ystdlib::error_handling::ErrorCode<ClppErrorCodeEnum>;
}  // namespace clpp

YSTDLIB_ERROR_HANDLING_MARK_AS_ERROR_CODE_ENUM(clpp::ClppErrorCodeEnum);

#endif  // CLPP_ERRORCODE_HPP

