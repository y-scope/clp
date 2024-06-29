#include "CurlGlobalInstance.hpp"

#include <mutex>

#include <curl/curl.h>

#include "CurlOperationFailed.hpp"
#include "ErrorCode.hpp"

namespace clp {
CurlGlobalInstance::CurlGlobalInstance() {
    std::lock_guard<std::mutex> const global_lock{m_global_mutex};
    if (0 == m_num_living_instances) {
        if (auto const err{curl_global_init(CURL_GLOBAL_ALL)}; CURLE_OK != err) {
            throw CurlOperationFailed(
                    ErrorCode_Failure,
                    __FILE__,
                    __LINE__,
                    err,
                    "`curl_global_init` failed."
            );
        }
    }
    ++m_num_living_instances;
}

CurlGlobalInstance::~CurlGlobalInstance() {
    std::lock_guard<std::mutex> const global_lock{m_global_mutex};
    --m_num_living_instances;
    if (0 == m_num_living_instances) {
#if defined(__APPLE__)
        // NOTE: On macOS, calling `curl_global_init` after `curl_global_cleanup` will fail with
        // CURLE_SSL_CONNECT_ERROR. Thus, for now, we skip `deinit` on macOS. Related issues:
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
