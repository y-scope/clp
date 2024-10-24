#ifndef CLP_FFI_IR_STREAM_IRUNITTYPE_HPP
#define CLP_FFI_IR_STREAM_IRUNITTYPE_HPP

#include <cstdint>

namespace clp::ffi::ir_stream {
/**
 * Enum defining the possible IR unit types in CLP kv-pair IR stream.
 */
enum class IrUnitType : uint8_t {
    LogEvent = 0,
    SchemaTreeNodeInsertion,
    UtcOffsetChange,
    EndOfStream,
};
}  // namespace clp::ffi::ir_stream

#endif  // CLP_FFI_IR_STREAM_IRUNITTYPE_HPP
