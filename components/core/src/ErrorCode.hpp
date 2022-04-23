#ifndef ERRORCODE_HPP
#define ERRORCODE_HPP

// C libraries
#include <stdint.h>

enum class ErrorCode : uint8_t {
    Success = 0,
    BadParam,
    BadParam_DB_URI,
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
    Failure_Metadata_Corrupted,
    MetadataCorrupted,
    Failure_DB_Bulk_Write,
};

/*
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
    ErrorCode_Failure_DB_Bulk_Write
} ErrorCode;
*/

#endif