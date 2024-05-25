#ifndef CLP_CURLOPERATIONFAILED_HPP
#define CLP_CURLOPERATIONFAILED_HPP

#include <string>
#include <utility>

#include <curl/curl.h>

#include "ErrorCode.hpp"
#include "TraceableException.hpp"

namespace clp {
/**
 * The exception thrown by a failed libcurl operation.
 */
class CurlOperationFailed : public TraceableException {
public:
    CurlOperationFailed(
            ErrorCode error_code,
            char const* const filename,
            int line_number,
            CURLcode err,
            std::string msg
    )
            : TraceableException{error_code, filename, line_number},
              m_curl_err{err},
              m_msg{std::move(msg)} {}

    [[nodiscard]] auto get_curl_err() const -> CURLcode { return m_curl_err; }

    [[nodiscard]] auto what() const noexcept -> char const* override { return m_msg.c_str(); }

private:
    CURLcode m_curl_err;
    std::string m_msg;
};
}  // namespace clp

#endif
