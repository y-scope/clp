#ifndef CLP_REGEX_UTILS_CONSTANTS_HPP
#define CLP_REGEX_UTILS_CONSTANTS_HPP

#include <array>
#include <cstddef>
#include <string_view>

namespace clp::regex_utils {
constexpr size_t cCharBitarraySize = 128;

/**
 * Creates an ASCII character lookup table at compile time.
 *
 * @param char_str A string that contains the characters to look up.
 * @return The lookup table as bit array.
 */
[[nodiscard]] constexpr auto create_char_bit_array(std::string_view char_str)
        -> std::array<bool, cCharBitarraySize> {
    std::array<bool, cCharBitarraySize> bit_array{};
    bit_array.fill(false);
    for (auto const ch : char_str) {
        bit_array.at(ch) = true;
    }
    return bit_array;
}

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

// Character bitmaps
// The set of regex metacharacters that can be preceded with an escape backslash to be treated as a
// literal.
constexpr auto cRegexEscapeSeqMetaCharsLut = create_char_bit_array("*+?|^$.{}[]()<>-_/=!\\");
// The set of wildcard metacharacters that must remain escaped in the translated string to be
// treated as a literal.
constexpr auto cWildcardMetaCharsLut = create_char_bit_array("?*\\");
// The set of metacharacters that can be preceded with an escape backslash in the regex character
// set to be treated as a literal.
constexpr auto cRegexCharsetEscapeSeqMetaCharsLut = create_char_bit_array("^-]\\");
}  // namespace clp::regex_utils

#endif  // CLP_REGEX_UTILS_CONSTANTS_HPP
