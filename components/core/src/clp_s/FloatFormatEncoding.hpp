#ifndef CLP_S_FLOATFORMATENCODING_HPP
#define CLP_S_FLOATFORMATENCODING_HPP

#include <cstdint>

namespace clp_s::float_format_encoding {
// Scientific notation marker (2 bits)
constexpr uint16_t cScientificExponentNotePos = 14;
// Exponent sign presence (2 bits)
constexpr uint16_t cScientificExponentSignPos = 12;
// Number of exponent digits (2 bits)
constexpr uint16_t cScientificExponentDigitsPos = 10;
// Number of significant digits (4 bits)
constexpr uint16_t cSignificantDigitsPos = 6;
}  // namespace clp_s::float_format_encoding

#endif  // CLP_S_FLOATFORMATENCODING_HPP
