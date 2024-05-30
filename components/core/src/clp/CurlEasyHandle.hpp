#ifndef CLP_CURLEASYHANDLE_HPP
#define CLP_CURLEASYHANDLE_HPP

#include <curl/curl.h>

#include "CurlOperationFailed.hpp"
#include "ErrorCode.hpp"

namespace clp {
/**
 * A C++ wrapper for libcurl's easy handle.
 */
class CurlEasyHandle {
public:
    // Constants
    static constexpr long cCurlOptionEnable{1};
    static constexpr long cCurlOptionDisable{0};

    // Constructors
    explicit CurlEasyHandle() : m_handle{curl_easy_init()} {
        if (nullptr == m_handle) {
            throw CurlOperationFailed(
                    ErrorCode_Failure,
                    __FILE__,
                    __LINE__,
                    CURLE_FAILED_INIT,
                    "`curl_easy_init` failed."
            );
        }
        set_option(CURLOPT_FAILONERROR, cCurlOptionEnable);
    }

    // Disable copy/move constructors/assignment operators
    CurlEasyHandle(CurlEasyHandle const&) = delete;
    CurlEasyHandle(CurlEasyHandle&&) = delete;
    auto operator=(CurlEasyHandle const&) -> CurlEasyHandle& = delete;
    auto operator=(CurlEasyHandle&&) -> CurlEasyHandle& = delete;

    // Destructor
    ~CurlEasyHandle() { curl_easy_cleanup(m_handle); }

    // Methods
    /**
     * Starts performing data transfer and only returns when the transfer completes or fails.
     * @return Same as `curl_easy_perform`.
     */
    [[nodiscard]] auto perform() -> CURLcode { return curl_easy_perform(m_handle); }

    /**
     * Sets the given CURL option for this handle.
     * @tparam ValueType
     * @param option
     * @param value
     * @throw CurlOperationFailed if an error occurs.
     */
    template <typename ValueType>
    auto set_option(CURLoption option, ValueType value) -> void {
        if (auto const err{curl_easy_setopt(m_handle, option, value)}; CURLE_OK != err) {
            throw CurlOperationFailed(
                    ErrorCode_Failure,
                    __FILE__,
                    __LINE__,
                    err,
                    "`curl_easy_setopt` failed."
            );
        }
    }

private:
    CURL* m_handle{nullptr};
};
}  // namespace clp

#endif
