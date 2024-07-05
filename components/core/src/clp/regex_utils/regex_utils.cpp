#include "regex_utils/regex_utils.hpp"

#include <array>
#include <iostream>
#include <stdexcept>

#define CHAR_BITARRAY_SIZE 128

using std::array;
using std::invalid_argument;
using std::runtime_error;
using std::string;

using std::cout;
using std::endl;

namespace {

inline constexpr array<bool, CHAR_BITARRAY_SIZE> createCharBitArray(
        char const* charStr
) {
    array<bool, CHAR_BITARRAY_SIZE> bitArray;
    bitArray.fill(false);
    int idx = 0;
    for (int idx = 0; charStr[idx] != '\0'; ++idx) {
        bitArray.at(charStr[idx]) = true;
    }
    return bitArray;
}

constexpr char ZeroOrMoreCharsWildcard = '*';
constexpr char SingleCharWildcard = '?';
constexpr char RegexStartAnchor = '^';
constexpr char RegexEndAnchor = '$';
constexpr char EscapeChar = '\\';

enum RegexPatternState {
    NORMAL,
    DOT,  // Preceded by a single period `.`, used to start the wildcard syntax
    ESCAPED,  // Preceded by an escape backslash `\\`, used to suppress special meanings of meta characters
    GROUP,  // Enclosed by parenthesis `()`, used to specify a capture group
    CHARSET,  // Enclosed by square brackets `[]`, used to specify a character set
    QUANTIFIER,  // Enclosed by curly brackets `{}`, used to specify a quantity to repeat
    END,  // Regex string has reached the end anchor `$`
};

constexpr auto RegexNormalStateNonTransitionalMetaChars = createCharBitArray("^|?*+");

}  // namespace

namespace clp::regex_utils {

void regexPatternStateFinalCheck(
        string& wildcardStr,
        RegexPatternState& state,
        string& currQuantifier
);

}  // namespace clp::regex_utils

namespace clp::regex_utils {

string regexToWildcard(string const& regexStr) {
    if (regexStr.empty()) {
        return string();
    }

	// Initialize scan position, scan state, and return string
	int idx = 0;
    RegexPatternState state = NORMAL;
    string wildcardStr;

	// If there is no starting anchor character, append multichar wildcard prefix
    if (RegexStartAnchor == regexStr.at(0)) {
        idx++;
    } else {
        wildcardStr += ZeroOrMoreCharsWildcard;
    }

    // Initialize various string buffers
    string currGroup;
    string currQuantifier;
    for (; idx < regexStr.length(); ++idx) {
        // Main state transition table
        const char ch = regexStr.at(idx);
        switch (state) {
            case NORMAL:
                switch (ch) {
                    case '.':
                        state = DOT;
                        break;
                    case EscapeChar:
                        state = ESCAPED;
                        break;
                    case '(':
                        currGroup.clear();
                        state = GROUP;
                        break;
                    case '[':
                        state = CHARSET;
                        break;
                    case '{':
                        state = QUANTIFIER;
                        break;
                    case RegexEndAnchor:
                        state = END;
                        break;
                    case '|':
                        throw runtime_error(
                            "Currently does not support returning a list of wildcard options."
                        );
                    default:
                        if (RegexNormalStateNonTransitionalMetaChars.at(ch)) {
                            throw invalid_argument(
                                "Cannot translate due to an unescaped meta character " + ch
                            );
                        }
                        wildcardStr += ch;
                        break;
                }
                break;
            case DOT:
                if ('*' == ch) {
                    wildcardStr += ZeroOrMoreCharsWildcard;
                } else {
                    wildcardStr += SingleCharWildcard;
                    // Backtrack one position and handle the current char in the next iteration
                    --idx;
                }
                state = NORMAL;
                break;
            case END:
                if (RegexEndAnchor != ch) {
                    throw invalid_argument("Encountered non-anchor characters past the end of $.");
                }
                break;
            default:
                throw runtime_error("Entered illegal regex pattern state " + state);
                break;
        }

    }

    // Do the final state check and clean up
    // TODO: in the future there may be a need to backtrack to a previous scan position and rescan
    // from a different state.
    regexPatternStateFinalCheck(wildcardStr, state, currQuantifier);

    return wildcardStr;
}

void regexPatternStateFinalCheck(
        string& wildcardStr,
        RegexPatternState& state,
        string& currQuantifier
) {
    switch (state) {
        case DOT:
            // The last character is a single `.`, without the possibility of becoming a
            // multichar wildcard
            wildcardStr += SingleCharWildcard;
            break;
        case ESCAPED:
            throw invalid_argument("Incomplete escape sequence at the end.");
            break;
        case GROUP:
            throw invalid_argument("Unmatched closing `)` at the end.");
            break;
        case CHARSET:
            throw invalid_argument("Unmatched closing `]` at the end.");
            break;
        case QUANTIFIER:
			// Not a valid quantifier expression due to no closing curly bracket, but
            // everything inside the bracket is purely numeric, so append directly.
			wildcardStr += '{';
            wildcardStr += currQuantifier;
            break;
        default:
            break;
    }
    if (END != state) {
        wildcardStr += ZeroOrMoreCharsWildcard;
    }
}


string regexTrimLineAnchors(string const& regexStr) {
    const int lastIdx = regexStr.length() - 1;

    int beginPos = 0;
    int endPos = lastIdx;

    // Find the position of the first non-caret character
    while (beginPos <= endPos && RegexStartAnchor == regexStr.at(beginPos)) {
        ++beginPos;
    }
    // Backtrack one char to include at least one start anchor, if there was any.
    if (beginPos > 0) {
        --beginPos;
    }

    // Find the position of the last non-dollar-sign character
    while (beginPos <= endPos && RegexEndAnchor == regexStr.at(endPos)) {
        --endPos;
    }
    if (endPos < lastIdx) {
        // There was at least one end anchor so we include it by advancing one char
        ++endPos;
    }

    // If there was more than one end anchor, we need to check if the current end anchor is escaped.
    // If so, it's not a real end anchor, and we need to advance the end position once more to
    // append a real end anchor.
    string trimmedRegexStr = regexStr.substr(beginPos, endPos - beginPos + 1);
    if (endPos < lastIdx && !regexHasEndAnchor(trimmedRegexStr)) {
        trimmedRegexStr += RegexEndAnchor;
    }
    return trimmedRegexStr;
}

bool regexHasStartAnchor(string const& regexStr) {
    return !regexStr.empty() && RegexStartAnchor == regexStr.at(0);
}

bool regexHasEndAnchor(string const& regexStr) {
    int len = regexStr.length();
    if (len <= 0 || RegexEndAnchor != regexStr.back()) {
        return false;
    }

    // Check that ending regex dollar sigh char is unescaped.
    // We need to scan the suffix until we encounter a character that is not an
    // escape char, since escape chars can escape themselves.
    bool escaped = false;
    for (int idx = len - 2; idx >= 0 && EscapeChar == regexStr.at(idx); --idx) {
        escaped = !escaped;
    }
    return !escaped;
}

}  // namespace clp::regex_utils
