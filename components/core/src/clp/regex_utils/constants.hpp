#ifndef CLP_REGEX_UTILS_CONSTANTS_HPP
#define CLP_REGEX_UTILS_CONSTANTS_HPP

#include <array>
#include <cstddef>
#include <string_view>

namespace clp::regex_utils {

constexpr size_t cCharBitarraySize = 128;

/**
 * Create an ASCII character lookup table (bit array) at compile time.
 *
 * @param char_str A string that contains the characters to look up.
 * @return The lookup table as bit array
 */
[[nodiscard]] constexpr auto create_char_bit_array(std::string_view char_str
) -> std::array<bool, cCharBitarraySize> {
    std::array<bool, cCharBitarraySize> bit_array{};
    bit_array.fill(false);
    for (char const ch : char_str) {
        bit_array.at(ch) = true;
    }
    return bit_array;
}

constexpr char cZeroOrMoreCharsWildcard{'*'};
constexpr char cSingleCharWildcard{'?'};
constexpr char cRegexZeroOrMore{'*'};
constexpr char cRegexOneOrMore{'+'};
constexpr char cRegexZeroOrOne{'+'};
constexpr char cRegexStartAnchor{'^'};
constexpr char cRegexEndAnchor{'$'};
constexpr char cEscapeChar{'\\'};
constexpr char cCharsetNegate{'^'};

// This is a more complete set of meta characters than necessary, as the user might not be fully
// knowledgeable on which meta characters to escape, and may introduce unnecessary escape sequences.
constexpr auto cRegexEscapeSeqAcceptedMetaChars = create_char_bit_array("^$.*{}[]()+|?<>-_/=!\\");
// This is the set of meta characters that need escaping in the wildcard syntax.
constexpr auto cRegexEscapeSeqWildcardOnlyMetaChars = create_char_bit_array("?*\\");
// This is the set of meta characters that need escaping in the character set.
constexpr auto cRegexCharsetEscapeSeqMetaChars = create_char_bit_array("^-]\\");

}  // namespace clp::regex_utils

#endif  // CLP_REGEX_UTILS_CONSTANTS_HPP
