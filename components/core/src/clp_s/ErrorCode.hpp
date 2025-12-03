// Code from CLP

#ifndef CLP_S_ERRORCODE_HPP
#define CLP_S_ERRORCODE_HPP

#include <cstdint>

#include <ystdlib/error_handling/ErrorCode.hpp>

namespace clp_s {
typedef enum {
    ErrorCodeSuccess = 0,
    ErrorCodeBadParam,
    ErrorCodeBadParamDbUri,
    ErrorCodeCorrupt,
    ErrorCodeErrno,
    ErrorCodeEndOfFile,
    ErrorCodeFileExists,
    ErrorCodeFileNotFound,
    ErrorCodeNoMem,
    ErrorCodeNotInit,
    ErrorCodeNotReady,
    ErrorCodeOutOfBounds,
    ErrorCodeTooLong,
    ErrorCodeTruncated,
    ErrorCodeUnsupported,
    ErrorCodeNoAccess,
    ErrorCodeFailure,
    ErrorCodeFailureMetadataCorrupted,
    ErrorCodeMetadataCorrupted,
    ErrorCodeFailureDbBulkWrite,
    ErrorCodeFailureNetwork,
} ErrorCode;

enum class ClpsErrorCodeEnum : uint8_t {
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
};

using ClpsErrorCode = ystdlib::error_handling::ErrorCode<ClpsErrorCodeEnum>;
}  // namespace clp_s

YSTDLIB_ERROR_HANDLING_MARK_AS_ERROR_CODE_ENUM(clp_s::ClpsErrorCodeEnum);

#endif  // CLP_S_ERRORCODE_HPP
