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
        case ErrorCodeEnum::AstDynamicCastFailure:
            return "Failed to dynamically cast an AST node to the expected type.";
        case ErrorCodeEnum::ColumnDescriptorTokenIteratorOutOfBounds:
            return "Attempted to access a token beyond the end of the column descriptor.";
        case ErrorCodeEnum::ColumnTokenizationFailure:
            return "Failed to tokenize the column descriptor.";
        case ErrorCodeEnum::DuplicateProjectedColumn:
            return "The projected column is not unique.";
        case ErrorCodeEnum::EncodedTextAstDecodingFailure:
            return "Failed to decode the given encoded text AST.";
        case ErrorCodeEnum::LiteralTypeUnexpected:
            return "Unexpected literal type.";
        case ErrorCodeEnum::LiteralTypeUnsupported:
            return "Unsupported literal type.";
        case ErrorCodeEnum::MethodNotImplemented:
            return "The requested method is not implemented.";
        case ErrorCodeEnum::ProjectionColumnDescriptorCreationFailure:
            return "Failed to create a column descriptor for the given projection.";
        case ErrorCodeEnum::QueryExpressionIsNull:
            return "The query expression is NULL.";
        case ErrorCodeEnum::QueryTransformationPassFailed:
            return "Failed to execute transformation passes on the query expression.";
        default:
            return "Unknown error code enum.";
    }
}
