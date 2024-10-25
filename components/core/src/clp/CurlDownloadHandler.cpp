#include "CurlDownloadHandler.hpp"

#include <chrono>
#include <cstddef>
#include <map>
#include <memory>
#include <regex>
#include <set>
#include <string>
#include <string_view>
#include <utility>

#include <curl/curl.h>

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
        std::map<std::string, std::string> const& custom_headers
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
    if (0 != offset) {
        std::string const range{"Range: bytes=" + std::to_string(offset) + "-"};
        m_http_headers.append(range);
    }
    if (disable_caching) {
        m_http_headers.append("Cache-Control: no-cache");
        m_http_headers.append("Pragma: no-cache");
    }
    const std::set<std::string> kReservedHeaders = {
        "range",
        "cache-control",
        "pragma"
    };
    for (const auto& [key, value] : custom_headers) {
        // Convert to lowercase for case-insensitive comparison
        std::string lower_key = key;
        std::transform(lower_key.begin(), lower_key.end(), lower_key.begin(),
            [](unsigned char c){ return std::tolower(c); });
        
        if (kReservedHeaders.end() == kReservedHeaders.find(lower_key)) {
            // Filter out illegal header names and header values by regex
            // Can contain alphanumeric characters (A-Z, a-z, 0-9), hyphens (`-`), and underscores
            // (`_`)
            std::regex header_name_pattern("^[A-Za-z0-9_-]+$");
            // Must consist of printable ASCII characters (values between 0x20 and 0x7E)
            std::regex header_value_pattern("^[\\x20-\\x7E]*$");
            if (std::regex_match(key, header_name_pattern) && 
                std::regex_match(value, header_value_pattern)) {
                m_http_headers.append(key + ": " + value);
            }
        }
    }
    if (false == m_http_headers.is_empty()) {
        m_easy_handle.set_option(CURLOPT_HTTPHEADER, m_http_headers.get_raw_list());
    }

    // Set up failure on HTTP error reponse
    m_easy_handle.set_option(CURLOPT_FAILONERROR, static_cast<long>(true));
}
}  // namespace clp
