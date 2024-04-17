// Code from CLP

#ifndef CLP_S_ERRORCODE_HPP
#define CLP_S_ERRORCODE_HPP

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
}  // namespace clp_s

#endif  // CLP_S_ERRORCODE_HPP
