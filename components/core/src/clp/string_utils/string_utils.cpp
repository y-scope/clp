#include "string_utils/string_utils.hpp"
#include "string_utils/constants.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <string>
#include <string_view>

using std::string;
using std::string_view;
using clp::string_utils::cSingleCharWildcard;
using clp::string_utils::cWildcardEscapeChar;
using clp::string_utils::cZeroOrMoreCharsWildcard;

namespace {
/**
 * Helper for ``wildcard_match_unsafe_case_sensitive`` to advance the pointer in
 * tame to the next character which matches wild. This method should be inlined
 * for performance.
 *
 * @param tame_current
 * @param tame_bookmark
 * @param tame_end
 * @param wild_current
 * @param wild_bookmark
 * @return true on success, false if wild cannot match tame
 */
[[nodiscard]] inline auto advance_tame_to_next_match(
        char const*& tame_current,
        char const*& tame_bookmark,
        char const* tame_end,
        char const*& wild_current
) -> bool;

inline auto advance_tame_to_next_match(
        char const*& tame_current,
        char const*& tame_bookmark,
        char const* tame_end,
        char const*& wild_current
) -> bool {
    auto w = *wild_current;
    if (cSingleCharWildcard != w) {
        // No need to check for '*' since the caller ensures wild doesn't
        // contain consecutive '*'

        // Handle escaped characters
        if (cWildcardEscapeChar == w) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            ++wild_current;
            // This is safe without a bounds check since this the caller ensures
            // there are no dangling escape characters
            w = *wild_current;
        }

        // Advance tame_current until it matches wild_current
        while (true) {
            if (tame_end == tame_current) {
                // Wild group is longer than last group in tame, so can't match
                // e.g. "*abc" doesn't match "zab"
                return false;
            }
            auto t = *tame_current;
            if (t == w) {
                break;
            }
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            ++tame_current;
        }
    }

    tame_bookmark = tame_current;

    return true;
}
}  // namespace

namespace clp::string_utils {
auto find_first_of(
        string_view haystack,
        char const* needles,
        size_t search_start_pos,
        size_t& needle_ix
) -> size_t {
    size_t const haystack_length{haystack.length()};
    size_t const needles_length{strlen(needles)};
    for (size_t i = search_start_pos; i < haystack_length; ++i) {
        for (needle_ix = 0; needle_ix < needles_length; ++needle_ix) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            if (haystack[i] == needles[needle_ix]) {
                return i;
            }
        }
    }

    return string::npos;
}

auto replace_characters(
        char const* characters_to_replace,
        char const* replacement_characters,
        string_view value,
        bool escape
) -> string {
    string new_value;
    size_t search_start_pos{0};
    while (true) {
        size_t replace_char_ix{0};
        size_t const char_to_replace_pos
                = find_first_of(value, characters_to_replace, search_start_pos, replace_char_ix);
        if (string::npos == char_to_replace_pos) {
            new_value.append(value, search_start_pos);
            break;
        }
        new_value.append(value, search_start_pos, char_to_replace_pos - search_start_pos);
        if (escape) {
            new_value += "\\";
        }
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        new_value += replacement_characters[replace_char_ix];
        search_start_pos = char_to_replace_pos + 1;
    }
    return new_value;
}

auto replace_unescaped_char(
        char const escape_char,
        char const from_char,
        char const to_char,
        std::string& str
) -> void {
    bool escaped{false};

    auto should_replace_char = [&](char c) -> bool {
        if (escaped) {
            escaped = false;
        } else if (escape_char == c) {
            escaped = true;
        } else if (from_char == c) {
            return true;
        }
        return false;
    };

    std::replace_if(str.begin(), str.end(), should_replace_char, to_char);
}

void to_lower(string& str) {
    std::transform(str.cbegin(), str.cend(), str.begin(), [](unsigned char c) {
        return std::tolower(c);
    });
}

auto is_wildcard(char c) -> bool {
    return cSingleCharWildcard == c || cZeroOrMoreCharsWildcard == c;
}

auto clean_up_wildcard_search_string(string_view str) -> string {
    string cleaned_str;

    bool is_escaped{false};
    auto const* const str_end = str.cend();
    for (auto const* current = str.cbegin(); current != str_end;) {
        auto c = *current;
        if (is_escaped) {
            is_escaped = false;

            if (is_wildcard(c) || cWildcardEscapeChar == c) {
                // Keep escaping if c is a wildcard character or an escape
                // character
                cleaned_str += cWildcardEscapeChar;
            }
            cleaned_str += c;
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            ++current;
        } else if (cZeroOrMoreCharsWildcard == c) {
            cleaned_str += c;

            // Skip over all '*' to find the next non-'*'
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            ++current;
            while (current != str_end && cZeroOrMoreCharsWildcard == *current) {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                ++current;
            }
        } else {
            if (cWildcardEscapeChar == c) {
                is_escaped = true;
            } else {
                cleaned_str += c;
            }
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            ++current;
        }
    }

    return cleaned_str;
}

auto unescape_string(std::string_view str) -> std::string {
    std::string unescaped_str;
    bool escaped{false};
    for (auto const c : str) {
        if (escaped) {
            unescaped_str.push_back(c);
            escaped = false;
        } else if (cWildcardEscapeChar == c) {
            escaped = true;
        } else {
            unescaped_str.push_back(c);
        }
    }
    return unescaped_str;
}

auto wildcard_match_unsafe(string_view tame, string_view wild, bool case_sensitive_match) -> bool {
    if (case_sensitive_match) {
        return wildcard_match_unsafe_case_sensitive(tame, wild);
    }
    // We convert to lowercase (rather than uppercase) anticipating that callers
    // use lowercase more frequently, so little will need to change.
    string lowercase_tame{tame};
    to_lower(lowercase_tame);
    string lowercase_wild{wild};
    to_lower(lowercase_wild);
    return wildcard_match_unsafe_case_sensitive(lowercase_tame, lowercase_wild);
}

/**
 * The algorithm basically works as follows:
 * Given a wild string "*abc*def*ghi*", it can be broken into groups of
 * characters delimited by one or more '*' characters. The goal of the algorithm
 * is then to determine whether the tame string contains each of those groups in
 * the same order.
 *
 * Thus, the algorithm:
 * 1. searches for the start of one of these groups in wild,
 * 2. searches for a group in tame starting with the same character, and then
 * 3. checks if the two match. If not, the search repeats with the next group in
 *    tame.
 */
// NOLINTBEGIN(readability-function-cognitive-complexity)
auto wildcard_match_unsafe_case_sensitive(string_view tame, string_view wild) -> bool {
    auto const tame_length = tame.length();
    auto const wild_length = wild.length();
    char const* tame_current{tame.data()};
    char const* wild_current{wild.data()};
    char const* tame_bookmark{nullptr};
    char const* wild_bookmark{nullptr};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    char const* tame_end{tame_current + tame_length};
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    char const* wild_end{wild_current + wild_length};

    // Handle wild or tame being empty
    if (0 == wild_length) {
        return 0 == tame_length;
    }
    if (0 == tame_length) {
        return "*" == wild;
    }

    char w{'\0'};
    char t{'\0'};
    while (true) {
        w = *wild_current;
        if (cZeroOrMoreCharsWildcard == w) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            ++wild_current;
            if (wild_end == wild_current) {
                // Trailing '*' means everything remaining in tame will match
                return true;
            }

            // Set wild and tame bookmarks
            wild_bookmark = wild_current;
            if (false
                == advance_tame_to_next_match(tame_current, tame_bookmark, tame_end, wild_current))
            {
                return false;
            }
        } else {
            // Handle escaped characters
            bool const is_escaped{cWildcardEscapeChar == w};
            if (is_escaped) {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                ++wild_current;
                // This is safe without a bounds check since this the caller
                // ensures there are no dangling escape characters
                w = *wild_current;
            }

            // Handle a mismatch
            t = *tame_current;
            if (false == ((false == is_escaped && cSingleCharWildcard == w) || t == w)) {
                if (nullptr == wild_bookmark) {
                    // No bookmark to return to
                    return false;
                }

                wild_current = wild_bookmark;
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                tame_current = tame_bookmark + 1;
                if (false
                    == advance_tame_to_next_match(
                            tame_current,
                            tame_bookmark,
                            tame_end,
                            wild_current
                    ))
                {
                    return false;
                }
            }
        }

        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        ++tame_current;
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        ++wild_current;

        // Handle reaching the end of tame or wild
        if (tame_end == tame_current) {
            return (wild_end == wild_current
                    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                    || (cZeroOrMoreCharsWildcard == *wild_current && (wild_current + 1) == wild_end));
        }
        if (wild_end == wild_current) {
            if (nullptr == wild_bookmark) {
                // No bookmark to return to
                return false;
            }
            wild_current = wild_bookmark;
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            tame_current = tame_bookmark + 1;
            if (false
                == advance_tame_to_next_match(tame_current, tame_bookmark, tame_end, wild_current))
            {
                return false;
            }
        }
    }
}
// NOLINTEND(readability-function-cognitive-complexity)
}  // namespace clp::string_utils
