#include "CurlDownloadHandler.hpp"

namespace clp {
CurlDownloadHandler::CurlDownloadHandler(
        std::string_view src_url,
        void* data,
        ProgressCallback progress_callback,
        WriteCallback write_callback,
        long connection_timeout,
        long overall_timeout,
        size_t offset,
        bool disable_caching
)
        : m_handler{curl_easy_init()} {
    if (nullptr == m_handler) {
        throw CurlOperationFailed(
                ErrorCode_Failure,
                __FILE__,
                __LINE__,
                CURLE_FAILED_INIT,
                "Failed to call `curl_easy_init`."
        );
    }

    // Set up src url
    set_option(CURLOPT_URL, src_url.data());

    // Set up progress callback
    set_option(CURLOPT_XFERINFOFUNCTION, progress_callback);
    set_option(CURLOPT_XFERINFODATA, data);
    set_option(CURLOPT_NOPROGRESS, 0);

    // Set up write callback
    set_option(CURLOPT_WRITEFUNCTION, write_callback);
    set_option(CURLOPT_WRITEDATA, data);

    // Set up timeout
    set_option(CURLOPT_CONNECTTIMEOUT, connection_timeout);
    set_option(CURLOPT_TIMEOUT, overall_timeout);

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
        set_option(CURLOPT_HTTPHEADER, m_http_headers.get_raw_list());
    }
}
}  // namespace clp
