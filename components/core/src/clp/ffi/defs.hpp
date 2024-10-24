#ifndef CLP_FFI_DEFS_HPP
#define CLP_FFI_DEFS_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

#include "../ErrorCode.hpp"
#include "../TraceableException.hpp"

namespace clp::ffi {
/*
 * These constants can be used by callers to store the version of the schemas and encoding methods
 * they're using. At some point, we may update and/or add built-in schemas/encoding methods. So
 * callers must store the versions they used for encoding to ensure that they can choose the same
 * versions for decoding.
 *
 * We use versions which look like package names in anticipation of users writing their own custom
 * schemas and encoding methods.
 */
constexpr std::string_view cVariableEncodingMethodsVersion
        = "com.yscope.clp.VariableEncodingMethodsV1";
constexpr std::string_view cVariablesSchemaVersion = "com.yscope.clp.VariablesSchemaV2";

constexpr std::string_view cTooFewDictionaryVarsErrorMessage
        = "There are fewer dictionary variables than dictionary variable placeholders in the "
          "logtype.";
constexpr std::string_view cTooFewEncodedVarsErrorMessage
        = "There are fewer encoded variables than encoded variable placeholders in the logtype.";
constexpr std::string_view cUnexpectedEscapeCharacterMessage
        = "Unexpected escape character without escaped value at the end of the logtype.";
constexpr std::string_view cTooManyDigitsErrorMsg = "Encoded number of digits doesn't match "
                                                    "encoded digits in encoded float.";

constexpr size_t cMaxDigitsInRepresentableEightByteFloatVar = 16;
constexpr size_t cMaxDigitsInRepresentableFourByteFloatVar = 8;
constexpr uint64_t cEightByteEncodedFloatNumDigits = 54;
constexpr uint64_t cFourByteEncodedFloatNumDigits = 25;
constexpr uint64_t cEightByteEncodedFloatDigitsBitMask = (1ULL << 54) - 1;
constexpr uint32_t cFourByteEncodedFloatDigitsBitMask = (1UL << 25) - 1;

constexpr size_t cDecimalBase = 10;
constexpr uint32_t cLowerFourDigitsBitMask = (1UL << 4) - 1;
constexpr uint32_t cLowerThreeDigitsBitMask = (1UL << 3) - 1;

class EncodingException : public TraceableException {
public:
    // Constructors
    EncodingException(
            ErrorCode error_code,
            char const* const filename,
            int line_number,
            std::string message
    )
            : TraceableException(error_code, filename, line_number),
              m_message(std::move(message)) {}

    // Methods
    [[nodiscard]] auto what() const noexcept -> char const* override { return m_message.c_str(); }

private:
    std::string m_message;
};
}  // namespace clp::ffi

#endif  // CLP_FFI_DEFS_HPP
