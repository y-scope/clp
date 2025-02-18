#include "Utils.hpp"

#include <charconv>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <set>

#include <boost/url.hpp>
#include <fmt/core.h>
#include <simdjson.h>
#include <spdlog/spdlog.h>

#include "archive_constants.hpp"

using std::string;
using std::string_view;

namespace clp_s {
bool FileUtils::find_all_files_in_directory(
        std::string const& path,
        std::vector<std::string>& file_paths
) {
    try {
        if (false == std::filesystem::is_directory(path)) {
            // path is a file
            file_paths.push_back(path);
            return true;
        }

        if (std::filesystem::is_empty(path)) {
            // path is an empty directory
            return true;
        }

        // Iterate directory
        std::filesystem::recursive_directory_iterator iter(
                path,
                std::filesystem::directory_options::follow_directory_symlink
        );
        std::filesystem::recursive_directory_iterator end;
        for (; iter != end; ++iter) {
            // Check if current entry is an empty directory or a file
            if (std::filesystem::is_directory(iter->path())) {
                if (std::filesystem::is_empty(iter->path())) {
                    iter.disable_recursion_pending();
                }
            } else {
                file_paths.push_back(iter->path().string());
            }
        }
    } catch (std::exception const& exception) {
        SPDLOG_ERROR(
                "Failed to find files/directories at '{}' - {}.",
                path.c_str(),
                exception.what()
        );
        return false;
    }

    return true;
}

namespace {
/**
 * Determines if a directory is a multi-file archive.
 * @param path
 * @return true if this directory is a multi-file archive, false otherwise
 */
bool is_multi_file_archive(std::string_view const path) {
    for (auto const& entry : std::filesystem::directory_iterator{path}) {
        if (entry.is_directory()) {
            return false;
        }

        std::string file_name;
        if (false == FileUtils::get_last_non_empty_path_component(entry.path().string(), file_name))
        {
            return false;
        }
        auto formatted_name = fmt::format("/{}", file_name);
        if (constants::cArchiveHeaderFile == formatted_name
            || constants::cArchiveSchemaTreeFile == formatted_name
            || constants::cArchiveSchemaMapFile == formatted_name
            || constants::cArchiveVarDictFile == formatted_name
            || constants::cArchiveLogDictFile == formatted_name
            || constants::cArchiveArrayDictFile == formatted_name
            || constants::cArchiveTableMetadataFile == formatted_name)
        {
            continue;
        } else {
            uint64_t segment_file_number{};
            auto const* begin = file_name.data();
            auto const* end = file_name.data() + file_name.size();
            auto [last, rc] = std::from_chars(begin, end, segment_file_number);
            if (std::errc{} != rc || last != end) {
                return false;
            }
        }
    }
    return true;
}
}  // namespace

bool FileUtils::find_all_archives_in_directory(
        std::string_view const path,
        std::vector<std::string>& archive_paths
) {
    try {
        if (false == std::filesystem::is_directory(path)) {
            return false;
        }
    } catch (std::exception const& e) {
        return false;
    }

    if (is_multi_file_archive(path)) {
        archive_paths.emplace_back(path);
        return true;
    }

    for (auto const& entry : std::filesystem::directory_iterator{path}) {
        archive_paths.emplace_back(entry.path().string());
    }
    return true;
}

bool FileUtils::get_last_non_empty_path_component(std::string_view const path, std::string& name) {
    std::filesystem::path fs_path;
    try {
        fs_path = std::filesystem::path{path}.lexically_normal();
    } catch (std::exception const& e) {
        return false;
    }

    if (fs_path.has_filename() && false == fs_path.filename().string().empty()) {
        name = fs_path.filename().string();
        return true;
    }

    while (fs_path.has_parent_path()) {
        fs_path = fs_path.parent_path();
        if (fs_path.has_filename() && false == fs_path.filename().string().empty()) {
            name = fs_path.filename().string();
            return true;
        }
    }

    return false;
}

bool UriUtils::get_last_uri_component(std::string_view const uri, std::string& name) {
    auto parsed_result = boost::urls::parse_uri(uri);
    if (false == parsed_result.has_value()) {
        return false;
    }
    auto parsed_uri = parsed_result.value();
    auto path_segments_view = parsed_uri.segments();
    if (path_segments_view.empty()) {
        return false;
    }
    name = path_segments_view.back();
    return true;
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

bool
StringUtils::wildcard_match_unsafe(string_view tame, string_view wild, bool case_sensitive_match) {
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

bool StringUtils::tokenize_column_descriptor(
        std::string const& descriptor,
        std::vector<std::string>& tokens
) {
    std::string cur_tok;
    bool escaped{false};
    for (size_t i = 0; i < descriptor.size(); ++i) {
        if (false == escaped) {
            if ('\\' == descriptor[i]) {
                escaped = true;
            } else if ('.' == descriptor[i]) {
                if (cur_tok.empty()) {
                    return false;
                }
                std::string unescaped_token;
                if (unescape_kql_internal(cur_tok, unescaped_token, false)) {
                    tokens.push_back(unescaped_token);
                    cur_tok.clear();
                } else {
                    return false;
                }
            } else {
                cur_tok.push_back(descriptor[i]);
            }
            continue;
        }

        escaped = false;
        switch (descriptor[i]) {
            case '.':
                cur_tok.push_back('.');
                break;
            default:
                cur_tok.push_back('\\');
                cur_tok.push_back(descriptor[i]);
                break;
        }
    }

    if (escaped) {
        return false;
    }

    if (cur_tok.empty()) {
        return false;
    }

    std::string unescaped_token;
    if (unescape_kql_internal(cur_tok, unescaped_token, false)) {
        tokens.push_back(unescaped_token);
    } else {
        return false;
    }
    return true;
}

void StringUtils::escape_json_string(std::string& destination, std::string_view const source) {
    // Escaping is implemented using this `append_unescaped_slice` approach to offer a fast path
    // when strings are mostly or entirely valid escaped JSON. Benchmarking shows that this offers
    // a net decompression speedup of ~30% compared to adding every character to the destination one
    // character at a time.
    size_t slice_begin{0ULL};
    auto append_unescaped_slice = [&](size_t i) {
        if (slice_begin < i) {
            destination.append(source.substr(slice_begin, i - slice_begin));
        }
        slice_begin = i + 1;
    };
    for (size_t i = 0; i < source.size(); ++i) {
        char c = source[i];
        switch (c) {
            case '"':
                append_unescaped_slice(i);
                destination.append("\\\"");
                break;
            case '\\':
                append_unescaped_slice(i);
                destination.append("\\\\");
                break;
            case '\t':
                append_unescaped_slice(i);
                destination.append("\\t");
                break;
            case '\r':
                append_unescaped_slice(i);
                destination.append("\\r");
                break;
            case '\n':
                append_unescaped_slice(i);
                destination.append("\\n");
                break;
            case '\b':
                append_unescaped_slice(i);
                destination.append("\\b");
                break;
            case '\f':
                append_unescaped_slice(i);
                destination.append("\\f");
                break;
            default:
                if ('\x00' <= c && c <= '\x1f') {
                    append_unescaped_slice(i);
                    char_to_escaped_four_char_hex(destination, c);
                }
                break;
        }
    }
    append_unescaped_slice(source.size());
}

namespace {
/**
 * Converts a four byte hex sequence to utf8. We perform the conversion in this cumbersome way
 * because c++20 deprecates most of the much more convenient std::codecvt utilities.
 */
bool convert_four_byte_hex_to_utf8(std::string_view const hex, std::string& destination) {
    std::string buf = "\"\\u";
    buf += hex;
    buf.push_back('"');
    buf.reserve(buf.size() + simdjson::SIMDJSON_PADDING);
    simdjson::ondemand::parser parser;
    auto value = parser.iterate(buf);
    try {
        if (false == value.is_scalar()) {
            return false;
        }

        if (simdjson::ondemand::json_type::string != value.type()) {
            return false;
        }

        std::string_view unescaped_utf8 = value.get_string(false);
        destination.append(unescaped_utf8);
    } catch (std::exception const& e) {
        return false;
    }
    return true;
}
}  // namespace

bool StringUtils::unescape_kql_value(std::string const& value, std::string& unescaped) {
    return unescape_kql_internal(value, unescaped, true);
}

bool StringUtils::unescape_kql_internal(
        std::string const& value,
        std::string& unescaped,
        bool is_value
) {
    bool escaped{false};
    for (size_t i = 0; i < value.size(); ++i) {
        if (false == escaped) {
            if ('\\' == value[i]) {
                escaped = true;
            } else {
                unescaped.push_back(value[i]);
            }
            continue;
        }

        escaped = false;
        switch (value[i]) {
            case '\\':
                unescaped.append("\\\\");
                break;
            case '"':
                unescaped.push_back('"');
                break;
            case 't':
                unescaped.push_back('\t');
                break;
            case 'r':
                unescaped.push_back('\r');
                break;
            case 'n':
                unescaped.push_back('\n');
                break;
            case 'b':
                unescaped.push_back('\b');
                break;
            case 'f':
                unescaped.push_back('\f');
                break;
            case 'u': {
                size_t last_char_in_codepoint = i + 4;
                if (value.size() <= last_char_in_codepoint) {
                    return false;
                }

                auto four_byte_hex = std::string_view{value}.substr(i + 1, 4);
                i += 4;

                std::string tmp;
                if (false == convert_four_byte_hex_to_utf8(four_byte_hex, tmp)) {
                    return false;
                }

                // Make sure unicode escape sequences are always treated as literal characters
                if ("\\" == tmp) {
                    unescaped.append("\\\\");
                } else if ("?" == tmp && is_value) {
                    unescaped.append("\\?");
                } else if ("*" == tmp) {
                    unescaped.append("\\*");
                } else {
                    unescaped.append(tmp);
                }
                break;
            }
            case '{':
                unescaped.push_back('{');
                break;
            case '}':
                unescaped.push_back('}');
                break;
            case '(':
                unescaped.push_back('(');
                break;
            case ')':
                unescaped.push_back(')');
                break;
            case '<':
                unescaped.push_back('<');
                break;
            case '>':
                unescaped.push_back('>');
                break;
            case '*':
                unescaped.append("\\*");
                break;
            case '?':
                if (is_value) {
                    unescaped.append("\\?");
                } else {
                    unescaped.push_back('?');
                }
                break;
            default:
                return false;
        }
    }

    if (escaped) {
        return false;
    }
    return true;
}
}  // namespace clp_s
