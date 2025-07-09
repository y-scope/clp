#ifndef CLP_STRING_UTILS_CONSTANTS_HPP
#define CLP_STRING_UTILS_CONSTANTS_HPP

#include <array>
#include <cstddef>
#include <string_view>

namespace clp::string_utils {
// Wildcard meta characters
constexpr char cZeroOrMoreCharsWildcard{'*'};
constexpr char cSingleCharWildcard{'?'};

// Escape meta characters
constexpr char cEscapeChar{'\\'};
}  // namespace clp::string_utils

#endif  // CLP_STRING_UTILS_CONSTANTS_HPP
