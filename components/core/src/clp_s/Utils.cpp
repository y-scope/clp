#include "Utils.hpp"

#include <charconv>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <set>

#include <boost/url.hpp>
#include <fmt/format.h>
#include <spdlog/spdlog.h>
#include <string_utils/string_utils.hpp>

#if !CLP_S_EXCLUDE_LIBCURL
    #include "../clp/NetworkReader.hpp"
#endif
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

#if CLP_S_EXCLUDE_LIBCURL
auto
NetworkUtils::check_and_log_curl_error(std::string_view path, clp::ReaderInterface const* reader)
        -> bool {
    std::ignore = path;
    std::ignore = reader;
    return false;
}
#else
auto
NetworkUtils::check_and_log_curl_error(std::string_view path, clp::ReaderInterface const* reader)
        -> bool {
    auto const* network_reader = dynamic_cast<clp::NetworkReader const*>(reader);
    if (nullptr == network_reader) {
        return false;
    }
    if (auto const curl_error_info = network_reader->get_curl_error_info();
        curl_error_info.has_value())
    {
        SPDLOG_ERROR(
                "Encountered curl error while reading {} - Code: {} - Message: {}",
                path,
                static_cast<int64_t>(curl_error_info->code),
                curl_error_info->message
        );
        return true;
    }
    return false;
}
#endif

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
