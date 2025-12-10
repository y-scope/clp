#ifndef CLP_FFI_ENCODEDTEXTASTERROR_HPP
#define CLP_FFI_ENCODEDTEXTASTERROR_HPP

#include <cstdint>

#include <ystdlib/error_handling/ErrorCode.hpp>

namespace clp::ffi {
/**
 * Error enums for `EncodedTextAst`.
 */
enum class EncodedTextAstErrorEnum : uint8_t {
    MissingDictVar = 1,
    MissingEncodedVar,
    MissingLogtype,
    UnexpectedTrailingEscapeCharacter,
};

using EncodedTextAstError = ystdlib::error_handling::ErrorCode<EncodedTextAstErrorEnum>;
}  // namespace clp::ffi

YSTDLIB_ERROR_HANDLING_MARK_AS_ERROR_CODE_ENUM(clp::ffi::EncodedTextAstErrorEnum);

#endif  // CLP_FFI_ENCODEDTEXTASTERROR_HPP
