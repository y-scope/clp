#ifndef CLP_CURLDOWNLOADHANDLER_HPP
#define CLP_CURLDOWNLOADHANDLER_HPP

#include <cstddef>

#include <curl/curl.h>

#include "CurlOperationFailed.hpp"
#include "CurlStringList.hpp"
#include "ErrorCode.hpp"

namespace clp {
/**
 * A class to download data from a URL using libcurl. In particular, this class manages the
 * lifecycle of the libcurl handle and any arguments that libcurl uses during the download.
 */
class CurlDownloadHandler {
public:
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

    // Constructor
    /**
     * @param src_url
     * @param arg Argument to pass to `progress_callback` and `write_callback`
     * @param progress_callback
     * @param write_callback
     * @param connection_timeout Maximum time that the connection phase may take.
     * Doc: https://curl.se/libcurl/c/CURLOPT_CONNECTTIMEOUT.html
     * @param overall_timeout Maximum time that the transfer may take. Note that this includes
     * `connection_timeout`. Doc: https://curl.se/libcurl/c/CURLOPT_TIMEOUT.html
     * @param offset Index of the byte at which to start the download
     * @param disable_caching Whether to disable caching
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
     * Starts downloading the data and only returns when the download completes or fails.
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
                    "`curl_easy_setopt` failed."
            );
        }
    }

    CurlStringList m_http_headers;
    CURL* m_handler{nullptr};
};
}  // namespace clp

#endif
