#ifndef CLP_FFI_IR_STREAM_SEARCH_QUERYHANDLERERRORCODE_HPP
#define CLP_FFI_IR_STREAM_SEARCH_QUERYHANDLERERRORCODE_HPP

#include <cstdint>

#include <ystdlib/error_handling/ErrorCode.hpp>

namespace clp::ffi::ir_stream::search {
/**
 * This enum class represents all possible error codes related to query handler.
 */
enum class ErrorCodeEnum : uint8_t {
    MethodNotImplemented,
};

using ErrorCode = ystdlib::error_handling::ErrorCode<ErrorCodeEnum>;
}  // namespace clp::ffi::ir_stream::search

YSTDLIB_ERROR_HANDLING_MARK_AS_ERROR_CODE_ENUM(clp::ffi::ir_stream::search::ErrorCodeEnum);

#endif  // CLP_FFI_IR_STREAM_SEARCH_QUERYHANDLERERRORCODE_HPP
