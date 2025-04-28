#include "QueryHandlerErrorCode.hpp"

#include <ystdlib/error_handling/ErrorCode.hpp>

using clp::ffi::ir_stream::search::QueryHandlerErrorCodeEnum;
using QueryHandlerErrorCategory = ystdlib::error_handling::ErrorCategory<QueryHandlerErrorCodeEnum>;

template <>
auto QueryHandlerErrorCategory::name() const noexcept -> char const* {
    return "clp::ffi::ir_stream::search::QueryHandlerError";
}

template <>
auto QueryHandlerErrorCategory::message(QueryHandlerErrorCodeEnum error_enum) const -> std::string {
    switch (error_enum) {
        case QueryHandlerErrorCodeEnum::MethodNotImplemented:
            return "The requested method is not implemented.";
        default:
            return "Unknown error code enum.";
    }
}
