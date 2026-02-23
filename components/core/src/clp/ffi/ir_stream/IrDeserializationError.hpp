#ifndef CLP_IR_DESERIALIZATION_ERROR_HPP
#define CLP_IR_DESERIALIZATION_ERROR_HPP

#include <cstdint>

#include <ystdlib/error_handling/ErrorCode.hpp>

namespace clp::ffi::ir_stream {
/**
 * Error code enum for IR stream deserialization.
 */
enum class IrDeserializationErrorEnum : uint8_t {
    DecodingMethodFailure,
    EndOfStream,
    IncompleteStream,
    CorruptedIR,
    UnsupportedFormat,
};

using IrDeserializationError = ystdlib::error_handling::ErrorCode<IrDeserializationErrorEnum>;
}  // namespace clp::ffi::ir_stream

YSTDLIB_ERROR_HANDLING_MARK_AS_ERROR_CODE_ENUM(clp::ffi::ir_stream::IrDeserializationErrorEnum);

#endif  // CLP_IR_DESERIALIZATION_ERROR_HPP
