#include "ErrorCode.hpp"

#include <string>

#include <ystdlib/error_handling/ErrorCode.hpp>

using clp_s::ClpsErrorCodeEnum;

using ClpsErrorCategory = ystdlib::error_handling::ErrorCategory<ClpsErrorCodeEnum>;

template <>
auto ClpsErrorCategory::name() const noexcept -> char const* {
    return "Clp-s";
}

template <>
auto ClpsErrorCategory::message(ClpsErrorCodeEnum error_enum) const -> std::string {
    switch (error_enum) {
        case ClpsErrorCodeEnum::Success:
            return "Clps Success";
        case ClpsErrorCodeEnum::BadParam:
            return "Clps BadParam";
        case ClpsErrorCodeEnum::BadParamDbUri:
            return "Clps BadParamDbUri";
        case ClpsErrorCodeEnum::Corrupt:
            return "Clps Corrupt";
        case ClpsErrorCodeEnum::Errno:
            return "Clps Errno";
        case ClpsErrorCodeEnum::EndOfFile:
            return "Clps EndOfFile";
        case ClpsErrorCodeEnum::FileExists:
            return "Clps FileExists";
        case ClpsErrorCodeEnum::FileNotFound:
            return "Clps FileNotFound";
        case ClpsErrorCodeEnum::NoMem:
            return "Clps NoMem";
        case ClpsErrorCodeEnum::NotInit:
            return "Clps NotInit";
        case ClpsErrorCodeEnum::NotReady:
            return "Clps NotReady";
        case ClpsErrorCodeEnum::OutOfBounds:
            return "Clps OutOfBounds";
        case ClpsErrorCodeEnum::TooLong:
            return "Clps TooLong";
        case ClpsErrorCodeEnum::Truncated:
            return "Clps Truncated";
        case ClpsErrorCodeEnum::Unsupported:
            return "Clps Unsupported";
        case ClpsErrorCodeEnum::NoAccess:
            return "Clps NoAccess";
        case ClpsErrorCodeEnum::Failure:
            return "Clps Failure";
        case ClpsErrorCodeEnum::FailureMetadataCorrupted:
            return "Clps FailureMetadataCorrupted";
        case ClpsErrorCodeEnum::MetadataCorrupted:
            return "Clps MetadataCorrupted";
        case ClpsErrorCodeEnum::FailureDbBulkWrite:
            return "Clps FailureDbBulkWrite";
        case ClpsErrorCodeEnum::FailureNetwork:
            return "Clps FailureNetwork";
        default:
            return "Unrecognized ClpsErrorCode";
    }
}
