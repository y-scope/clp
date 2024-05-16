#ifndef CLP_CURLDOWNLOADHANDLER_HPP
#define CLP_CURLDOWNLOADHANDLER_HPP

#include <cstddef>
#include <memory>

#include <curl/curl.h>

#include "CurlOperationFailed.hpp"
#include "CurlStringList.hpp"
#include "ErrorCode.hpp"

namespace clp {
/**
 * This class wraps the C implementation of the CURL handler to perform data downloading. It
 * provides a cleaner interface to manage the life cycle of the object with proper error handling.
 */
class CurlDownloadHandler {
public:
    using ProgressCallback = int (*)(void*, curl_off_t, curl_off_t, curl_off_t, curl_off_t);
    using WriteCallback = size_t (*)(char*, size_t, size_t, void*);

    // Constructor
    /**
     * Constructs a CURL download handler with the given parameters.
     * @param src_url
     * @param data Client specified data passing to `progress_callback` and `write_callback`
     * @param progress_callback CURL progress callback. The function must have C linkage. Doc:
     * Doc: https://curl.se/libcurl/c/CURLOPT_XFERINFOFUNCTION.html
     * @param write_callback CURL write callback. The function must have C linkage.
     * Doc: https://curl.se/libcurl/c/CURLOPT_WRITEFUNCTION.html
     * @param connection_timeout CURL connection timeout in seconds.
     * Doc: https://curl.se/libcurl/c/CURLOPT_CONNECTTIMEOUT.html
     * @param overall_timeout CURL overall timeout in seconds. Notice that `connection_timeout` is
     * included in the overall timeout.
     * Doc: https://curl.se/libcurl/c/CURLOPT_TIMEOUT.html
     * @param offset The offset of bytes to start downloading.
     * @param disable_caching whether to disable caching.
     */
    explicit CurlDownloadHandler(
            std::string_view src_url,
            void* data,
            ProgressCallback progress_callback,
            WriteCallback write_callback,
            long connection_timeout,
            long overall_timeout,
            size_t offset = 0,
            bool disable_caching = false
    );

    // Disable copy/move constructors/assignment operators
    CurlDownloadHandler(CurlDownloadHandler const&) = delete;
    CurlDownloadHandler(CurlDownloadHandler&&) = delete;
    auto operator=(CurlDownloadHandler const&) -> CurlDownloadHandler& = delete;
    auto operator=(CurlDownloadHandler&&) -> CurlDownloadHandler& = delete;

    // Destructor
    ~CurlDownloadHandler() { curl_easy_cleanup(m_handler); }

    // Methods
    /**
     * Starts downloading the data. This function returns when the download completes or fails.
     * This is a wrapper of `curl_easy_perform`.
     * @return Same as `curl_easy_perform`.
     */
    [[nodiscard]] auto perform() -> CURLcode { return curl_easy_perform(m_handler); }

private:
    /**
     * Sets the given CURL option for this handler.
     * @tparam ValueType
     * @param option
     * @param value
     * @throw CurlOperationFailed if an error occurs.
     */
    template <typename ValueType>
    auto set_option(CURLoption option, ValueType value) -> void {
        if (auto const err{curl_easy_setopt(m_handler, option, value)}; CURLE_OK != err) {
            throw CurlOperationFailed(
                    ErrorCode_Failure,
                    __FILE__,
                    __LINE__,
                    err,
                    "Failed to call `curl_easy_setopt`."
            );
        }
    }

    CurlStringList m_http_headers;
    CURL* m_handler{nullptr};
};

}  // namespace clp

#endif
