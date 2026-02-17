#include "IrErrorCode.hpp"

#include <string>

#include <ystdlib/error_handling/ErrorCode.hpp>

using clp::ffi::ir_stream::IrErrorCodeEnum;
using IrErrorCategory = ystdlib::error_handling::ErrorCategory<IrErrorCodeEnum>;

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
        case IrErrorCodeEnum::CorruptedIR:
            return "The IR stream contains corrupted IR.";
        case IrErrorCodeEnum::UnsupportedFormat:
            return "The IR stream uses an unsupported metadata format, version, or structure.";
        default:
            return "Unknown error code enum.";
    }
}
