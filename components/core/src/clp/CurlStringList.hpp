#ifndef CLP_CURLSTRINGLIST_HPP
#define CLP_CURLSTRINGLIST_HPP

#include <cstddef>
#include <string_view>

#include <curl/curl.h>

#include "CurlOperationFailed.hpp"
#include "ErrorCode.hpp"

namespace clp {
/**
 * A C++ wrapper for libcurl's string linked list (curl_slist).
 */
class CurlStringList {
public:
    // Constructors
    CurlStringList() = default;

    // Disable copy/move constructors/assignment operators
    CurlStringList(CurlStringList const&) = delete;
    CurlStringList(CurlStringList&&) = delete;
    auto operator=(CurlStringList const&) -> CurlStringList& = delete;
    auto operator=(CurlStringList&&) -> CurlStringList& = delete;

    // Destructor
    ~CurlStringList() { curl_slist_free_all(m_list); }

    // Methods
    /**
     * Appends a string to the end of the list.
     * @param str
     * @throw CurlOperationFailed if the append operation failed.
     */
    auto append(std::string_view str) -> void {
        auto* list_after_appending{curl_slist_append(m_list, str.data())};
        if (nullptr == list_after_appending) {
            throw CurlOperationFailed(
                    ErrorCode_Failure,
                    __FILE__,
                    __LINE__,
                    CURLE_OUT_OF_MEMORY,
                    "curl_slist_append failed."
            );
        }
        m_list = list_after_appending;
        ++m_size;
    }

    [[nodiscard]] auto get_raw_list() const -> struct curl_slist* { return m_list; }

    [[nodiscard]] auto get_size() const -> size_t { return m_size; }

    [[nodiscard]] auto is_empty() const -> bool { return 0 == get_size(); }

private:
    size_t m_size{0};
    struct curl_slist* m_list{nullptr};
};
}  // namespace clp

#endif
