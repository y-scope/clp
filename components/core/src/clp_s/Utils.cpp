#include "Utils.hpp"

#include <boost/filesystem.hpp>
#include <spdlog/spdlog.h>

using std::string;
using std::string_view;

namespace clp_s {
bool FileUtils::find_all_files(std::string const& path, std::vector<std::string>& file_paths) {
    try {
        if (false == boost::filesystem::is_directory(path)) {
            // path is a file
            file_paths.push_back(path);
            return true;
        }

        if (boost::filesystem::is_empty(path)) {
            // path is an empty directory
            return true;
        }

        // Iterate directory
        boost::filesystem::recursive_directory_iterator iter(
                path,
                boost::filesystem::directory_options::follow_directory_symlink
        );
        boost::filesystem::recursive_directory_iterator end;
        for (; iter != end; ++iter) {
            // Check if current entry is an empty directory or a file
            if (boost::filesystem::is_directory(iter->path())) {
                if (boost::filesystem::is_empty(iter->path())) {
                    iter.disable_recursion_pending();
                }
            } else {
                file_paths.push_back(iter->path().string());
            }
        }
    } catch (boost::filesystem::filesystem_error& exception) {
        SPDLOG_ERROR(
                "Failed to find files/directories at '{}' - {}.",
                path.c_str(),
                exception.what()
        );
        return false;
    }

    return true;
}

bool FileUtils::validate_path(std::vector<std::string> const& paths) {
    bool all_paths_exist = true;
    for (auto const& path : paths) {
        if (false == boost::filesystem::exists(path)) {
            SPDLOG_ERROR("'{}' does not exist.", path.c_str());
            all_paths_exist = false;
        }
    }

    return all_paths_exist;
}

bool StringUtils::get_bounds_of_next_var(string const& msg, size_t& begin_pos, size_t& end_pos) {
    auto const msg_length = msg.length();
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
        if (contains_decimal_digit
            || (begin_pos > 0 && '=' == msg[begin_pos - 1] && contains_alphabet)
            || could_be_multi_digit_hex_value(msg, begin_pos, end_pos))
        {
            break;
        }
    }

    return (msg_length != begin_pos);
}

size_t StringUtils::find_first_of(
        string const& haystack,
        char const* needles,
        size_t search_start_pos,
        size_t& needle_ix
) {
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

string StringUtils::replace_characters(
        char const* characters_to_escape,
        char const* replacement_characters,
        string const& value,
        bool escape
) {
    string new_value;
    size_t search_start_pos = 0;
    while (true) {
        size_t replace_char_ix;
        size_t char_to_replace_pos
                = find_first_of(value, characters_to_escape, search_start_pos, replace_char_ix);
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

void StringUtils::to_lower(string& str) {
    std::transform(str.cbegin(), str.cend(), str.begin(), [](unsigned char c) {
        return std::tolower(c);
    });
}

bool StringUtils::is_wildcard(char c) {
    static constexpr char cWildcards[] = "?*";
    for (size_t i = 0; i < strlen(cWildcards); ++i) {
        if (cWildcards[i] == c) {
            return true;
        }
    }
    return false;
}

bool StringUtils::has_unescaped_wildcards(std::string const& str) {
    for (size_t i = 0; i < str.size(); ++i) {
        if ('*' == str[i] || '?' == str[i]) {
            return true;
        }
        if ('\\' == str[i]) {
            ++i;
        }
    }
    return false;
}

string StringUtils::clean_up_wildcard_search_string(string_view str) {
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

bool StringUtils::advance_tame_to_next_match(
        char const*& tame_current,
        char const*& tame_bookmark,
        char const* tame_end,
        char const*& wild_current,
        char const*& wild_bookmark
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

bool StringUtils::wildcard_match_unsafe(
        string_view tame,
        string_view wild,
        bool case_sensitive_match
) {
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
bool StringUtils::wildcard_match_unsafe_case_sensitive(string_view tame, string_view wild) {
    auto const tame_length = tame.length();
    auto const wild_length = wild.length();
    char const* tame_current = tame.data();
    char const* wild_current = wild.data();
    char const* tame_bookmark = nullptr;
    char const* wild_bookmark = nullptr;
    char const* tame_end = tame_current + tame_length;
    char const* wild_end = wild_current + wild_length;

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
            if (!advance_tame_to_next_match(
                        tame_current,
                        tame_bookmark,
                        tame_end,
                        wild_current,
                        wild_bookmark
                ))
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
            if (false == ((false == is_escaped && '?' == w) || t == w)) {
                if (nullptr == wild_bookmark) {
                    // No bookmark to return to
                    return false;
                }

                wild_current = wild_bookmark;
                tame_current = tame_bookmark + 1;
                if (!advance_tame_to_next_match(
                            tame_current,
                            tame_bookmark,
                            tame_end,
                            wild_current,
                            wild_bookmark
                    ))
                {
                    return false;
                }
            }
        }

        ++tame_current;
        ++wild_current;

        // Handle reaching the end of tame or wild
        if (tame_end == tame_current) {
            return (wild_end == wild_current
                    || ('*' == *wild_current && (wild_current + 1) == wild_end));
        } else {
            if (wild_end == wild_current) {
                if (nullptr == wild_bookmark) {
                    // No bookmark to return to
                    return false;
                } else {
                    wild_current = wild_bookmark;
                    tame_current = tame_bookmark + 1;
                    if (!advance_tame_to_next_match(
                                tame_current,
                                tame_bookmark,
                                tame_end,
                                wild_current,
                                wild_bookmark
                        ))
                    {
                        return false;
                    }
                }
            }
        }
    }
}

bool StringUtils::convert_string_to_int64(std::string_view raw, int64_t& converted) {
    auto raw_end = raw.cend();
    auto result = std::from_chars(raw.cbegin(), raw_end, converted);
    if (raw_end != result.ptr) {
        return false;
    } else {
        return result.ec == std::errc();
    }
}

bool StringUtils::convert_string_to_double(std::string const& raw, double& converted) {
    if (raw.empty()) {
        // Can't convert an empty string
        return false;
    }

    char const* c_str = raw.c_str();
    char* end_ptr;
    // Reset errno so we can detect a new error
    errno = 0;
    double raw_as_double = strtod(c_str, &end_ptr);
    if (ERANGE == errno || (end_ptr - c_str) < raw.length()) {
        return false;
    }
    converted = raw_as_double;
    return true;
}

void StringUtils::tokenize_column_descriptor(
        std::string const& descriptor,
        std::vector<std::string>& tokens
) {
    // TODO: handle escaped . correctly
    auto start = 0U;
    auto end = descriptor.find('.');
    while (end != std::string::npos) {
        tokens.push_back(descriptor.substr(start, end - start));
        start = end + 1;
        end = descriptor.find('.', start);
    }
    tokens.push_back(descriptor.substr(start));
}
}  // namespace clp_s
