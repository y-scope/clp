#include "Utils.hpp"

// C libraries
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

// C++ libraries
#include <algorithm>
#include <iostream>
#include <set>

// Boost libraries
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

// spdlog
#include <spdlog/spdlog.h>

// Project headers
#include "Profiler.hpp"

using std::list;
using std::string;
using std::vector;

static const char* const cWildcards = "?*";

/**
 * Checks if the given character is an alphabet
 * @param c
 * @return true if c is an alphabet, false otherwise
 */
static inline bool is_alphabet (char c) {
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z');
}

/**
 * Checks if character is a decimal (base-10) digit
 * @param c
 * @return true if c is a decimal digit, false otherwise
 */
static inline bool is_decimal_digit (char c) {
    return '0' <= c && c <= '9';
}

/**
 * Checks if character is a delimiter
 * We treat everything except the following quoted characters as a delimiter: "+-./0-9A-Z\a-z"
 * NOTE: For performance, we rely on the ASCII ordering of characters to compare ranges of characters at a time instead of comparing individual characters
 * @param c
 * @return true if c is a delimiter, false otherwise
 */
static inline bool is_delim (char c) {
    return !('+' == c || ('-' <= c && c <= '9') || ('A' <= c && c <= 'Z') || '\\' == c || '_' == c || ('a' <= c && c <= 'z'));
}

/**
 * Checks if the given segment of the stringcould be a multi-digit hex value
 * @param str
 * @param begin_pos
 * @param end_pos
 * @return true if yes, false otherwise
 */
static inline bool could_be_multi_digit_hex_value (const string& str, size_t begin_pos, size_t end_pos) {
    if (end_pos - begin_pos < 2) {
        return false;
    }

    for (size_t i = begin_pos; i < end_pos; ++i) {
        auto c = str[i];
        if (!( ('a' <= c && c <= 'f') || ('A' <= c && c <= 'F') || ('0' <= c && c <= '9') )) {
            return false;
        }
    }

    return true;
}

/**
 * Checks if character is a wildcard
 * @param c
 * @return true if c is a wildcard, false otherwise
 */
static bool is_wildcard (char c) {
    for (size_t i = 0; i < strlen(cWildcards); ++i) {
        if (cWildcards[i] == c) {
            return true;
        }
    }
    return false;
}

string clean_up_wildcard_search_string (const string& str) {
    string cleaned_str;

    for (size_t begin_pos = 0; begin_pos < str.length(); ) {
        begin_pos = str.find_first_not_of('*', begin_pos);
        if (string::npos == begin_pos) {
            break;
        }
        cleaned_str += '*';

        // Add everything up to next unescaped '*'
        bool is_escaped = false;
        for (; begin_pos < str.length(); ++begin_pos) {
            char c = str[begin_pos];

            if (is_escaped) {
                if (is_wildcard(c) || '\\' == c) {
                    // Keep escaping if c is a wildcard character or an escape character
                    cleaned_str += '\\';
                }
                cleaned_str += c;

                is_escaped = false;
            } else if ('\\' == c) {
                is_escaped = true;
            } else if ('*' == c) {
                break;
            } else {
                cleaned_str += c;
            }
        }
    }
    cleaned_str += '*';

    return cleaned_str;
}

bool convert_string_to_int64 (const std::string& raw, int64_t& converted) {
    if (raw.empty()) {
        // Can't convert an empty string
        return false;
    }

    const char* c_str = raw.c_str();
    char* endptr;
    // Reset errno so we can detect if it's been set
    errno = 0;
    int64_t raw_as_int = strtoll(c_str, &endptr, 10);
    if (endptr - c_str != raw.length() || (LLONG_MAX == raw_as_int && ERANGE == errno)) {
        // Conversion failed
        return false;
    }
    converted = raw_as_int;
    return true;
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

ErrorCode create_directory (const string& path, __mode_t mode, bool exist_ok) {
    int retval = mkdir(path.c_str(), mode);
    if (0 != retval ) {
        if (EEXIST != errno) {
            return ErrorCode_errno;
        } else if (false == exist_ok) {
            return ErrorCode_FileExists;
        }
    }

    return ErrorCode_Success;
}

ErrorCode create_directory_structure (const string& path, __mode_t mode) {
    assert(!path.empty());

    // Check if entire path already exists
    struct stat s = {};
    if (0 == stat(path.c_str(), &s)) {
        // Deepest directory exists, so can return here
        return ErrorCode_Success;
    } else if (ENOENT != errno) {
        // Unexpected error
        return ErrorCode_errno;
    }

    // Find deepest directory which exists, starting from the (2nd) deepest directory
    size_t path_end_pos = path.find_last_of('/');
    size_t last_path_end_pos = path.length();
    string dir_path;
    while (string::npos != path_end_pos) {
        if (last_path_end_pos - path_end_pos > 1) {
            dir_path.assign(path, 0, path_end_pos);
            if (0 == stat(dir_path.c_str(), &s)) {
                break;
            } else if (ENOENT != errno) {
                // Unexpected error
                return ErrorCode_errno;
            }
        }

        last_path_end_pos = path_end_pos;
        path_end_pos = path.find_last_of('/', path_end_pos - 1);
    }

    if (string::npos == path_end_pos) {
        // NOTE: Since the first path we create below contains more than one character, this assumes the path "/"
        // already exists
        path_end_pos = 0;
    }
    while (string::npos != path_end_pos) {
        path_end_pos = path.find_first_of('/', path_end_pos + 1);
        dir_path.assign(path, 0, path_end_pos);
        // Technically the directory shouldn't exist at this point in the code, but it may have been created concurrently.
        auto error_code = create_directory(dir_path, mode, true);
        if (ErrorCode_Success != error_code) {
            return error_code;
        }
    }

    return ErrorCode_Success;
}

size_t find_first_of (const string& haystack, const char* needles, size_t search_start_pos, size_t& needle_ix) {
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

bool get_bounds_of_next_potential_var (const string& value, size_t& begin_pos, size_t& end_pos, bool& is_var) {
    const auto value_length = value.length();
    if (end_pos >= value_length) {
        return false;
    }

    is_var = false;
    bool contains_wildcard = false;
    while (false == is_var && false == contains_wildcard && begin_pos < value_length) {
        // Start search at end of last token
        begin_pos = end_pos;

        // Find next wildcard or non-delimiter
        bool is_escaped = false;
        for (; begin_pos < value_length; ++begin_pos) {
            char c = value[begin_pos];

            if (is_escaped) {
                is_escaped = false;

                if (false == is_delim(c)) {
                    // Found escaped non-delimiter, so reverse the index to retain the escape character
                    --begin_pos;
                    break;
                }
            } else if ('\\' == c) {
                // Escape character
                is_escaped = true;
            } else {
                if (is_wildcard(c)) {
                    contains_wildcard = true;
                    break;
                }
                if (false == is_delim(c)) {
                    break;
                }
            }
        }

        bool contains_decimal_digit = false;
        bool contains_alphabet = false;

        // Find next delimiter
        is_escaped = false;
        end_pos = begin_pos;
        for (; end_pos < value_length; ++end_pos) {
            char c = value[end_pos];

            if (is_escaped) {
                is_escaped = false;

                if (is_delim(c)) {
                    // Found escaped delimiter, so reverse the index to retain the escape character
                    --end_pos;
                    break;
                }
            } else if ('\\' == c) {
                // Escape character
                is_escaped = true;
            } else {
                if (is_wildcard(c)) {
                    contains_wildcard = true;
                } else if (is_delim(c)) {
                    // Found delimiter that's not also a wildcard
                    break;
                }
            }

            if (is_decimal_digit(c)) {
                contains_decimal_digit = true;
            } else if (is_alphabet(c)) {
                contains_alphabet = true;
            }
        }

        // Treat token as a definite variable if:
        // - it contains a decimal digit, or
        // - it could be a multi-digit hex value, or
        // - it's directly preceded by an equals sign and contains an alphabet without a wildcard between the equals sign and the first alphabet of the token
        if (contains_decimal_digit || could_be_multi_digit_hex_value(value, begin_pos, end_pos)) {
            is_var = true;
        } else if (begin_pos > 0 && '=' == value[begin_pos - 1] && contains_alphabet) {
            // Find first alphabet or wildcard in token
            is_escaped = false;
            bool found_wildcard_before_alphabet = false;
            for (auto i = begin_pos; i < end_pos; ++i) {
                auto c = value[i];

                if (is_escaped) {
                    is_escaped = false;

                    if (is_alphabet(c)) {
                        break;
                    }
                } else if ('\\' == c) {
                    // Escape character
                    is_escaped = true;
                } else if (is_wildcard(c)) {
                    found_wildcard_before_alphabet = true;
                    break;
                }
            }

            if (false == found_wildcard_before_alphabet) {
                is_var = true;
            }
        }
    }

    return (value_length != begin_pos);
}

bool get_bounds_of_next_var (const string& msg, size_t& begin_pos, size_t& end_pos) {
    const auto msg_length = msg.length();
    if (end_pos >= msg_length) {
        return false;
    }

    while (true) {
        begin_pos = end_pos;
        // Find next non-delimiter
        for (; begin_pos < msg_length; ++begin_pos) {
            if (false == is_delim(msg[begin_pos])) {
                break;
            }
        }
        if (msg_length == begin_pos) {
            // Early exit for performance
            return false;
        }

        bool contains_decimal_digit = false;
        bool contains_alphabet = false;

        // Find next delimiter
        end_pos = begin_pos;
        for (; end_pos < msg_length; ++end_pos) {
            char c = msg[end_pos];
            if (is_decimal_digit(c)) {
                contains_decimal_digit = true;
            } else if (is_alphabet(c)) {
                contains_alphabet = true;
            } else if (is_delim(c)) {
                break;
            }
        }

        // Treat token as variable if:
        // - it contains a decimal digit, or
        // - it's directly preceded by an equals sign and contains an alphabet, or
        // - it could be a multi-digit hex value
        if (contains_decimal_digit || (begin_pos > 0 && '=' == msg[begin_pos - 1] && contains_alphabet) ||
            could_be_multi_digit_hex_value(msg, begin_pos, end_pos))
        {
            break;
        }
    }

    return (msg_length != begin_pos);
}

string get_parent_directory_path (const string& path) {
    string dirname = get_unambiguous_path(path);

    size_t last_slash_pos = dirname.find_last_of('/');
    if (0 == last_slash_pos) {
        dirname = "/";
    } else if (string::npos == last_slash_pos) {
        dirname = ".";
    } else {
        dirname.resize(last_slash_pos);
    }

    return dirname;
}

string get_unambiguous_path (const string& path) {
    string unambiguous_path;
    if (path.empty()) {
        return unambiguous_path;
    }

    // Break path into components
    vector<string> path_components;
    boost::split(path_components, path, boost::is_any_of("/"), boost::token_compress_on);

    // Remove ambiguous components
    list<string> unambiguous_components;
    size_t num_components_to_ignore = 0;
    for (size_t i = path_components.size(); i-- > 0; ) {
        if (".." == path_components[i]) {
            ++num_components_to_ignore;
        } else if ("." == path_components[i] || path_components[i].empty()) {
            // Do nothing
        } else if (num_components_to_ignore > 0) {
            --num_components_to_ignore;
        } else {
            unambiguous_components.emplace_front(path_components[i]);
        }
    }

    // Assemble unambiguous path from leading slash (if any) and the unambiguous components
    if ('/' == path[0]) {
        unambiguous_path += '/';
    }
    if (!unambiguous_components.empty()) {
        unambiguous_path += boost::join(unambiguous_components, "/");
    }

    return unambiguous_path;
}

string replace_characters (const char* characters_to_replace, const char* replacement_characters, const string& value, bool escape) {
    string new_value;
    size_t search_start_pos = 0;
    while (true) {
        size_t replace_char_ix;
        size_t char_to_replace_pos = find_first_of(value, characters_to_replace, search_start_pos, replace_char_ix);
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

/**
 * Highly Efficient Wild Card Match - The Most Fundamental and Performance Critical Component of Search
 * @param cStringPtr_tame - c style string pointer pointing to string without wild characters
 * @param cStringPtr_wild - c style string pointer pointing to string with or without wild characters
 * @return - returns true if wild card matched, else returns false
 *
 * Basic wild card definition
 * 1) "*" refers to 0 or more ascii characters
 * 2) "?" refers to exactly 1 ascii character
 * 3) "\" refers to escape char, any char after this is treated as literal character rather than special function
 *      - only '\', '*', '?' can be escaped for now, others are simply ignored
 *          - the use of other unsupported escape characters causes wild card match to ignore the escape character
 *
 * Special properties of wild card match to take advantage of:
 * 1) When we encountered a mismatched character, we only need to rewind the wildcard string back to the previous "*"
 *    to continue searching. We can use position pointers to keep track of the nearest location we can rollback.
 *
 * Pros:
 * 1) No recursion, no need to worry about preventing stack overflow at expense of performance, easy to debug
 * 2) In place checking - no need to allocate string buffers, only char type buffers and pointers are used
 *
 * Future Work:
 * 1) Consider integrating token with space as delimiter in addition to "*" or "?"
 *      - maybe also differentiate variable or static token
 *
 * Notes:
 * 1) This is the highest performance version which only capable of performing case insensitive wild card match.
 *    It has all other functionality stripped away to achieve maximum performance.
 * 2) Performance is tuned to find matches extremely fast, however it is much slower to find mismatches becuase we want
 *    to gurantee 100% confidence that we correctly identified a mismatch.
 *    This is an important property which we need in CLG. It would be good to have algorithm tuned to find non-matches,
 *    but currently, there's no good algorithm available. The only optimization left is possibly distance based where
 *    it checks if the leftover tame string has enough characters match if "*" wa assumed to be 0 characters.
 *    Overall, this algorithm is possibly thousands of times if not more faster compared to a standard solution.
 */


bool wildCardMatch_caseSensitive(const string& string_tame, const string& cppString_wild){
    const char *cStringPtr_tame = string_tame.c_str();
    const char *cStringPtr_wild = cppString_wild.c_str();

    if (cppString_wild.empty()) {
        // Optimization: empty wild card search string
        //  - sometimes, clg users may search for an empty wild card search string
        //  - the default behavior should be to match everything
        return true;
    }

    bool wildCharNotEscaped;
    const char* bookmark_tame = nullptr;
    const char* bookmark_wild = nullptr;

    while (true) {
        wildCharNotEscaped = true;
        if (*cStringPtr_wild == '\\') {
            // encountered an escape character, set wildCharEscaped to true
            // Then directly advance to next character instead of taking another loop iteration
            wildCharNotEscaped = false;
            cStringPtr_wild++;
            if (!(*cStringPtr_wild == '*' or *cStringPtr_wild == '?' or *cStringPtr_wild == '\\')){
                // Silently ignore unsupported escape sequences for now by resetting the wildCharNotEscaped flag
                // Emit warning message if necessary in the future
                wildCharNotEscaped = true;
            }
        }

        if (wildCharNotEscaped && *cStringPtr_wild == '*'){
            // '*' character receives special treatment
            // Skip as much consecutive '*' as possible until we reach a non '*' character
            while (*(++cStringPtr_wild) == '*') { }

            // If we are at end of the c style wild string, we found a match
            if (*cStringPtr_wild == '\0') {
                return true;
            }

            // Save the bookmark_wild at the character after the last observed '*'
            // If the 1st new wild char is escape character, advance cStringPtr_wild to the next char to be used later
            bookmark_wild = cStringPtr_wild;
            if (*(cStringPtr_wild) == '\\'){
                cStringPtr_wild++;
                // Silently ignore unsupported escape sequences for now
                // Emit warning message if necessary in the future
            }

            // Efficiently fast-forward tame string pointer to the next possible match before saving the bookmark_tame
            if (*cStringPtr_wild != '?') {
                while (*cStringPtr_tame != *cStringPtr_wild) {
                    if (*(++cStringPtr_tame) == '\0') {
                        // If we reached the end of tame string at this point, return no match found
                        return false;
                    }
                }
            }
            bookmark_tame = cStringPtr_tame;
        } else if (*cStringPtr_tame != *cStringPtr_wild && wildCharNotEscaped && *cStringPtr_wild != '?') {
            // found a mismatch, rewind one or both bookmarks
            if (bookmark_wild) {
                if (cStringPtr_wild != bookmark_wild) {
                    cStringPtr_wild = bookmark_wild;
                    if (*cStringPtr_wild == '\\'){
                        // Handle case when bookmark starts with an escape character
                        cStringPtr_wild++;
                        // Silently ignore unsupported escape sequences for now
                        // Emit warning message if necessary in the future
                    }
                    if (*cStringPtr_tame != *cStringPtr_wild) {
                        // Don't go this far back again
                        cStringPtr_tame = ++bookmark_tame;
                        continue;   // "xy" matches "*y"
                    } else {
                        cStringPtr_wild++;
                    }
                }
                if (*cStringPtr_tame) {
                    cStringPtr_tame++;
                    continue;       // "mississippi" matches "*sip*"
                }
            }
            return false;           // "xy" doesn't match "x"
        }

        cStringPtr_tame++;
        cStringPtr_wild++;

        if (*cStringPtr_tame == '\0') {
            while (*cStringPtr_wild == '*') {
                cStringPtr_wild++;  // "x" matches "x*"
            }
            if (*cStringPtr_wild == '\0') {
                return true;        // "x" matches "x"
            } else if (*cStringPtr_wild == '\\') {
                // Silently ignore invalid escape sequences for now
                // Emit warning message if necessary in the future
                return true;
            } else {
                return false;       // "x" doesn't matches "xy"
            }
        }
    }
}

/**
 * Checks if a given string, tame, matches a string containing wildcards, wild.
 * Two wildcards are currently supported: '*' to match 0 or more characters, and '?' to match any single character.
 * Each can be escaped using a preceeding '\'. Other characters which are escaped are treated as normal characters.
 *
 * The algorithm basically works as follows:
 * Given a wild string "*abc*def*ghi*", it can be broken into groups of characters delimited by one or more '*'
 * characters. Essentially, the goal of the algorithm is then to determine whether the tame string contains each of
 * those groups in the same order.
 *
 * There are three additional points to handle:
 * 1. '?' must match any character,
 * 2. if there is no leading '*', then tame must start with the same group as wild, and
 * 3. if there is no trailing '*', then tame must end with the same group as wild.
 *
 * Thus, the algorithm:
 * 1. searches for the start of one of these groups in wild,
 * 2. searches for a group in tame starting with the same character, and then
 * 3. checks if the two match. If not, the search repeats with the next group in tame.
 *
 * @param tame The original string
 * @param wild The string containing wildcards
 * @return true if the two strings match (accounting for wildcards), false otherwise
 */
bool wildcard_match_simple (const string& tame, const string& wild) {
    const size_t cTameLen = tame.length();
    const size_t cWildLen = wild.length();
    int tame_ix = 0;
    int wild_ix = 0;
    int tame_bookmark_ix = -1;
    int wild_bookmark_ix = -1;
    bool escaped = false;
    while (true) {
        if ('\\' == wild[wild_ix]) {
            // Found escape character, so skip to next character
            escaped = true;
            ++wild_ix;

            // Should be impossible to have an escape character without something after it
            assert(cWildLen != wild_ix);
        }

        if (!escaped && '*' == wild[wild_ix]) {
            // Fast-forward wild_bookmark_ix to non-wildcard character
            while ('*' == wild[wild_ix]) {
                ++wild_ix;
            }
            if (cWildLen == wild_ix) {
                // Trailing '*' implies we don't care about anything after this point, so exit now
                return true;
            }

            // Bookmark wild and tame
            wild_bookmark_ix = wild_ix;
            tame_bookmark_ix = tame_ix;
        } else {
            if ( (!escaped && '?' == wild[wild_ix]) || tame[tame_ix] == wild[wild_ix]) {
                // Current characters in wild and tame match
                if (cWildLen == wild_ix && cTameLen == tame_ix) {
                    // Reached the end of both, so exit
                    return true;
                }

                ++wild_ix;
                ++tame_ix;
            } else {
                // Current characters in wild and tame don't match
                if (-1 == wild_bookmark_ix) {
                    // No bookmark indicates no '*', so no option to retry match
                    // Exit
                    return false;
                }

                // Reset to bookmark
                wild_ix = wild_bookmark_ix;
                char wild_char = wild[wild_ix];
                if ('\\' == wild_char) {
                    // Found escape chaacter, so skip to next character
                    wild_char = wild[wild_ix + 1];
                    escaped = true;
                }
                tame_ix = tame_bookmark_ix;

                if (!escaped && '?' == wild_char) {
                    if (cTameLen == tame_ix) {
                        // Reached end of tame before end of wild
                        return false;
                    }
                    ++tame_ix;
                } else {
                    // Fast-forward tame_bookmark_ix to next matching character
                    do {
                        if (cTameLen == tame_ix) {
                            // Reached end of tame before end of wild
                            return false;
                        }
                        ++tame_ix;
                    } while (tame[tame_ix] != wild_char);
                }
                tame_bookmark_ix = tame_ix;
            }
        }

        escaped = false;
    }
}

/***
 *
 * @param string_tame
 * @param string_wild
 * @param isCaseSensitive - defaults to case sensitive wild card search
 * @return
 */
bool wildCardMatch (
        const string& string_tame,        // A string without wildcards
        const string& string_wild,        // A (potentially) corresponding string with wildcards
        const bool isCaseSensitive = true   // Defaults to case sensitive wild card match (faster performance)
){
    if (isCaseSensitive) {
        return wildcard_match_simple(string_tame, string_wild);
    } else {
        return wildcard_match_simple(boost::to_upper_copy(string_tame), boost::to_upper_copy(string_wild));
    }
}

ErrorCode memory_map_file (const string& path, bool read_ahead, int& fd, size_t& file_size, void*& ptr) {
    fd = open(path.c_str(), O_RDONLY);
    if (fd < 0) {
        SPDLOG_ERROR("Failed to open {}, errno={}", path.c_str(), errno);
        return ErrorCode_errno;
    }

    // Check file exists and get size
    struct stat s = {};
    if (0 != fstat(fd, &s)) {
        if (ENOENT == errno) {
            return ErrorCode_FileNotFound;
        }
        SPDLOG_ERROR("Failed to stat {}, errno={}", path.c_str(), errno);
        return ErrorCode_errno;
    }
    file_size = s.st_size;

    if (0 == file_size) {
        if (0 != close(fd)) {
            SPDLOG_ERROR("Failed to close empty file - {}, errno={}", path.c_str(), errno);
            return ErrorCode_errno;
        }
    } else {
        int flags = MAP_SHARED;
        if (read_ahead) {
            flags |= MAP_POPULATE;
        } else {
            flags |= MAP_NONBLOCK;
        }
        void* mapped_region = mmap(nullptr, file_size, PROT_READ, flags, fd, 0);
        if (MAP_FAILED == mapped_region) {
            SPDLOG_ERROR("Failed to mmap {}, errno={}", path.c_str(), errno);
            return ErrorCode_errno;
        }
        ptr = mapped_region;
    }

    return ErrorCode_Success;
}

ErrorCode memory_unmap_file (int fd, size_t file_size, void* ptr) {
    if (0 != file_size) {
        if (0 != munmap(ptr, file_size)) {
            SPDLOG_ERROR("Failed to unmap file, errno={}", errno);
            return ErrorCode_errno;
        }
        if (0 != close(fd)) {
            SPDLOG_ERROR("Failed to close file, errno={}", errno);
            return ErrorCode_errno;
        }
    }

    return ErrorCode_Success;
}



ErrorCode read_list_of_paths (const string& list_path, vector<string>& paths) {
    FileReader file_reader;
    ErrorCode error_code = file_reader.try_open(list_path);
    if (ErrorCode_Success != error_code) {
        return error_code;
    }

    // Read file
    string line;
    while (true) {
        error_code = file_reader.try_read_to_delimiter('\n', false, false, line);
        if (ErrorCode_Success != error_code) {
            break;
        }
        // Only add non-empty paths
        if (line.empty() == false) {
            paths.push_back(line);
        }
    }
    // Check for any unexpected errors
    if (ErrorCode_EndOfFile != error_code) {
        return error_code;
    }

    file_reader.close();

    return ErrorCode_Success;
}
