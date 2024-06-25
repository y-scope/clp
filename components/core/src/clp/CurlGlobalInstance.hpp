#ifndef CLP_CURLGLOBALINSTANCE_HPP
#define CLP_CURLGLOBALINSTANCE_HPP

#include <cstddef>
#include <mutex>

namespace clp {
/**
 * Class to wrap `libcurl`'s global initialization/de-initialization calls. Before using any
 * `libcurl` functionalities, an instance of this function must be created to ensure underlying
 * `libcurl` resources have been initialized. This class maintains a static reference count to all
 * the living instances. De-initialization happens when the reference count reaches 0.
 */
class CurlGlobalInstance {
public:
    // Constructors
    CurlGlobalInstance();

    // Disable copy/move constructors/assignment operators
    CurlGlobalInstance(CurlGlobalInstance const&) = delete;
    CurlGlobalInstance(CurlGlobalInstance&&) = delete;
    auto operator=(CurlGlobalInstance const&) -> CurlGlobalInstance& = delete;
    auto operator=(CurlGlobalInstance&&) -> CurlGlobalInstance& = delete;

    // Destructor
    ~CurlGlobalInstance();

private:
    static std::mutex m_global_mutex;
    static size_t m_num_living_instances;
};
}  // namespace clp

#endif  // CLP_CURLGLOBALINSTANCE_HPP
