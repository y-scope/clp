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
        case IrDeserializationErrorEnum::CorruptedIR:
            return "the IR stream contains corrupted IR";
        case IrDeserializationErrorEnum::DecodingMethodFailure:
            return "the decoding method failed";
        case IrDeserializationErrorEnum::DuplicateKey:
            return "a key is duplicated in the deserialized log event";
        case IrDeserializationErrorEnum::EndOfStream:
            return "the end-of-stream IR unit has already been consumed";
        case IrDeserializationErrorEnum::IncompleteStream:
            return "the IR stream ended with a truncated IR unit or did not terminate with an "
                   "end-of-stream IR unit";
        case IrDeserializationErrorEnum::InvalidKeyOrdering:
            return "auto-generated key IDs appear after user-generated key IDs in the log event";
        case IrDeserializationErrorEnum::InvalidTag:
            return "the tag byte does not match the expected type";
        case IrDeserializationErrorEnum::UnsupportedFormat:
            return "the IR stream uses an unsupported metadata format, version, or structure";
        case IrDeserializationErrorEnum::UnsupportedNodeType:
            return "the deserialized node or value type is not supported";
        default:
            return "unknown error code enum";
    }
}
