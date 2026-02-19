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
            return "the decoding method failed";
        case IrErrorCodeEnum::EndOfStream:
            return "the end-of-stream IR unit has already been consumed";
        case IrErrorCodeEnum::IncompleteStream:
            return "the IR stream ended with a truncated IR unit or did not terminate with an "
                   "end-of-stream IR unit";
        default:
            return "unknown error code enum";
    }
}
