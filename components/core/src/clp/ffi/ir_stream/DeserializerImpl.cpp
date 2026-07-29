#include "DeserializerImpl.hpp"

#include "utils.hpp"

namespace clp::ffi::ir_stream {
auto DeserializerImpl::deserialize_ir_unit_utc_offset_change(ReaderInterface& reader)
        -> ystdlib::error_handling::Result<UtcOffset> {
    return UtcOffset{YSTDLIB_ERROR_HANDLING_TRYX(deserialize_int<int64_t>(reader))};
}
}  // namespace clp::ffi::ir_stream
