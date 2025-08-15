#include "Utils.hpp"

#include <charconv>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <set>

#include <boost/url.hpp>
#include <fmt/core.h>
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
            if (clp::string_utils::is_decimal_digit(c)) {
                contains_decimal_digit = true;
            } else if (clp::string_utils::is_alphabet(c)) {
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
