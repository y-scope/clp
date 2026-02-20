#ifndef CLP_FFI_IR_STREAM_IR_SERIALIZATION_ERROR_HPP
#define CLP_FFI_IR_STREAM_IR_SERIALIZATION_ERROR_HPP

#include <cstdint>

#include <ystdlib/error_handling/ErrorCode.hpp>

namespace clp::ffi::ir_stream {
/**
 * This enum class represents error codes for IR stream serialization
 */
enum class IrSerializationErrorEnum : uint8_t {
    KeyValuePairSerializationFailure,
    MetadataSerializationFailure,
    SchemaTreeNodeSerializationFailure,
    SchemaTreeNodeIdSerializationFailure,
    UnsupportedSchemaTreeNodeType,
    UnsupportedUserDefinedMetadata,
};

using IrSerializationError = ystdlib::error_handling::ErrorCode<IrSerializationErrorEnum>;
}  // namespace clp::ffi::ir_stream

YSTDLIB_ERROR_HANDLING_MARK_AS_ERROR_CODE_ENUM(clp::ffi::ir_stream::IrSerializationErrorEnum);

#endif  // CLP_FFI_IR_STREAM_IR_SERIALIZATION_ERROR_HPP
