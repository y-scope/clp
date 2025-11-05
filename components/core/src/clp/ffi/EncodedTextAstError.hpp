#ifndef CLP_FFI_ENCODEDTEXTASTERROR_HPP
#define CLP_FFI_ENCODEDTEXTASTERROR_HPP

#include <cstdint>

#include <ystdlib/error_handling/ErrorCode.hpp>

namespace clp::ffi {
/**
 * Error enums for `EncodedTextAst`
 */
enum class EncodedTextAstErrEnum : uint8_t {
    MissingDictVar = 1,
    MissingEncodedVar,
    MissingLogtype,
    UnexpectedTrailingEscapeCharacter,
};

using EncodedTextAstErr = ystdlib::error_handling::ErrorCode<EncodedTextAstErrEnum>;
}  // namespace clp::ffi

YSTDLIB_ERROR_HANDLING_MARK_AS_ERROR_CODE_ENUM(clp::ffi::EncodedTextAstErrEnum);

#endif  // CLP_FFI_ENCODEDTEXTASTERROR_HPP
