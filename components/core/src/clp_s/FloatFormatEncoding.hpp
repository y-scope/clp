#ifndef CLP_S_FLOATFORMATENCODING_HPP
#define CLP_S_FLOATFORMATENCODING_HPP

#include <cstdint>

namespace clp_s::float_format_encoding {
// The 14~15 bits are for
constexpr uint16_t cScientificExponentNotePos = 14;
constexpr uint16_t cScientificExponentSignPos = 12;
constexpr uint16_t cScientificExponentDigitsPos = 10;
constexpr uint16_t cSignificantDigitsPos = 6;
}  // namespace  clp_s::float_format_encoding

#endif  // CLP_S_FLOATFORMATENCODING_HPP
