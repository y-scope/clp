#include "IrDeserializationError.hpp"

#include <string>

#include <ystdlib/error_handling/ErrorCode.hpp>

using clp::ffi::ir_stream::IrDeserializationErrorEnum;
using IrErrorCategory = ystdlib::error_handling::ErrorCategory<IrDeserializationErrorEnum>;

template <>
auto IrErrorCategory::name() const noexcept -> char const* {
    return "clp::ffi::ir_stream::IrDeserializationError";
}

template <>
auto IrErrorCategory::message(IrDeserializationErrorEnum error_enum) const -> std::string {
    switch (error_enum) {
        case IrDeserializationErrorEnum::DecodingMethodFailure:
            return "the decoding method failed";
        case IrDeserializationErrorEnum::EndOfStream:
            return "the end-of-stream IR unit has already been consumed";
        case IrDeserializationErrorEnum::IncompleteStream:
            return "the IR stream ended with a truncated IR unit or did not terminate with an "
                   "end-of-stream IR unit";
        case IrDeserializationErrorEnum::CorruptedIR:
            return "the IR stream contains corrupted IR";
        case IrDeserializationErrorEnum::UnsupportedFormat:
            return "the IR stream uses an unsupported metadata format, version, or structure";
        default:
            return "unknown error code enum";
    }
}
