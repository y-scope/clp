#include "Utils.hpp"

#include <charconv>
#include <cstdint>
#include <cstring>
#include <exception>
#include <filesystem>
#include <set>

#include <boost/url.hpp>
#include <fmt/format.h>
#include <spdlog/spdlog.h>
#include <string_utils/string_utils.hpp>

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

JsonSanitizeResult StringUtils::sanitize_json_buffer(
        char*& buf,
        size_t& buf_size,
        size_t buf_occupied,
        size_t simdjson_padding
) {
    // Build sanitized content in a temporary buffer, escaping control characters
    // inside JSON strings. This is a fallback path only triggered on UNESCAPED_CHARS error,
    // so we prioritize correctness over performance.
    //
    // Note: If the buffer contains unmatched quotes (e.g., truncated JSON), the string state
    // tracking may be incorrect, potentially escaping control characters outside of actual
    // JSON strings. This is acceptable since such malformed JSON will fail parsing anyway.
    std::string sanitized;

    std::map<char, size_t> sanitized_char_counts;
    bool in_string = false;
    bool escape_next = false;

    for (size_t i = 0; i < buf_occupied; ++i) {
        char c = buf[i];

        if (escape_next) {
            escape_next = false;
            sanitized.push_back(c);
            continue;
        }

        if (c == '\\' && in_string) {
            escape_next = true;
            sanitized.push_back(c);
            continue;
        }

        if (c == '"') {
            in_string = !in_string;
            sanitized.push_back(c);
            continue;
        }

        // Escape control characters (0x00-0x1F) inside strings to \u00XX format
        if (in_string && static_cast<unsigned char>(c) < 0x20) {
            char_to_escaped_four_char_hex(sanitized, c);
            ++sanitized_char_counts[c];
        } else {
            sanitized.push_back(c);
        }
    }

    // If no changes were made, return early
    if (sanitized.size() == buf_occupied) {
        return {buf_occupied, {}};
    }

    // Grow buffer if needed to hold sanitized content
    if (sanitized.size() > buf_size) {
        // Allocate exactly the size needed since we know the exact size upfront
        size_t new_buf_size = sanitized.size();
        char* new_buf = new char[new_buf_size + simdjson_padding];
        delete[] buf;
        buf = new_buf;
        buf_size = new_buf_size;
    }

    // Copy sanitized content to buffer
    std::memcpy(buf, sanitized.data(), sanitized.size());
    return {sanitized.size(), std::move(sanitized_char_counts)};
}

namespace {
/**
 * Checks if a byte is a valid UTF-8 continuation byte (10xxxxxx pattern).
 * @param byte The byte to check
 * @return true if the byte is a valid continuation byte (0x80-0xBF)
 */
constexpr bool is_continuation_byte(unsigned char byte) {
    return (byte & 0xC0) == 0x80;
}

/**
 * Validates a UTF-8 sequence starting at the given position and returns the sequence length.
 * @param buf The buffer containing the UTF-8 data
 * @param pos Current position in the buffer
 * @param buf_occupied Total bytes in the buffer
 * @return The length of the valid UTF-8 sequence (1-4), or 0 if invalid
 */
size_t validate_utf8_sequence(char const* buf, size_t pos, size_t buf_occupied) {
    auto const byte = static_cast<unsigned char>(buf[pos]);
    size_t remaining = buf_occupied - pos;

    // ASCII (0x00-0x7F)
    if (byte <= 0x7F) {
        return 1;
    }

    // Invalid: continuation byte without leader, or invalid lead bytes
    if (byte < 0xC2 || byte > 0xF4) {
        return 0;
    }

    // 2-byte sequence (0xC2-0xDF)
    if (byte <= 0xDF) {
        if (remaining < 2 || !is_continuation_byte(static_cast<unsigned char>(buf[pos + 1]))) {
            return 0;
        }
        return 2;
    }

    // 3-byte sequence (0xE0-0xEF)
    if (byte <= 0xEF) {
        if (remaining < 3) {
            return 0;
        }
        auto const byte2 = static_cast<unsigned char>(buf[pos + 1]);
        auto const byte3 = static_cast<unsigned char>(buf[pos + 2]);

        if (!is_continuation_byte(byte2) || !is_continuation_byte(byte3)) {
            return 0;
        }

        // Check for overlong encoding (E0 requires second byte >= 0xA0)
        if (byte == 0xE0 && byte2 < 0xA0) {
            return 0;
        }

        // Check for surrogate code points (ED with second byte >= 0xA0 means 0xD800-0xDFFF)
        if (byte == 0xED && byte2 >= 0xA0) {
            return 0;
        }

        return 3;
    }

    // 4-byte sequence (0xF0-0xF4)
    if (remaining < 4) {
        return 0;
    }
    auto const byte2 = static_cast<unsigned char>(buf[pos + 1]);
    auto const byte3 = static_cast<unsigned char>(buf[pos + 2]);
    auto const byte4 = static_cast<unsigned char>(buf[pos + 3]);

    if (!is_continuation_byte(byte2) || !is_continuation_byte(byte3)
        || !is_continuation_byte(byte4))
    {
        return 0;
    }

    // Check for overlong encoding (F0 requires second byte >= 0x90)
    if (byte == 0xF0 && byte2 < 0x90) {
        return 0;
    }

    // Check for code points > U+10FFFF (F4 requires second byte <= 0x8F)
    if (byte == 0xF4 && byte2 > 0x8F) {
        return 0;
    }

    return 4;
}
}  // namespace

JsonSanitizeResult StringUtils::sanitize_utf8_buffer(
        char*& buf,
        size_t& buf_size,
        size_t buf_occupied,
        size_t simdjson_padding
) {
    // Build sanitized content in a temporary buffer, replacing invalid UTF-8 sequences
    // with the Unicode replacement character U+FFFD (0xEF 0xBF 0xBD).
    std::string sanitized;

    // We use a special key to count invalid UTF-8 sequences
    // Using 0xFF as the key since it's never a valid UTF-8 byte
    std::map<char, size_t> sanitized_char_counts;
    constexpr char cInvalidUtf8Key = static_cast<char>(0xFF);

    size_t i = 0;
    while (i < buf_occupied) {
        size_t seq_len = validate_utf8_sequence(buf, i, buf_occupied);
        if (seq_len > 0) {
            // Valid sequence - copy it
            sanitized.append(buf + i, seq_len);
            i += seq_len;
        } else {
            // Invalid sequence - replace with U+FFFD
            sanitized.append("\xEF\xBF\xBD", 3);
            ++sanitized_char_counts[cInvalidUtf8Key];
            // Skip one byte and continue (maximal subpart replacement strategy)
            ++i;
        }
    }

    // If no changes were made, return early
    if (sanitized.size() == buf_occupied) {
        return {buf_occupied, {}};
    }

    // Grow buffer if needed to hold sanitized content
    if (sanitized.size() > buf_size) {
        size_t new_buf_size = sanitized.size();
        char* new_buf = new char[new_buf_size + simdjson_padding];
        delete[] buf;
        buf = new_buf;
        buf_size = new_buf_size;
    }

    // Copy sanitized content to buffer
    std::memcpy(buf, sanitized.data(), sanitized.size());
    return {sanitized.size(), std::move(sanitized_char_counts)};
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
}  // namespace clp_s
