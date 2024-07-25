#ifndef CLP_CURLGLOBALINSTANCE_HPP
#define CLP_CURLGLOBALINSTANCE_HPP

#include <cstddef>
#include <mutex>

namespace clp {
/**
 * Class to wrap `libcurl`'s global initialization/de-initialization calls using RAII. Before using
 * any `libcurl` functionalities, an instance of this class must be created. Although unnecessasry,
 * it can be safely instantiated multiple times; it maintains a static reference count to all
 * existing instances and only de-initializes `libcurl`'s global resources when the reference count
 * reaches 0.
 */
class CurlGlobalInstance {
public:
    // Constructors
    CurlGlobalInstance();

    // Disable copy/move constructors and assignment operators
    CurlGlobalInstance(CurlGlobalInstance const&) = delete;
    CurlGlobalInstance(CurlGlobalInstance&&) = delete;
    auto operator=(CurlGlobalInstance const&) -> CurlGlobalInstance& = delete;
    auto operator=(CurlGlobalInstance&&) -> CurlGlobalInstance& = delete;

    // Destructor
    ~CurlGlobalInstance();

private:
    static inline std::mutex m_ref_count_mutex;
    static inline size_t m_ref_count{0};
};
}  // namespace clp

#endif  // CLP_CURLGLOBALINSTANCE_HPP
