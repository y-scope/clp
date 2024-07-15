#ifndef CLP_REGEX_UTILS_CONSTANTS_HPP
#define CLP_REGEX_UTILS_CONSTANTS_HPP

namespace clp::regex_utils {

// Wildcard meta characters
constexpr char cZeroOrMoreCharsWildcard{'*'};
constexpr char cSingleCharWildcard{'?'};

// Regex meta characters
constexpr char cRegexZeroOrMore{'*'};
constexpr char cRegexOneOrMore{'+'};
constexpr char cRegexZeroOrOne{'?'};
constexpr char cRegexStartAnchor{'^'};
constexpr char cRegexEndAnchor{'$'};
constexpr char cEscapeChar{'\\'};
constexpr char cCharsetNegate{'^'};

}  // namespace clp::regex_utils

#endif  // CLP_REGEX_UTILS_CONSTANTS_HPP
