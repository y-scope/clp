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
        case ClppErrorCodeEnum::BadParam:
            return "Clpp BadParam";
        case ClppErrorCodeEnum::Corrupt:
            return "Clpp Corrupt";
        case ClppErrorCodeEnum::Failure:
            return "Clpp Failure";
        case ClppErrorCodeEnum::OutOfBounds:
            return "Clpp OutOfBounds";
        case ClppErrorCodeEnum::Unsupported:
            return "Clpp Unsupported";
        case ClppErrorCodeEnum::UnsupportedDecomposition:
            return "clpp query decomposition is not supported in this build; rebuild with"
                   " -DCLP_BUILD_CLPP_DECOMPOSITION=ON";
        default:
            return "Unrecognized ClppErrorCode";
    }
}
