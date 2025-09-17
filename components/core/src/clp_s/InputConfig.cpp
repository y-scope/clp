#include "InputConfig.hpp"

#include <array>
#include <cctype>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <spdlog/spdlog.h>

#include "../clp/aws/AwsAuthenticationSigner.hpp"
#include "../clp/BufferedFileReader.hpp"
#include "../clp/ffi/ir_stream/protocol_constants.hpp"
#include "../clp/FileReader.hpp"
#include "../clp/NetworkReader.hpp"
#include "../clp/ReaderInterface.hpp"
#include "../clp/spdlog_with_specializations.hpp"
#include "../clp/streaming_compression/zstd/Decompressor.hpp"
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
/**
 * Peeks the first few bytes of input from a reader and tries to deduce the type of the underlying
 * content.
 * @param reader
 * @return The deduced type, or `FileType::Unknown` if the type could not be deduced.
 */
auto peek_start_and_deduce_type(std::shared_ptr<clp::BufferedFileReader>& reader) -> FileType;

/**
 * Checks if an input contains Zstd data, based on the first few bytes of data from the input.
 * @return Whether the input could be Zstd.
 */
auto could_be_zstd(char const* peek_buf, size_t peek_size) -> bool;

/**
 * Checks if an input contains KV-IR data, based on the first few bytes of data from the input.
 * @return Whether the input could be KV-IR.
 */
auto could_be_kvir(char const* peek_buf, size_t peek_size) -> bool;

/**
 * Checks if an input contains JSON data, based on the first few bytes of data from the input.
 * @return Whether the input could be JSON.
 */
auto could_be_json(char const* peek_buf, size_t peek_size) -> bool;

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

auto could_be_zstd(char const* peek_buf, size_t peek_size) -> bool {
    constexpr std::array<char, 4> cZstdMagicNumber = {'\x28', '\xB5', '\x2F', '\xFD'};
    if (peek_size < cZstdMagicNumber.size()) {
        return false;
    }

    for (size_t i{0ULL}; i < cZstdMagicNumber.size(); ++i) {
        if (cZstdMagicNumber.at(i) != peek_buf[i]) {
            return false;
        }
    }
    return true;
}

auto could_be_kvir(char const* peek_buf, size_t peek_size) -> bool {
    if (peek_size < clp::ffi::ir_stream::cProtocol::MagicNumberLength) {
        return false;
    }

    bool could_be_four_byte{true};
    bool could_be_eight_byte{true};
    for (size_t i{0ULL}; i < clp::ffi::ir_stream::cProtocol::MagicNumberLength; ++i) {
        could_be_four_byte
                &= clp::ffi::ir_stream::cProtocol::FourByteEncodingMagicNumber[i] == peek_buf[i];
        could_be_eight_byte
                &= clp::ffi::ir_stream::cProtocol::EightByteEncodingMagicNumber[i] == peek_buf[i];
    }
    return could_be_four_byte || could_be_eight_byte;
}

auto could_be_json(char const* peek_buf, size_t peek_size) -> bool {
    for (size_t i{0ULL}; i < peek_size; ++i) {
        if (std::isspace(static_cast<int>(peek_buf[i]))) {
            continue;
        }

        if ('{' == peek_buf[i]) {
            return true;
        } else {
            return false;
        }
    }
    return false;
}

auto peek_start_and_deduce_type(std::shared_ptr<clp::BufferedFileReader>& reader) -> FileType {
    char const* peek_buf{};
    size_t peek_size{};
    reader->peek_buffered_data(peek_buf, peek_size);
    if (nullptr == peek_buf || 0 == peek_size) {
        return FileType::Unknown;
    }

    if (could_be_zstd(peek_buf, peek_size)) {
        return FileType::Zstd;
    }

    if (could_be_kvir(peek_buf, peek_size)) {
        return FileType::KeyValueIr;
    }

    if (could_be_json(peek_buf, peek_size)) {
        return FileType::Json;
    }

    return FileType::Unknown;
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

[[nodiscard]] auto try_deduce_reader_type(std::shared_ptr<clp::ReaderInterface> reader)
        -> std::pair<std::shared_ptr<clp::ReaderInterface>, FileType> {
    constexpr size_t cFileReadBufferCapacity = 64 * 1024;  // 64 KB
    constexpr size_t cMaxNestedFormatDepth = 5;
    if (nullptr == reader) {
        return std::make_pair(nullptr, FileType::Unknown);
    }

    size_t original_pos{};
    if (clp::ErrorCode::ErrorCode_Success != reader->try_get_pos(original_pos)
        || 0ULL != original_pos)
    {
        return std::make_pair(nullptr, FileType::Unknown);
    }

    auto prev_reader{reader};
    for (size_t nesting_depth{0ULL}; nesting_depth < cMaxNestedFormatDepth; ++nesting_depth) {
        auto buffered_reader{
                std::make_shared<clp::BufferedFileReader>(prev_reader, cFileReadBufferCapacity)
        };
        auto const rc{buffered_reader->try_refill_buffer_if_empty()};
        if (clp::ErrorCode::ErrorCode_Success != rc && clp::ErrorCode::ErrorCode_EndOfFile != rc) {
            return std::make_pair(nullptr, FileType::Unknown);
        }

        auto const type{peek_start_and_deduce_type(buffered_reader)};
        switch (type) {
            case FileType::Json:
            case FileType::KeyValueIr:
                return std::make_pair(buffered_reader, type);
            case FileType::Zstd: {
                prev_reader = std::make_shared<clp::streaming_compression::zstd::Decompressor>();
                try {
                    std::static_pointer_cast<clp::streaming_compression::zstd::Decompressor>(
                            prev_reader
                    )
                            ->open(*buffered_reader, cFileReadBufferCapacity);
                } catch (std::exception const&) {
                    return std::make_pair(nullptr, FileType::Unknown);
                }
            }
            case FileType::Unknown:
            default:
                return std::make_pair(nullptr, FileType::Unknown);
        }
    }
    return std::make_pair(nullptr, FileType::Unknown);
}
}  // namespace clp_s
