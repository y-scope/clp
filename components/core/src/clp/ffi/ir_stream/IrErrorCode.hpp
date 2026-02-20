#ifndef CLP_IRERRORCODE_HPP
#define CLP_IRERRORCODE_HPP

#include <cstdint>

#include <ystdlib/error_handling/ErrorCode.hpp>

namespace clp::ffi::ir_stream {
/**
 * This enum class represents error codes for IR stream deserialization
 */
enum class IrErrorCodeEnum : uint8_t {
    DecodingMethodFailure,
    EndOfStream,
    IncompleteStream,
};

using IrErrorCode = ystdlib::error_handling::ErrorCode<IrErrorCodeEnum>;
}  // namespace clp::ffi::ir_stream

YSTDLIB_ERROR_HANDLING_MARK_AS_ERROR_CODE_ENUM(clp::ffi::ir_stream::IrErrorCodeEnum);

#endif  // CLP_IRERRORCODE_HPP
