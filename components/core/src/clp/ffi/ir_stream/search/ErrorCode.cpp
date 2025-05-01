#include "ErrorCode.hpp"

#include <string>

#include <ystdlib/error_handling/ErrorCode.hpp>

namespace {
using clp::ffi::ir_stream::search::ErrorCodeEnum;
using ErrorCategory = ystdlib::error_handling::ErrorCategory<ErrorCodeEnum>;
}  // namespace

template <>
auto ErrorCategory::name() const noexcept -> char const* {
    return "clp::ffi::ir_stream::search::Error";
}

template <>
auto ErrorCategory::message(ErrorCodeEnum error_enum) const -> std::string {
    switch (error_enum) {
        case ErrorCodeEnum::EncodedTextAstDecodingFailure:
            return "Failed to decode the given encoded text AST.";
        case ErrorCodeEnum::LiteralTypeUnexpected:
            return "An unexpected literal type is reached.";
        case ErrorCodeEnum::LiteralTypeUnsupported:
            return "The given literal type is not supported.";
        case ErrorCodeEnum::MethodNotImplemented:
            return "The requested method is not implemented.";
        default:
            return "Unknown error code enum.";
    }
}
