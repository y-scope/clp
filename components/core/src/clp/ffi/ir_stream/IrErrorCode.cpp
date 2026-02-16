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
        case IrErrorCodeEnum::MetadataSerializationFailure:
            return "Failed to serialize the stream's metadata.";
        case IrErrorCodeEnum::SchemaTreeNodeSerializationFailure:
            return "Failed to serialize a schema tree node.";
        case IrErrorCodeEnum::KeyValuePairSerializationFailure:
            return "Failed to serialize a key-value pair log event.";
        case IrErrorCodeEnum::UnsupportedUserDefinedMetadata:
            return "The user-defined metadata is not a valid JSON object.";
        default:
            return "Unknown error code enum.";
    }
}
