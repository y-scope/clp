#include "CurlDownloadHandler.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include <curl/curl.h>
#include <fmt/core.h>

#include "ErrorCode.hpp"

namespace clp {
CurlDownloadHandler::CurlDownloadHandler(
        std::shared_ptr<ErrorMsgBuf> error_msg_buf,
        ProgressCallback progress_callback,
        WriteCallback write_callback,
        void* arg,
        std::string_view src_url,
        size_t offset,
        bool disable_caching,
        std::chrono::seconds connection_timeout,
        std::chrono::seconds overall_timeout,
        std::optional<std::unordered_map<std::string, std::string>> const& http_header_kv_pairs
)
        : m_error_msg_buf{std::move(error_msg_buf)} {
    if (nullptr != m_error_msg_buf) {
        // Set up error message buffer
        // According to the docs (https://curl.se/libcurl/c/CURLOPT_ERRORBUFFER.html), since 7.60.0,
        // a successful call to set `CURLOPT_ERRORBUFFER` will initialize the buffer to an empty
        // string 7.60.0. Since we require at least 7.68.0, we don't need to clear the provided
        // buffer before it's used.
        m_easy_handle.set_option(CURLOPT_ERRORBUFFER, m_error_msg_buf->data());
    }

    // Set up src url
    m_easy_handle.set_option(CURLOPT_URL, src_url.data());

    // Set up progress callback
    m_easy_handle.set_option(CURLOPT_XFERINFOFUNCTION, progress_callback);
    m_easy_handle.set_option(CURLOPT_XFERINFODATA, arg);
    m_easy_handle.set_option(CURLOPT_NOPROGRESS, 0);

    // Set up write callback
    m_easy_handle.set_option(CURLOPT_WRITEFUNCTION, write_callback);
    m_easy_handle.set_option(CURLOPT_WRITEDATA, arg);

    // Set up timeouts
    m_easy_handle.set_option(CURLOPT_CONNECTTIMEOUT, static_cast<long>(connection_timeout.count()));
    m_easy_handle.set_option(CURLOPT_TIMEOUT, static_cast<long>(overall_timeout.count()));

    // Set up http headers
    constexpr std::string_view cRangeHeaderName{"range"};
    constexpr std::string_view cCacheControlHeaderName{"cache-control"};
    constexpr std::string_view cPragmaHeaderName{"pragma"};
    std::unordered_set<std::string_view> const reserved_headers{
            cRangeHeaderName,
            cCacheControlHeaderName,
            cPragmaHeaderName
    };
    if (0 != offset) {
        m_http_headers.append(fmt::format("{}: bytes={}-", cRangeHeaderName, offset));
    }
    if (disable_caching) {
        m_http_headers.append(fmt::format("{}: no-cache", cCacheControlHeaderName));
        m_http_headers.append(fmt::format("{}: no-cache", cPragmaHeaderName));
    }
    if (http_header_kv_pairs.has_value()) {
        for (auto const& [key, value] : http_header_kv_pairs.value()) {
            // HTTP header field-name (key) is case-insensitive:
            // https://www.w3.org/Protocols/rfc2616/rfc2616-sec4.html#sec4.2
            // Therefore, we convert keys to lowercase for comparison with the reserved keys.
            // NOTE: We do not check for duplicate keys due to case insensitivity, leaving duplicate
            // handling to the server.
            auto lower_key{key};
            std::transform(
                    lower_key.begin(),
                    lower_key.end(),
                    lower_key.begin(),
                    [](unsigned char c) -> char {
                        // Implicitly cast the input character into `unsigned char` to avoid UB:
                        // https://en.cppreference.com/w/cpp/string/byte/tolower
                        return static_cast<char>(std::tolower(c));
                    }
            );
            if (reserved_headers.contains(lower_key) || value.ends_with("\r\n")) {
                throw CurlOperationFailed(
                        ErrorCode_Failure,
                        __FILE__,
                        __LINE__,
                        CURLE_BAD_FUNCTION_ARGUMENT,
                        fmt::format(
                                "`CurlDownloadHandler` failed to construct with the following "
                                "invalid header: {}:{}",
                                key,
                                value
                        )
                );
            }
            m_http_headers.append(fmt::format("{}: {}", key, value));
        }
    }
    if (false == m_http_headers.is_empty()) {
        m_easy_handle.set_option(CURLOPT_HTTPHEADER, m_http_headers.get_raw_list());
    }

    // Set up failure on HTTP error reponse
    m_easy_handle.set_option(CURLOPT_FAILONERROR, static_cast<long>(true));
}
}  // namespace clp
