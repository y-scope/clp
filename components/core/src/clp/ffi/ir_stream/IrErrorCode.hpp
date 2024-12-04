#ifndef CLP_IRERRORCODE_HPP
#define CLP_IRERRORCODE_HPP

#include <cstdint>

#include "../../error_handling/ErrorCode.hpp"

namespace clp::ffi::ir_stream {
/**
 * This enum class represents all possible error codes related to serializing or deserializing CLP
 * IR streams.
 */
enum class IrErrorCodeEnum : uint8_t {
    DecodingMethodFailure,
    EndOfStream,
    IncompleteStream,
};

using IrErrorCode = clp::error_handling::ErrorCode<IrErrorCodeEnum>;
}  // namespace clp::ffi::ir_stream

CLP_ERROR_HANDLING_MARK_AS_ERROR_CODE_ENUM(clp::ffi::ir_stream::IrErrorCodeEnum);

#endif  // CLP_IRERRORCODE_HPP
