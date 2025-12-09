#ifndef CLP_CURLDOWNLOADHANDLER_HPP
#define CLP_CURLDOWNLOADHANDLER_HPP

#include <array>
#include <chrono>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

#include <curl/curl.h>
#include <curl/system.h>

#include "CurlEasyHandle.hpp"
#include "CurlStringList.hpp"

namespace clp {
/**
 * A class to download data from a URL using libcurl. In particular, this class manages the
 * lifecycle of the libcurl handle and any arguments that libcurl uses during the download.
 */
class CurlDownloadHandler {
public:
    // Types
    using ErrorMsgBuf = std::array<char, CURL_ERROR_SIZE>;
    /**
     * libcurl progress callback. This method must have C linkage. Doc:
     * https://curl.se/libcurl/c/CURLOPT_WRITEFUNCTION.html
     */
    using ProgressCallback = int (*)(void*, curl_off_t, curl_off_t, curl_off_t, curl_off_t);
    /**
     * libcurl write callback. This method must have C linkage. Doc:
     * https://curl.se/libcurl/c/CURLOPT_WRITEFUNCTION.html
     */
    using WriteCallback = size_t (*)(char*, size_t, size_t, void*);

    // Constants
    // See https://curl.se/libcurl/c/CURLOPT_CONNECTTIMEOUT.html
    static constexpr std::chrono::seconds cDefaultConnectionTimeout{0};
    // See https://curl.se/libcurl/c/CURLOPT_TIMEOUT.html
    static constexpr std::chrono::seconds cDefaultOverallTimeout{0};

    // Constructor
    /**
     * @param error_msg_buf The buffer to store the CURL error message or `nullptr` if it shouldn't
     * be stored.
     * Doc: https://curl.se/libcurl/c/CURLOPT_ERRORBUFFER.html
     * @param progress_callback
     * @param write_callback
     * @param arg Argument to pass to `progress_callback` and `write_callback`
     * @param src_url
     * @param offset Index of the byte at which to start the download
     * @param disable_caching Whether to disable caching
     * @param connection_timeout Maximum time that the connection phase may take.
     * Doc: https://curl.se/libcurl/c/CURLOPT_CONNECTTIMEOUT.html
     * @param overall_timeout Maximum time that the transfer may take. Note that this includes
     * `connection_timeout`. Doc: https://curl.se/libcurl/c/CURLOPT_TIMEOUT.html
     * @param http_header_kv_pairs Key-value pairs representing HTTP headers to pass to the server
     * in the download request. Doc: https://curl.se/libcurl/c/CURLOPT_HTTPHEADER.html
     * @throw CurlOperationFailed if an error occurs.
     */
    explicit CurlDownloadHandler(
            std::shared_ptr<ErrorMsgBuf> error_msg_buf,
            ProgressCallback progress_callback,
            WriteCallback write_callback,
            void* arg,
            std::string_view src_url,
            size_t offset = 0,
            bool disable_caching = false,
            std::chrono::seconds connection_timeout = cDefaultConnectionTimeout,
            std::chrono::seconds overall_timeout = cDefaultOverallTimeout,
            std::optional<std::unordered_map<std::string, std::string>> const& http_header_kv_pairs
            = std::nullopt
    );

    // Disable copy/move constructors/assignment operators
    CurlDownloadHandler(CurlDownloadHandler const&) = delete;
    CurlDownloadHandler(CurlDownloadHandler&&) = delete;
    auto operator=(CurlDownloadHandler const&) -> CurlDownloadHandler& = delete;
    auto operator=(CurlDownloadHandler&&) -> CurlDownloadHandler& = delete;

    // Destructor
    ~CurlDownloadHandler() = default;

    // Methods
    /**
     * Starts downloading the data and only returns when the download completes or fails.
     * @return Same as clp::CurlEasyHandle::perform.
     */
    [[nodiscard]] auto perform() -> CURLcode { return m_easy_handle.perform(); }

private:
    /**
     * Locate the certificate authority(CA) bundle file on the current host.
     *
     * This provides consistent CA bundle resolution for curl on Linux systems when the runtime
     * environment differs from the build environment. MacOS and Windows rely on their
     * native certificate stores and are not checked. See also: https://curl.se/docs/sslcerts.html
     *
     * Resolution order:
     *   1. Environment variables CURL_CA_BUNDLE then SSL_CERT_FILE, if set.
     *   2. Well-known CA bundle paths.
     *   3. If nothing is found, return an empty string. The caller needs to check the output so as
     *      not to override curl defaults and let it use the platform's own certificate store or
     *      whatever it was built with.
     *
     * @return Absolute path to the CA bundle file.
     */
    static auto get_host_ca_bundle_path() -> std::string;

    static constexpr std::string_view cDebianCaBundlePath = "/etc/ssl/certs/ca-certificates.crt";
    static constexpr std::string_view cCentOsCaBundlePath = "/etc/pki/tls/certs/ca-bundle.crt";

    CurlEasyHandle m_easy_handle;
    CurlStringList m_http_headers;
    std::shared_ptr<ErrorMsgBuf> m_error_msg_buf;
};
}  // namespace clp

#endif
