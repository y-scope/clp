#ifndef CLP_ERRORCODE_HPP
#define CLP_ERRORCODE_HPP

namespace clp {
typedef enum {
    ErrorCode_Success = 0,
    ErrorCode_BadParam,
    ErrorCode_BadParam_DB_URI,
    ErrorCode_Corrupt,
    ErrorCode_errno,
    ErrorCode_EndOfFile,
    ErrorCode_FileExists,
    ErrorCode_FileNotFound,
    ErrorCode_NoMem,
    ErrorCode_NotInit,
    ErrorCode_NotReady,
    ErrorCode_OutOfBounds,
    ErrorCode_TooLong,
    ErrorCode_Truncated,
    ErrorCode_Unsupported,
    ErrorCode_NoAccess,
    ErrorCode_Failure,
    ErrorCode_Failure_Metadata_Corrupted,
    ErrorCode_MetadataCorrupted,
    ErrorCode_Failure_DB_Bulk_Write,
    ErrorCode_Failure_Network,
} ErrorCode;
}  // namespace clp

#endif  // CLP_ERRORCODE_HPP
