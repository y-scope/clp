#include "SfaErrorCode.hpp"

#include <string>

#include <ystdlib/error_handling/ErrorCode.hpp>

using clp_s::ffi::sfa::SfaErrorCodeEnum;
using SfaErrorCategory = ystdlib::error_handling::ErrorCategory<SfaErrorCodeEnum>;

template <>
auto SfaErrorCategory::name() const noexcept -> char const* {
    return "clp_s::ffi::sfa::SfaErrorCode";
}

template <>
auto SfaErrorCategory::message(SfaErrorCodeEnum error_enum) const -> std::string {
    switch (error_enum) {
        case SfaErrorCodeEnum::DecodeRangeOutOfBounds:
            return "the requested decode range is out of bounds";
        case SfaErrorCodeEnum::IoFailure:
            return "an I/O operation failed";
        case SfaErrorCodeEnum::InvalidQuery:
            return "the search query is invalid";
        case SfaErrorCodeEnum::LogEventIndexUnavailable:
            return "a search result could not be mapped to a decoded log event";
        case SfaErrorCodeEnum::MalformedRangeIndex:
            return "the archive range index is malformed";
        case SfaErrorCodeEnum::NoMemory:
            return "insufficient memory";
        case SfaErrorCodeEnum::NotInit:
            return "the object is not initialized or has already been closed";
        case SfaErrorCodeEnum::SearchFailure:
            return "the archive search failed";
        default:
            return "unknown error code enum";
    }
}
