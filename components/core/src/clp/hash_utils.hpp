#ifndef CLP_HASH_UTILS_HPP
#define CLP_HASH_UTILS_HPP

#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <openssl/hmac.h>

#include "ErrorCode.hpp"
#include "TraceableException.hpp"

namespace clp {
/**
 * A class the wraps around openssl EVP_MD_CTX to manage the life cycle of
 * dynamically allocated EVP_MD_CTX object.
 */
class EvpCtxManager {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}

        // Methods
        [[nodiscard]] auto what() const noexcept -> char const* override {
            return "EvpCtxManager operation failed";
        }
    };

    // Constructors
    EvpCtxManager() : m_md_ctx{EVP_MD_CTX_new()} {
        if (nullptr == m_md_ctx) {
            throw OperationFailed(ErrorCode_NotInit, __FILENAME__, __LINE__);
        }
    }

    // Disable copy and move constructor/assignment
    EvpCtxManager(EvpCtxManager const&) = delete;
    auto operator=(EvpCtxManager const&) -> EvpCtxManager& = delete;

    // Destructor
    ~EvpCtxManager() { EVP_MD_CTX_free(m_md_ctx); }

    auto digest_init_ex(const EVP_MD* type, ENGINE* impl) -> int {
        return EVP_DigestInit_ex(m_md_ctx, type, impl);
    }

    auto digest_update(void const* d, size_t cnt) -> int {
        return EVP_DigestUpdate(m_md_ctx, d, cnt);
    }

    auto digest_final_ex(unsigned char* md, unsigned int* s) -> int {
        return EVP_DigestFinal_ex(m_md_ctx, md, s);
    }

private:
    EVP_MD_CTX* m_md_ctx{nullptr};
};

/**
 * Converts input hash into a hex string
 * @param input
 * @return input hash as a hex string
 */
auto convert_hash_to_hex_string(std::span<unsigned char> input) -> std::string;

/**
 * Gets the HMAC-SHA256 hash of input with key
 * @param input
 * @param key
 * @param hash Returns the result hash by reference.
 * @return ErrorCode_BadParam if input key exceeds maximum length.
 * ErrorCode_Failure if hash generation fails
 * ErrorCode_Success on success
 */
auto get_hmac_sha256_hash(
        std::span<unsigned char const> input,
        std::span<unsigned char const> key,
        std::vector<unsigned char>& hash
) -> ErrorCode;

/**
 * Gets the SHA256 hash of input
 * @param input
 * @param hash Returns the result hash by reference.
 * @return ErrorCode_BadParam if input key exceeds maximum length.
 * ErrorCode_Failure if hash generation fails
 * ErrorCode_Success on success
 * @throw EvpCtxManager::OperationFailed if EvpCtxManager can not be initalized
 */
auto get_sha256_hash(std::string_view input, std::vector<unsigned char>& hash) -> ErrorCode;
}  // namespace clp
#endif  // CLP_HASH_UTILS_HPP
