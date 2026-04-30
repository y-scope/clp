#ifndef CLPP_ERRORCODE_HPP
#define CLPP_ERRORCODE_HPP

#include <cstdint>

#include <ystdlib/error_handling/ErrorCode.hpp>

namespace clpp {
enum class ClppErrorCodeEnum : uint8_t {
    Success = 0,
    BadParam,
    BadParamDbUri,
    Corrupt,
    Errno,
    EndOfFile,
    FileExists,
    FileNotFound,
    NoMem,
    NotInit,
    NotReady,
    OutOfBounds,
    TooLong,
    Truncated,
    Unsupported,
    NoAccess,
    Failure,
    FailureMetadataCorrupted,
    MetadataCorrupted,
    FailureDbBulkWrite,
    FailureNetwork,
    DecomposeQueryFailure
};

using ClppErrorCode = ystdlib::error_handling::ErrorCode<ClppErrorCodeEnum>;
}  // namespace clpp

YSTDLIB_ERROR_HANDLING_MARK_AS_ERROR_CODE_ENUM(clpp::ClppErrorCodeEnum);

#endif  // CLPP_ERRORCODE_HPP

