#include "EncodedTextAstError.hpp"

#include <string>

#include <ystdlib/error_handling/ErrorCode.hpp>

using clp::ffi::EncodedTextAstErrEnum;
using EncodedTextAstErrorCategory = ystdlib::error_handling::ErrorCategory<EncodedTextAstErrEnum>;

template <>
auto EncodedTextAstErrorCategory::name() const noexcept -> char const* {
    return "clp::ffi::ir_stream::IrErrorCode";
}

template <>
auto EncodedTextAstErrorCategory::message(EncodedTextAstErrEnum error_enum) const -> std::string {
    switch (error_enum) {
        case EncodedTextAstErrEnum::MissingEncodedVar:
            return "An encoded variable is missing from the `EncodedTextAst`";
        case EncodedTextAstErrEnum::MissingDictVar:
            return "A dictionary variable is missing from the `EncodedTextAst`";
        case EncodedTextAstErrEnum::MissingLogtype:
            return "The logtype is missing from the `EncodedTextAst`";
        case EncodedTextAstErrEnum::UnexpectedTrailingEscapeCharacter:
            "Unexpected escape character without escaped value at the end of the logtype";
        default:
            return "Unknown error code enum.";
    }
}
