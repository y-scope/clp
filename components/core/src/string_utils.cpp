#include "string_utils.hpp"

// C++ standard libraries
#include <algorithm>
#include <charconv>
#include <cstring>

using std::string;
using std::string_view;

// Local function prototypes
/**
 * Helper for ``wildcard_match_unsafe_case_sensitive`` to advance the pointer in tame to
 * the next character which matches wild. This method should be inlined for
 * performance.
 * @param tame_current
 * @param tame_bookmark
 * @param tame_end
 * @param wild_current
 * @param wild_bookmark
 * @return true on success, false if wild cannot match tame
 */
static inline bool advance_tame_to_next_match (
        const char*& tame_current,
        const char*& tame_bookmark,
        const char* tame_end,
        const char*& wild_current,
        const char*& wild_bookmark
);

size_t find_first_of (const string& haystack, const char* needles, size_t search_start_pos,
                      size_t& needle_ix)
{
    size_t haystack_length = haystack.length();
    size_t needles_length = strlen(needles);
    for (size_t i = search_start_pos; i < haystack_length; ++i) {
        for (needle_ix = 0; needle_ix < needles_length; ++needle_ix) {
            if (haystack[i] == needles[needle_ix]) {
                return i;
            }
        }
    }

    return string::npos;
}

string replace_characters (const char* characters_to_replace, const char* replacement_characters,
                           const string& value, bool escape)
{
    string new_value;
    size_t search_start_pos = 0;
    while (true) {
        size_t replace_char_ix;
        size_t char_to_replace_pos = find_first_of(value, characters_to_replace, search_start_pos,
                                                   replace_char_ix);
        if (string::npos == char_to_replace_pos) {
            new_value.append(value, search_start_pos, string::npos);
            break;
        } else {
            new_value.append(value, search_start_pos, char_to_replace_pos - search_start_pos);
            if (escape) {
                new_value += "\\";
            }
            new_value += replacement_characters[replace_char_ix];
            search_start_pos = char_to_replace_pos + 1;
        }
    }
    return new_value;
}

void to_lower (string& str) {
    std::transform(str.cbegin(), str.cend(), str.begin(), [](unsigned char c) {
        return std::tolower(c);
    });
}

bool is_wildcard (char c) {
    static constexpr char cWildcards[] = "?*";
    for (size_t i = 0; i < strlen(cWildcards); ++i) {
        if (cWildcards[i] == c) {
            return true;
        }
    }
    return false;
}

string clean_up_wildcard_search_string (string_view str) {
    string cleaned_str;

    bool is_escaped = false;
    auto str_end = str.cend();
    for (auto current = str.cbegin(); current != str_end;) {
        auto c = *current;
        if (is_escaped) {
            is_escaped = false;

            if (is_wildcard(c) || '\\' == c) {
                // Keep escaping if c is a wildcard character or an escape character
                cleaned_str += '\\';
            }
            cleaned_str += c;
            ++current;
        } else if ('*' == c) {
            cleaned_str += c;

            // Skip over all '*' to find the next non-'*'
            do {
                ++current;
            } while (current != str_end && '*' == *current);
        } else {
            if ('\\' == c) {
                is_escaped = true;
            } else {
                cleaned_str += c;
            }
            ++current;
        }
    }

    return cleaned_str;
}

static inline bool advance_tame_to_next_match (
        const char*& tame_current,
        const char*& tame_bookmark,
        const char* tame_end,
        const char*& wild_current,
        const char*& wild_bookmark
) {
    auto w = *wild_current;
    if ('?' != w) {
        // No need to check for '*' since the caller ensures wild doesn't
        // contain consecutive '*'

        // Handle escaped characters
        if ('\\' == w) {
            ++wild_current;
            // This is safe without a bounds check since this the caller
            // ensures there are no dangling escape characters
            w = *wild_current;
        }

        // Advance tame_current until it matches wild_current
        while (true) {
            if (tame_end == tame_current) {
                // Wild group is longer than last group in tame, so
                // can't match
                // e.g. "*abc" doesn't match "zab"
                return false;
            }
            auto t = *tame_current;
            if (t == w) {
                break;
            }
            ++tame_current;
        }
    }

    tame_bookmark = tame_current;

    return true;
}

bool wildcard_match_unsafe (string_view tame, string_view wild, bool case_sensitive_match) {
    if (case_sensitive_match) {
        return wildcard_match_unsafe_case_sensitive(tame, wild);
    } else {
        // We convert to lowercase (rather than uppercase) anticipating that
        // callers use lowercase more frequently, so little will need to change.
        string lowercase_tame(tame);
        to_lower(lowercase_tame);
        string lowercase_wild(wild);
        to_lower(lowercase_wild);
        return wildcard_match_unsafe_case_sensitive(lowercase_tame, lowercase_wild);
    }
}

/**
 * The algorithm basically works as follows:
 * Given a wild string "*abc*def*ghi*", it can be broken into groups of
 * characters delimited by one or more '*' characters. The goal of the
 * algorithm is then to determine whether the tame string contains each of
 * those groups in the same order.
 *
 * Thus, the algorithm:
 * 1. searches for the start of one of these groups in wild,
 * 2. searches for a group in tame starting with the same character, and then
 * 3. checks if the two match. If not, the search repeats with the next group in
 *    tame.
 */
bool wildcard_match_unsafe_case_sensitive (string_view tame, string_view wild) {
    const auto tame_length = tame.length();
    const auto wild_length = wild.length();
    const char* tame_current = tame.data();
    const char* wild_current = wild.data();
    const char* tame_bookmark = nullptr;
    const char* wild_bookmark = nullptr;
    const char* tame_end = tame_current + tame_length;
    const char* wild_end = wild_current + wild_length;

    // Handle wild or tame being empty
    if (0 == wild_length) {
        return 0 == tame_length;
    } else {
        if (0 == tame_length) {
            return "*" == wild;
        }
    }

    char w;
    char t;
    bool is_escaped = false;
    while (true) {
        w = *wild_current;
        if ('*' == w) {
            ++wild_current;
            if (wild_end == wild_current) {
                // Trailing '*' means everything remaining in tame will match
                return true;
            }

            // Set wild and tame bookmarks
            wild_bookmark = wild_current;
            if (false == advance_tame_to_next_match(tame_current, tame_bookmark, tame_end,
                                                    wild_current, wild_bookmark))
            {
                return false;
            }
        } else {
            // Handle escaped characters
            if ('\\' == w) {
                is_escaped = true;
                ++wild_current;
                // This is safe without a bounds check since this the caller
                // ensures there are no dangling escape characters
                w = *wild_current;
            }

            // Handle a mismatch
            t = *tame_current;
            if (!((false == is_escaped && '?' == w) || t == w)) {
                if (nullptr == wild_bookmark) {
                    // No bookmark to return to
                    return false;
                }

                wild_current = wild_bookmark;
                tame_current = tame_bookmark + 1;
                if (false == advance_tame_to_next_match(tame_current, tame_bookmark, tame_end,
                                                        wild_current, wild_bookmark))
                {
                    return false;
                }
            }
        }

        ++tame_current;
        ++wild_current;

        // Handle reaching the end of tame or wild
        if (tame_end == tame_current) {
            return (wild_end == wild_current ||
                    ('*' == *wild_current && (wild_current + 1) == wild_end));
        } else {
            if (wild_end == wild_current) {
                if (nullptr == wild_bookmark) {
                    // No bookmark to return to
                    return false;
                } else {
                    wild_current = wild_bookmark;
                    tame_current = tame_bookmark + 1;
                    if (false == advance_tame_to_next_match(tame_current, tame_bookmark, tame_end,
                                                            wild_current, wild_bookmark))
                    {
                        return false;
                    }
                }
            }
        }
    }
}

bool convert_string_to_int64 (std::string_view raw, int64_t& converted) {
    auto raw_end = raw.cend();
    auto result = std::from_chars(raw.cbegin(), raw_end, converted);
    if (raw_end != result.ptr) {
        return false;
    } else {
        return result.ec == std::errc();
    }
}

bool convert_string_to_double (const std::string& raw, double& converted) {
    if (raw.empty()) {
        // Can't convert an empty string
        return false;
    }

    const char* c_str = raw.c_str();
    char* end_ptr;
    // Reset errno so we can detect a new error
    errno = 0;
    double raw_as_double = strtod(c_str, &end_ptr);
    if (ERANGE == errno || (0.0 == raw_as_double && ((end_ptr - c_str) < raw.length()))) {
        return false;
    }
    converted = raw_as_double;
    return true;
}

