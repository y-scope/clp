#include "IrErrorCode.hpp"

#include <string>

using IrErrorCategory = clp::error_handling::ErrorCategory<clp::ffi::ir_stream::IrErrorCodeEnum>;
using clp::ffi::ir_stream::IrErrorCodeEnum;

template <>
auto IrErrorCategory::name() const noexcept -> char const* {
    return "clp::ffi::ir_stream::IrErrorCode";
}

template <>
auto IrErrorCategory::message(IrErrorCodeEnum error_enum) const -> std::string {
    switch (error_enum) {
        case IrErrorCodeEnum::DecodingMethodFailure:
            return "Decoding method failure.";
        case IrErrorCodeEnum::EndOfStream:
            return "End-of-stream has been reached.";
        case IrErrorCodeEnum::IncompleteStream:
            return "Incomplete IR stream.";
        default:
            return "Unknown error code enum.";
    }
}
