#include "ErrorCode.hpp"

#include <string>

#include <ystdlib/error_handling/ErrorCode.hpp>

using clpp::ClppErrorCodeEnum;

using ClppErrorCategory = ystdlib::error_handling::ErrorCategory<ClppErrorCodeEnum>;

template <>
auto ClppErrorCategory::name() const noexcept -> char const* {
    return "Clpp";
}

template <>
auto ClppErrorCategory::message(ClppErrorCodeEnum error_enum) const -> std::string {
    switch (error_enum) {
        case ClppErrorCodeEnum::Success:
            return "Clpp Success";
        case ClppErrorCodeEnum::BadParam:
            return "Clpp BadParam";
        case ClppErrorCodeEnum::BadParamDbUri:
            return "Clpp BadParamDbUri";
        case ClppErrorCodeEnum::Corrupt:
            return "Clpp Corrupt";
        case ClppErrorCodeEnum::Errno:
            return "Clpp Errno";
        case ClppErrorCodeEnum::EndOfFile:
            return "Clpp EndOfFile";
        case ClppErrorCodeEnum::FileExists:
            return "Clpp FileExists";
        case ClppErrorCodeEnum::FileNotFound:
            return "Clpp FileNotFound";
        case ClppErrorCodeEnum::NoMem:
            return "Clpp NoMem";
        case ClppErrorCodeEnum::NotInit:
            return "Clpp NotInit";
        case ClppErrorCodeEnum::NotReady:
            return "Clpp NotReady";
        case ClppErrorCodeEnum::OutOfBounds:
            return "Clpp OutOfBounds";
        case ClppErrorCodeEnum::TooLong:
            return "Clpp TooLong";
        case ClppErrorCodeEnum::Truncated:
            return "Clpp Truncated";
        case ClppErrorCodeEnum::Unsupported:
            return "Clpp Unsupported";
        case ClppErrorCodeEnum::NoAccess:
            return "Clpp NoAccess";
        case ClppErrorCodeEnum::Failure:
            return "Clpp Failure";
        case ClppErrorCodeEnum::FailureMetadataCorrupted:
            return "Clpp FailureMetadataCorrupted";
        case ClppErrorCodeEnum::MetadataCorrupted:
            return "Clpp MetadataCorrupted";
        case ClppErrorCodeEnum::FailureDbBulkWrite:
            return "Clpp FailureDbBulkWrite";
        case ClppErrorCodeEnum::FailureNetwork:
            return "Clpp FailureNetwork";
        case ClppErrorCodeEnum::DecomposeQueryFailure:
            return "Clpp DecomposeQueryFailure";
        default:
            return "Unrecognized ClppErrorCode";
    }
}
