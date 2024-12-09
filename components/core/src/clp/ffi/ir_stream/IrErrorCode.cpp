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
            return "The decoding method failed.";
        case IrErrorCodeEnum::EndOfStream:
            return "The end-of-stream IR unit has already been consumed.";
        case IrErrorCodeEnum::IncompleteStream:
            return "The IR stream ended with a truncated IR unit or did not terminate with an "
                   "end-of-stream IR unit.";
        default:
            return "Unknown error code enum.";
    }
}
