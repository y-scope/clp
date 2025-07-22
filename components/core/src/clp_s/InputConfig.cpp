#include "InputConfig.hpp"

#include <cstdlib>
#include <exception>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <spdlog/spdlog.h>

#include "../clp/aws/AwsAuthenticationSigner.hpp"
#include "../clp/FileReader.hpp"
#include "../clp/NetworkReader.hpp"
#include "../clp/ReaderInterface.hpp"
#include "../clp/spdlog_with_specializations.hpp"
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

namespace {
auto try_create_file_reader(std::string_view const file_path)
        -> std::shared_ptr<clp::ReaderInterface> {
    try {
        return std::make_shared<clp::FileReader>(std::string{file_path});
    } catch (clp::FileReader::OperationFailed const& e) {
        SPDLOG_ERROR("Failed to open file for reading - {} - {}", file_path, e.what());
        return nullptr;
    }
}

auto try_sign_url(std::string& url) -> bool {
    auto const aws_access_key = std::getenv(cAwsAccessKeyIdEnvVar);
    auto const aws_secret_access_key = std::getenv(cAwsSecretAccessKeyEnvVar);
    if (nullptr == aws_access_key || nullptr == aws_secret_access_key) {
        SPDLOG_ERROR(
                "{} and {} environment variables not available for presigned url authentication.",
                cAwsAccessKeyIdEnvVar,
                cAwsSecretAccessKeyEnvVar
        );
        return false;
    }
    std::optional<std::string> optional_aws_session_token{std::nullopt};
    auto const aws_session_token = std::getenv(cAwsSessionTokenEnvVar);
    if (nullptr != aws_session_token) {
        optional_aws_session_token = std::string{aws_session_token};
    }

    clp::aws::AwsAuthenticationSigner signer{
            aws_access_key,
            aws_secret_access_key,
            optional_aws_session_token
    };

    try {
        clp::aws::S3Url s3_url{url};
        if (auto const rc = signer.generate_presigned_url(s3_url, url);
            clp::ErrorCode::ErrorCode_Success != rc)
        {
            return false;
        }
    } catch (std::exception const& e) {
        return false;
    }
    return true;
}

auto try_create_network_reader(std::string_view const url, NetworkAuthOption const& auth)
        -> std::shared_ptr<clp::ReaderInterface> {
    std::string request_url{url};
    switch (auth.method) {
        case AuthMethod::S3PresignedUrlV4:
            if (false == try_sign_url(request_url)) {
                return nullptr;
            }
            break;
        case AuthMethod::None:
            break;
        default:
            return nullptr;
    }

    try {
        return std::make_shared<clp::NetworkReader>(request_url);
    } catch (clp::NetworkReader::OperationFailed const& e) {
        SPDLOG_ERROR("Failed to open url for reading - {}", e.what());
        return nullptr;
    }
}
}  // namespace

auto try_create_reader(Path const& path, NetworkAuthOption const& network_auth)
        -> std::shared_ptr<clp::ReaderInterface> {
    if (InputSource::Filesystem == path.source) {
        return try_create_file_reader(path.path);
    } else if (InputSource::Network == path.source) {
        return try_create_network_reader(path.path, network_auth);
    } else {
        return nullptr;
    }
}
}  // namespace clp_s
