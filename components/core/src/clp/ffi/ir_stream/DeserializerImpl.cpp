#include "DeserializerImpl.hpp"

#include "IrDeserializationError.hpp"
#include "utils.hpp"

namespace clp::ffi::ir_stream {
auto DeserializerImpl::deserialize_utc_offset_change(ReaderInterface& reader)
        -> ystdlib::error_handling::Result<UtcOffset> {
    int64_t serialized_utc_offset{};
    if (false == deserialize_int(reader, serialized_utc_offset)) {
        return IrDeserializationError{IrDeserializationErrorEnum::IncompleteStream};
    }
    return UtcOffset{serialized_utc_offset};
}
}  // namespace clp::ffi::ir_stream
