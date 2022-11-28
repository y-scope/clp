#include "Utils.hpp"

// C libraries
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

// C++ libraries
#include <algorithm>
#include <charconv>
#include <iostream>
#include <set>

// Boost libraries
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

// spdlog
#include <spdlog/spdlog.h>

// Project headers
#include "StringReader.hpp"

using std::list;
using std::string;
using std::string_view;
using std::vector;

static const char* const cWildcards = "?*";

// Local function prototypes
/**
 * Helper for ``do_wildcard_match_unsafe`` to advance the pointer in tame to
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

bool is_wildcard (char c) {
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
        boost::to_lower(lowercase_tame);
        string lowercase_wild(wild);
        boost::to_lower(lowercase_wild);
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
