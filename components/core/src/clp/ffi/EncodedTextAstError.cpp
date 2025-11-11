#include "EncodedTextAstError.hpp"

#include <string>

#include <ystdlib/error_handling/ErrorCode.hpp>

using clp::ffi::EncodedTextAstErrorEnum;
using EncodedTextAstErrorCategory = ystdlib::error_handling::ErrorCategory<EncodedTextAstErrorEnum>;

template <>
auto EncodedTextAstErrorCategory::name() const noexcept -> char const* {
    return "clp::ffi::EncodedTextAstErrorCode";
}

template <>
auto EncodedTextAstErrorCategory::message(EncodedTextAstErrorEnum error_enum) const -> std::string {
    switch (error_enum) {
        case EncodedTextAstErrorEnum::MissingEncodedVar:
            return "An encoded variable is missing from the `EncodedTextAst`";
        case EncodedTextAstErrorEnum::MissingDictVar:
            return "A dictionary variable is missing from the `EncodedTextAst`";
        case EncodedTextAstErrorEnum::MissingLogtype:
            return "The logtype is missing from the `EncodedTextAst`";
        case EncodedTextAstErrorEnum::UnexpectedTrailingEscapeCharacter:
            return "Unexpected escape character without escaped value at the end of the logtype";
        default:
            return "Unknown error code enum";
    }
}
