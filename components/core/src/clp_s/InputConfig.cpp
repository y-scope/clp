#include "InputConfig.hpp"

#include <array>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <simdjson.h>
#include <spdlog/spdlog.h>

#if !CLP_S_EXCLUDE_LIBCURL
    #include "../clp/aws/AwsAuthenticationSigner.hpp"
#endif
#include "../clp/BufferedReader.hpp"
#include "../clp/ffi/ir_stream/protocol_constants.hpp"
#include "../clp/FileReader.hpp"
#if !CLP_S_EXCLUDE_LIBCURL
    #include "../clp/NetworkReader.hpp"
#endif
#include "../clp/ReaderInterface.hpp"
#include "../clp/spdlog_with_specializations.hpp"
#include "../clp/streaming_compression/Decompressor.hpp"
#include "../clp/streaming_compression/zstd/Decompressor.hpp"
#include "../clp/utf8_utils.hpp"
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
auto peek_start_and_deduce_type(std::shared_ptr<clp::BufferedReader>& reader) -> FileType;

/**
 * Checks if an input contains Zstd data, based on the first few bytes of data from the input.
 * @param peek_buf A pointer to a buffer containing peeked data from the start of an input stream.
 * @param peek_size The number of bytes of peeked data in the buffer.
 * @return Whether the input could be Zstd.
 */
auto could_be_zstd(char const* peek_buf, size_t peek_size) -> bool;

/**
 * Checks if an input contains KV-IR data, based on the first few bytes of data from the input.
 * @param peek_buf A pointer to a buffer containing peeked data from the start of an input stream.
 * @param peek_size The number of bytes of peeked data in the buffer.
 * @return Whether the input could be KV-IR.
 */
auto could_be_kvir(char const* peek_buf, size_t peek_size) -> bool;

/**
 * Checks if an input contains JSON data, based on the first few bytes of data from the input.
 * @param peek_buf A pointer to a buffer containing peeked data from the start of an input stream.
 * @param peek_size The number of bytes of peeked data in the buffer.
 * @return Whether the input could be JSON.
 */
auto could_be_json(char const* peek_buf, size_t peek_size) -> bool;

/**
 * Checks if an input contains logtext, based on the first few bytes of data from the input.
 * @param peek_buf A pointer to a buffer containing peeked data from the start of an input stream.
 * @param peek_size The number of bytes of peeked data in the buffer.
 * @return Whether the input could be logtext.
 */
auto could_be_logtext(char const* peek_buf, size_t peek_size) -> bool;

auto try_create_file_reader(std::string_view const file_path)
        -> std::shared_ptr<clp::ReaderInterface> {
    try {
        return std::make_shared<clp::FileReader>(std::string{file_path});
    } catch (clp::FileReader::OperationFailed const& e) {
        SPDLOG_ERROR("Failed to open file for reading - {} - {}", file_path, e.what());
        return nullptr;
    }
}

#if CLP_S_EXCLUDE_LIBCURL
auto try_create_network_reader(std::string_view const url, NetworkAuthOption const& auth)
        -> std::shared_ptr<clp::ReaderInterface> {
    std::ignore = url;
    std::ignore = auth;
    SPDLOG_ERROR("This build of clp-s does not support network inputs (libcurl excluded).");
    return nullptr;
}
#else
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
#endif

auto could_be_zstd(char const* peek_buf, size_t peek_size) -> bool {
    constexpr std::array<char, 4> cZstdMagicNumber = {'\x28', '\xB5', '\x2F', '\xFD'};
    if (peek_size < cZstdMagicNumber.size()) {
        return false;
    }

    return 0 == std::memcmp(peek_buf, cZstdMagicNumber.data(), cZstdMagicNumber.size());
}

auto could_be_kvir(char const* peek_buf, size_t peek_size) -> bool {
    if (peek_size < clp::ffi::ir_stream::cProtocol::MagicNumberLength) {
        return false;
    }

    return (0
            == std::memcmp(
                    peek_buf,
                    clp::ffi::ir_stream::cProtocol::FourByteEncodingMagicNumber,
                    clp::ffi::ir_stream::cProtocol::MagicNumberLength
            ))
           || (0
               == std::memcmp(
                       peek_buf,
                       clp::ffi::ir_stream::cProtocol::EightByteEncodingMagicNumber,
                       clp::ffi::ir_stream::cProtocol::MagicNumberLength
               ));
}

auto could_be_json(char const* peek_buf, size_t peek_size) -> bool {
    for (size_t i{0ULL}; i < peek_size; ++i) {
        if (std::isspace(static_cast<unsigned char>(peek_buf[i]))) {
            continue;
        }

        return '{' == peek_buf[i];
    }
    return false;
}

/**
 * This function decides whether the buffer could be logtext based on whether it contains valid, but
 * potentially truncated, UTF-8 data.
 *
 * To check for valid UTF-8 while accounting for truncation we need to:
 *   1. Find the last complete UTF-8 codepoint in the buffer
 *   2. Validate that all of the data in the buffer including the last complete codepoint is valid
 *      UTF-8.
 *
 * For the first step we can always find the last complete codepoint by scanning the last 7 bytes of
 * the buffer. This is because in the worst case, the stream terminates with a 4-byte codepoint
 * followed by another 4-byte codepoint with its last byte truncated. The approach, then, is to
 * scan the last 7 bytes of the buffer to locate the last complete codepoint. Since we use full
 * UTF-8 validation in the second step, this scan only needs to look for UTF-8 header bytes without
 * validating continuation bytes. If no complete codepoint can be found, we know that the input is
 * not valid UTF-8, otherwise we continue on to validating the entire buffer through the end of the
 * last complete codepoint.
 *
 * For the second step, we simply use an off-the-shelf fast UTF-8 validator.
 */
auto could_be_logtext(char const* peek_buf, size_t peek_size) -> bool {
    constexpr size_t cMaxUtf8CodepointBytes = 4ULL;
    constexpr size_t cMaxRunWithoutFullUtf8Codepoint = 2 * cMaxUtf8CodepointBytes - 1;

    size_t cur_byte{
            peek_size < cMaxRunWithoutFullUtf8Codepoint
                    ? size_t{0}
                    : (peek_size - cMaxRunWithoutFullUtf8Codepoint)
    };

    std::optional<size_t> legal_last_byte_index{std::nullopt};
    auto mark_last_legal_character = [&](size_t remaining_bytes_in_char) {
        auto const last_byte_in_char = cur_byte + remaining_bytes_in_char;
        if (last_byte_in_char < peek_size) {
            legal_last_byte_index = last_byte_in_char;
        }
        cur_byte = last_byte_in_char;
    };

    for (; cur_byte < peek_size; ++cur_byte) {
        uint8_t const c{static_cast<uint8_t>(peek_buf[cur_byte])};
        if ((clp::cFourByteUtf8CharHeaderMask & c) == clp::cFourByteUtf8CharHeader) {
            mark_last_legal_character(3ULL);
        } else if ((clp::cThreeByteUtf8CharHeaderMask & c) == clp::cThreeByteUtf8CharHeader) {
            mark_last_legal_character(2ULL);
        } else if ((clp::cTwoByteUtf8CharHeaderMask & c) == clp::cTwoByteUtf8CharHeader) {
            mark_last_legal_character(1ULL);
        } else if (clp::utf8_utils_internal::is_ascii_char(c)) {
            mark_last_legal_character(0ULL);
        }
    }

    if (false == legal_last_byte_index.has_value()) {
        return false;
    }

    return simdjson::validate_utf8(peek_buf, legal_last_byte_index.value() + 1ULL);
}

auto peek_start_and_deduce_type(std::shared_ptr<clp::BufferedReader>& reader) -> FileType {
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

    if (could_be_logtext(peek_buf, peek_size)) {
        return FileType::LogText;
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
        -> std::pair<std::vector<std::shared_ptr<clp::ReaderInterface>>, FileType> {
    constexpr size_t cFileReadBufferCapacity = 64 * 1024;  // 64 KiB
    constexpr size_t cMaxNestedFormatDepth = 5;
    if (nullptr == reader) {
        return {{}, FileType::Unknown};
    }

    size_t original_pos{};
    if (clp::ErrorCode::ErrorCode_Success != reader->try_get_pos(original_pos)
        || 0ULL != original_pos)
    {
        return {{}, FileType::Unknown};
    }

    std::vector<std::shared_ptr<clp::ReaderInterface>> readers{reader};
    for (size_t nesting_depth{0ULL}; nesting_depth < cMaxNestedFormatDepth; ++nesting_depth) {
        auto buffered_reader{
                std::make_shared<clp::BufferedReader>(readers.back(), cFileReadBufferCapacity)
        };
        auto const rc{buffered_reader->try_refill_buffer_if_empty()};
        if (clp::ErrorCode::ErrorCode_Success != rc && clp::ErrorCode::ErrorCode_EndOfFile != rc) {
            return {{}, FileType::Unknown};
        }

        auto const type{peek_start_and_deduce_type(buffered_reader)};
        readers.emplace_back(buffered_reader);
        switch (type) {
            case FileType::Json:
            case FileType::KeyValueIr:
            case FileType::LogText:
                return {std::move(readers), type};
            case FileType::Zstd: {
                readers.emplace_back(
                        std::make_shared<clp::streaming_compression::zstd::Decompressor>()
                );
                try {
                    std::static_pointer_cast<clp::streaming_compression::zstd::Decompressor>(
                            readers.back()
                    )
                            ->open(*buffered_reader, cFileReadBufferCapacity);
                } catch (std::exception const&) {
                    return {{}, FileType::Unknown};
                }
            } break;
            case FileType::Unknown:
            default:
                return {{}, FileType::Unknown};
        }
    }
    return {{}, FileType::Unknown};
}

void close_nested_readers(std::vector<std::shared_ptr<clp::ReaderInterface>> const& readers) {
    for (auto rit = readers.rbegin(); readers.rend() != rit; ++rit) {
        if (auto decompressor
            = std::dynamic_pointer_cast<clp::streaming_compression::Decompressor>(*rit);
            nullptr != decompressor)
        {
            decompressor->close();
        }
    }
}
}  // namespace clp_s
