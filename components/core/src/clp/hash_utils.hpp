#ifndef CLP_HASH_UTILS_HPP
#define CLP_HASH_UTILS_HPP

#include <cstddef>
#include <span>
#include <string>
#include <vector>

#include <openssl/evp.h>

#include "ErrorCode.hpp"
#include "TraceableException.hpp"

namespace clp {
/**
 * A C++ wrapper for openssl's EVP digest message digest context (EVP_MD_CTX).
 */
class EvpDigestContext {
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
    EvpDigestContext(const EVP_MD* type, ENGINE* impl) : m_md_ctx{EVP_MD_CTX_create()} {
        if (nullptr == m_md_ctx) {
            throw OperationFailed(ErrorCode_NotInit, __FILENAME__, __LINE__);
        }
        if (1 != EVP_DigestInit_ex(m_md_ctx, type, impl)) {
            throw OperationFailed(ErrorCode_Failure, __FILENAME__, __LINE__);
        }
    }

    // Disable copy constructor/assignment operator
    EvpDigestContext(EvpDigestContext const&) = delete;
    auto operator=(EvpDigestContext const&) -> EvpDigestContext& = delete;

    // disable move constructor/assignment operator
    EvpDigestContext(EvpDigestContext&&) = delete;
    auto operator=(EvpDigestContext&&) -> EvpDigestContext& = delete;

    // Destructor
    ~EvpDigestContext() { EVP_MD_CTX_destroy(m_md_ctx); }

    // Methods
    [[nodiscard]] auto digest_update(std::span<unsigned char const> input) -> bool;

    [[nodiscard]] auto
    digest_final_ex(std::vector<unsigned char>& hash, unsigned int& length) -> bool;

private:
    EVP_MD_CTX* m_md_ctx{nullptr};
    bool m_is_digest_finalized{false};
};

/**
 * Converts input hash into a hex string.
 * @param input
 * @return input hash as a hex string
 */
[[nodiscard]] auto convert_hash_to_hex_string(std::span<unsigned char> input) -> std::string;

/**
 * Gets the HMAC-SHA256 hash of input with key.
 * @param input
 * @param key
 * @param hash Returns the hashing result.
 * @return ErrorCode_Success on success.
 * @return ErrorCode_BadParam if input key exceeds maximum length.
 * @return ErrorCode_Failure if hash generation fails.
 */
[[nodiscard]] auto get_hmac_sha256_hash(
        std::span<unsigned char const> input,
        std::span<unsigned char const> key,
        std::vector<unsigned char>& hash
) -> ErrorCode;

/**
 * Gets the SHA256 hash of input
 * @param input
 * @param hash Returns the hashing result.
 * @return ErrorCode_Success on success.
 * @return ErrorCode_BadParam if input key exceeds maximum length.
 * @return ErrorCode_Failure if hash generation fails.
 * @throw EvpCtxManager::OperationFailed if EvpCtxManager can not be initalized
 */
[[nodiscard]] auto get_sha256_hash(
        std::span<unsigned char const> input,
        std::vector<unsigned char>& hash
) -> ErrorCode;
}  // namespace clp
#endif  // CLP_HASH_UTILS_HPP
