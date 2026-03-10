#ifndef CLP_S_FFI_SFA_SFAERRORCODE_HPP
#define CLP_S_FFI_SFA_SFAERRORCODE_HPP

#include <cstdint>

#include <ystdlib/error_handling/ErrorCode.hpp>

namespace clp_s::ffi::sfa {
/**
 * Error code enum for SFA API operations.
 */
enum class SfaErrorCodeEnum : uint8_t {
    EndOfStream,
    IoFailure,
    NoMemory,
    NotInit,
};

using SfaErrorCode = ystdlib::error_handling::ErrorCode<SfaErrorCodeEnum>;
}  // namespace clp_s::ffi::sfa

YSTDLIB_ERROR_HANDLING_MARK_AS_ERROR_CODE_ENUM(clp_s::ffi::sfa::SfaErrorCodeEnum);

#endif  // CLP_S_FFI_SFA_SFAERRORCODE_HPP
