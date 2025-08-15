#ifndef CLP_S_FLOATFORMATENCODING_HPP
#define CLP_S_FLOATFORMATENCODING_HPP

#include <cstdint>

namespace clp_s::float_format_encoding {
// Bit positions for the 16-bit format descriptor.
// See docs/src/dev-guide/design-retain-float-format.md for the full layout.

// Scientific notation marker (2 bits)
constexpr uint16_t cScientificExponentNotePos = 14;
// Exponent sign presence (2 bits)
constexpr uint16_t cScientificExponentSignPos = 12;
// Number of exponent digits (2 bits)
constexpr uint16_t cScientificExponentDigitsPos = 10;
// Number of significant digits (4 bits)
constexpr uint16_t cSignificantDigitsPos = 6;

static_assert(cScientificExponentNotePos <= 15, "Bit position out of range");
static_assert(cScientificExponentSignPos <= 15, "Bit position out of range");
static_assert(cScientificExponentDigitsPos <= 15, "Bit position out of range");
static_assert(cSignificantDigitsPos <= 15, "Bit position out of range");
}  // namespace clp_s::float_format_encoding

#endif  // CLP_S_FLOATFORMATENCODING_HPP
