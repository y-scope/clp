#include "IrSerializationError.hpp"

#include <string>

#include <ystdlib/error_handling/ErrorCode.hpp>

using clp::ffi::ir_stream::IrSerializationErrorEnum;
using IrSerializationErrorCategory
        = ystdlib::error_handling::ErrorCategory<IrSerializationErrorEnum>;

template <>
auto IrSerializationErrorCategory::name() const noexcept -> char const* {
    return "clp::ffi::ir_stream::IrSerializationError";
}

template <>
auto IrSerializationErrorCategory::message(IrSerializationErrorEnum error_enum) const
        -> std::string {
    switch (error_enum) {
        case IrSerializationErrorEnum::KeyValuePairSerializationFailure:
            return "failed to serialize a key-value pair log event";
        case IrSerializationErrorEnum::MetadataSerializationFailure:
            return "failed to serialize the stream's metadata";
        case IrSerializationErrorEnum::SchemaTreeNodeSerializationFailure:
            return "failed to serialize a schema tree node";
        case IrSerializationErrorEnum::SchemaTreeNodeIdSerializationFailure:
            return "failed to serialize a schema tree node ID";
        case IrSerializationErrorEnum::UnsupportedSchemaTreeNodeType:
            return "the schema tree node type is not supported";
        case IrSerializationErrorEnum::UnsupportedUserDefinedMetadata:
            return "the user-defined metadata is not a valid JSON object";
        default:
            return "unknown serialization error code enum";
    }
}
