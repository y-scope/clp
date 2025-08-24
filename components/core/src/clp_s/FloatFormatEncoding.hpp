#ifndef CLP_S_FLOATFORMATENCODING_HPP
#define CLP_S_FLOATFORMATENCODING_HPP

#include <cstdint>

namespace clp_s::float_format_encoding {
// Bit positions for the 16-bit format descriptor.
// See docs/src/dev-guide/design-retain-float-format.md for the full layout.

// Scientific notation marker (2 bits)
constexpr uint16_t cExponentNotationPos = 14;
// Exponent sign presence (2 bits)
constexpr uint16_t cExponentSignPos = 12;
constexpr uint16_t cEmptyExponentSign = 0b00;
constexpr uint16_t cPlusExponentSign = 0b01;
constexpr uint16_t cMinusExponentSign = 0b10;
// Number of exponent digits (2 bits)
constexpr uint16_t cNumExponentDigitsPos = 10;
// Number of significant digits (4 bits)
constexpr uint16_t cNumSignificantDigitsPos = 6;

static_assert(cExponentNotationPos <= 15, "Bit position out of range");
static_assert(cExponentSignPos <= 15, "Bit position out of range");
static_assert(cNumExponentDigitsPos <= 15, "Bit position out of range");
static_assert(cNumSignificantDigitsPos <= 15, "Bit position out of range");
}  // namespace clp_s::float_format_encoding

#endif  // CLP_S_FLOATFORMATENCODING_HPP
