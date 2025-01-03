#include "InputConfig.hpp"

#include <exception>
#include <filesystem>
#include <string>
#include <vector>

#include "Utils.hpp"

namespace clp_s {
auto get_source_for_path(std::string_view const path) -> InputSource {
    try {
        return std::filesystem::exists(path) ? InputSource::Filesystem : InputSource::Network;
    } catch (std::exception const& e) {
        return InputSource::Network;
    }
}

auto get_path_object_for_raw_path(std::string_view const path) -> Path {
    return Path{.source = get_source_for_path(path), .path = std::string{path}};
}

auto get_input_files_for_raw_path(std::string_view const path, std::vector<Path>& files) -> bool {
    return get_input_files_for_path(get_path_object_for_raw_path(path), files);
}

auto get_input_files_for_path(Path const& path, std::vector<Path>& files) -> bool {
    if (InputSource::Network == path.source) {
        files.emplace_back(path);
        return true;
    }

    if (false == std::filesystem::is_directory(path.path)) {
        files.emplace_back(path);
        return true;
    }

    std::vector<std::string> file_paths;
    if (false == FileUtils::find_all_files_in_directory(path.path, file_paths)) {
        return false;
    }

    for (auto& file : file_paths) {
        files.emplace_back(Path{.source = InputSource::Filesystem, .path = std::move(file)});
    }
    return true;
}

auto get_input_archives_for_raw_path(std::string_view const path, std::vector<Path>& archives)
        -> bool {
    return get_input_archives_for_path(get_path_object_for_raw_path(path), archives);
}

auto get_input_archives_for_path(Path const& path, std::vector<Path>& archives) -> bool {
    if (InputSource::Network == path.source) {
        archives.emplace_back(path);
        return true;
    }

    if (false == std::filesystem::is_directory(path.path)) {
        archives.emplace_back(path);
        return true;
    }

    std::vector<std::string> archive_paths;
    if (false == FileUtils::find_all_archives_in_directory(path.path, archive_paths)) {
        return false;
    }

    for (auto& archive : archive_paths) {
        archives.emplace_back(Path{.source = InputSource::Filesystem, .path = std::move(archive)});
    }
    return true;
}

auto get_archive_id_from_path(Path const& archive_path, std::string& archive_id) -> bool {
    switch (archive_path.source) {
        case InputSource::Network:
            return UriUtils::get_last_uri_component(archive_path.path, archive_id);
        case InputSource::Filesystem:
            return FileUtils::get_last_non_empty_path_component(archive_path.path, archive_id);
        default:
            return false;
    }
    return true;
}

}  // namespace clp_s
