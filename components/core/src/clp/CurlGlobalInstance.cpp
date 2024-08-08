#include "CurlGlobalInstance.hpp"

#include <mutex>

#include <curl/curl.h>

#include "CurlOperationFailed.hpp"
#include "ErrorCode.hpp"

namespace clp {
CurlGlobalInstance::CurlGlobalInstance() {
    std::scoped_lock const global_lock{m_ref_count_mutex};
    if (0 == m_ref_count) {
        if (auto const err{curl_global_init(CURL_GLOBAL_ALL)}; 0 != err) {
            throw CurlOperationFailed(
                    ErrorCode_Failure,
                    __FILE__,
                    __LINE__,
                    err,
                    "`curl_global_init` failed."
            );
        }
    }
    ++m_ref_count;
}

CurlGlobalInstance::~CurlGlobalInstance() {
    std::scoped_lock const global_lock{m_ref_count_mutex};
    --m_ref_count;
    if (0 == m_ref_count) {
#if defined(__APPLE__)
        // NOTE: On macOS, calling `curl_global_init` after `curl_global_cleanup` will fail with
        // CURLE_SSL_CONNECT_ERROR. Thus, for now, we skip `deinit` on macOS. Luckily, it is safe to
        // call `curl_global_init` multiple times without calling `curl_global_cleanup`. Related
        // issues:
        // - https://github.com/curl/curl/issues/12525
        // - https://github.com/curl/curl/issues/13805
        // TODO: Remove this conditional logic when the issues are resolved.
        return;
#else
        curl_global_cleanup();
#endif
    }
}
}  // namespace clp
