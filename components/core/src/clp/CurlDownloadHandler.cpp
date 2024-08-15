#include "CurlDownloadHandler.hpp"

#include <chrono>
#include <cstddef>
#include <span>
#include <string>
#include <string_view>

#include <curl/curl.h>

#include "CurlOperationFailed.hpp"
#include "ErrorCode.hpp"

namespace clp {
CurlDownloadHandler::CurlDownloadHandler(
        std::span<char> error_msg_buf,
        ProgressCallback progress_callback,
        WriteCallback write_callback,
        void* arg,
        std::string_view src_url,
        size_t offset,
        bool disable_caching,
        std::chrono::seconds connection_timeout,
        std::chrono::seconds overall_timeout
) {
    if (error_msg_buf.empty() || nullptr == error_msg_buf.data()
        || error_msg_buf.size() < CURL_ERROR_SIZE)
    {
        throw CurlOperationFailed(
                ErrorCode_Failure,
                __FILE__,
                __LINE__,
                CURLE_FAILED_INIT,
                "Invalid CURL error message buffer."
        );
    }

    // Set up error message buffer
    // According to the doc: https://curl.se/libcurl/c/CURLOPT_ERRORBUFFER.html
    // A successful call of setting `CURLOPT_ERRORBUFFER` will initialize the buffer to an empty
    // string since 7.60.0. The minimum version we require is 7.68.0. Therefore, we don't need to
    // manually clean up the passed-in buffer.
    m_easy_handle.set_option(CURLOPT_ERRORBUFFER, error_msg_buf.data());

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
    if (0 != offset) {
        std::string const range{"Range: bytes=" + std::to_string(offset) + "-"};
        m_http_headers.append(range);
    }
    if (disable_caching) {
        m_http_headers.append("Cache-Control: no-cache");
        m_http_headers.append("Pragma: no-cache");
    }
    if (false == m_http_headers.is_empty()) {
        m_easy_handle.set_option(CURLOPT_HTTPHEADER, m_http_headers.get_raw_list());
    }

    // Set up failure on error
    m_easy_handle.set_option(CURLOPT_FAILONERROR, static_cast<long>(true));
}
}  // namespace clp
