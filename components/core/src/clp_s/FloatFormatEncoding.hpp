#ifndef CLP_S_FLOATFORMATENCODING_HPP
#define CLP_S_FLOATFORMATENCODING_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include <ystdlib/error_handling/Result.hpp>

namespace clp_s::float_format_encoding {
// Bit positions for the 16-bit format descriptor.
// See docs/src/dev-guide/design-retain-float-format.md for the full layout.

// Scientific notation marker (2 bits)
constexpr uint16_t cScientificNotationFlagPos = 14U;
constexpr uint16_t cScientificNotationFlagMask = 0b11U << cScientificNotationFlagPos;
constexpr uint16_t cScientificNotationEnabledBit = 0b01U << cScientificNotationFlagPos;
constexpr uint16_t cScientificNotationLowerCaseEFlag = 0b01U << cScientificNotationFlagPos;
constexpr uint16_t cScientificNotationUpperCaseEFlag = 0b11U << cScientificNotationFlagPos;
// Exponent sign presence (2 bits)
constexpr uint16_t cExponentSignFlagPos = 12U;
constexpr uint16_t cExponentSignFlagMask = 0b11U << cExponentSignFlagPos;
constexpr uint16_t cEmptyExponentSignFlag = 0b00U << cExponentSignFlagPos;
constexpr uint16_t cPlusExponentSignFlag = 0b01U << cExponentSignFlagPos;
constexpr uint16_t cMinusExponentSignFlag = 0b10U << cExponentSignFlagPos;
// Number of exponent digits (2 bits)
constexpr uint16_t cNumExponentDigitsPos = 10U;
constexpr uint16_t cNumExponentDigitsMask = 0b11U << cNumExponentDigitsPos;
// Number of significant digits (4 bits)
constexpr uint16_t cNumSignificantDigitsPos = 5U;
constexpr uint16_t cNumSignificantDigitsMask = 0b1'1111U << cNumSignificantDigitsPos;
constexpr size_t cMaxNumSignificantDigits = 17U;

static_assert(cScientificNotationFlagPos <= 15U, "Bit position out of range");
static_assert(cExponentSignFlagPos <= 15U, "Bit position out of range");
static_assert(cNumExponentDigitsPos <= 15U, "Bit position out of range");
static_assert(cNumSignificantDigitsPos <= 15U, "Bit position out of range");

/**
 * Tries to derive the floating point encoding of `float_str` according to what we describe in
 * `docs/src/dev-guide/design-retain-float-format.md`.
 * @param float_str A string representing a valid floating point number.
 * @return The encoded format of `float_str`, or `std::errc::protocol_not_supported` if `float_str`
 * is not representable in our encoding scheme.
 */
auto get_float_encoding(std::string_view float_str) -> ystdlib::error_handling::Result<uint16_t>;

auto restore_encoded_float(double value, uint16_t format)
        -> ystdlib::error_handling::Result<std::string>;
}  // namespace clp_s::float_format_encoding

#endif  // CLP_S_FLOATFORMATENCODING_HPP
