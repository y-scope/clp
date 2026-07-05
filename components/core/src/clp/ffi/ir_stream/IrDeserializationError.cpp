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
        case IrDeserializationErrorEnum::DuplicateKey:
            return "duplicated keys are found in the same kv-pair log event";
        case IrDeserializationErrorEnum::EncodedTextAstDeserializationFailure:
            return "failed to deserialize an encoded text AST";
        case IrDeserializationErrorEnum::EndOfStream:
            return "reached end-of-stream IR unit";
        case IrDeserializationErrorEnum::IncompleteStream:
            return "incomplete IR stream";
        case IrDeserializationErrorEnum::InvalidKeyGroupOrdering:
            return "invalid key-ID-group ordering";
        case IrDeserializationErrorEnum::InvalidMagicNumber:
            return "invalid magic number";
        case IrDeserializationErrorEnum::InvalidReferenceTimestampMetadata:
            return "reference timestamp metadata is missing or not a string";
        case IrDeserializationErrorEnum::InvalidReferenceTimestampValue:
            return "reference timestamp string could not be parsed as an integer";
        case IrDeserializationErrorEnum::InvalidTag:
            return "invalid tag";
        case IrDeserializationErrorEnum::MissingRequiredSchemaNodes:
            return "required message or timestamp schema tree node is missing";
        case IrDeserializationErrorEnum::UnknownSchemaTreeNodeType:
            return "unknown schema tree node type";
        case IrDeserializationErrorEnum::UnknownValueType:
            return "unknown value type";
        case IrDeserializationErrorEnum::UnsupportedMetadataFormat:
            return "IR stream metadata format unsupported";
        case IrDeserializationErrorEnum::UnsupportedVersion:
            return "IR stream version unsupported";
        default:
            return "unknown error code enum";
    }
}
