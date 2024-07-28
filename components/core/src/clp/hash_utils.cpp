#include "hash_utils.hpp"

#include <memory>
#include <span>
#include <string>
#include <vector>

#include <fmt/format.h>
#include <openssl/err.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>

#include "ErrorCode.hpp"
#include "TraceableException.hpp"

using std::make_unique;
using std::span;
using std::string;
using std::unique_ptr;
using std::vector;

namespace clp {
namespace {
/**
 * Gets the OpenSSL error string as a c++ string
 * @return The string representing the OpenSSL error
 */
auto get_openssl_error_string() -> string {
    auto const openssl_err = ERR_get_error();
    return {ERR_error_string(openssl_err, nullptr)};
}

/**
 * A C++ wrapper for OpenSSL's EVP message digest context (EVP_MD_CTX).
 */
class EvpDigestContext {
public:
    // Types
    class OperationFailed : public clp::TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : OperationFailed(
                          error_code,
                          filename,
                          line_number,
                          "EvpDigestContext operation failed"
                  ) {}

        OperationFailed(
                ErrorCode error_code,
                char const* const filename,
                int line_number,
                std::string message
        )
                : TraceableException(error_code, filename, line_number),
                  m_message(std::move(message)) {}

        // Methods
        [[nodiscard]] auto what() const noexcept -> char const* override {
            return m_message.c_str();
        }

    private:
        string m_message;
    };

    // Constructors
    /**
     * @param type The type of digest (hash algorithm).
     * @throw EvpDigestContext::OperationFailed with ErrorCode_NoMem if
     * `EVP_MD_CTX_create` fails.
     * @throw EvpDigestContext::OperationFailed with ErrorCode_Failure if
     * `EVP_DigestInit_ex` fails.
     */
    EvpDigestContext(EVP_MD const* type)
            : m_md_ctx{EVP_MD_CTX_create()},
              m_digest_nid{EVP_MD_type(type)} {
        if (nullptr == m_md_ctx) {
            throw OperationFailed(ErrorCode_NoMem, __FILENAME__, __LINE__);
        }
        // Set impl to nullptr to use the default implementation of digest type
        if (1 != EVP_DigestInit_ex(m_md_ctx, type, nullptr)) {
            throw OperationFailed(
                    ErrorCode_Failure,
                    __FILENAME__,
                    __LINE__,
                    get_openssl_error_string()
            );
        }
    }

    // Disable copy constructor and assignment operator
    EvpDigestContext(EvpDigestContext const&) = delete;
    auto operator=(EvpDigestContext const&) -> EvpDigestContext& = delete;

    // Disable move constructor and assignment operator
    EvpDigestContext(EvpDigestContext&&) = delete;
    auto operator=(EvpDigestContext&&) -> EvpDigestContext& = delete;

    // Destructor
    ~EvpDigestContext() { EVP_MD_CTX_destroy(m_md_ctx); }

    // Methods
    /**
     * Hashes `input` into the digest.
     * @param input
     * @return ErrorCode_Success on success.
     * @return ErrorCode_Failure if `EVP_DigestUpdate` fails.
     */
    [[nodiscard]] auto digest_update(std::span<unsigned char const> input) -> ErrorCode;

    /**
     * Writes the digest into `hash` and clears the digest.
     * @param hash Returns the hashing result.
     * @return ErrorCode_Success on success.
     * @return ErrorCode_Corrupt if the hashing result has an unexpected length.
     * @return ErrorCode_Failure if `EVP_DigestFinal_ex` fails.
     * @throw EvpDigestContext::OperationFailed with ErrorCode_Failure if
     * `EVP_DigestInit_ex` fails.
     */
    [[nodiscard]] auto digest_final(std::vector<unsigned char>& hash) -> ErrorCode;

private:
    EVP_MD_CTX* m_md_ctx{nullptr};
    int m_digest_nid{};
};

auto EvpDigestContext::digest_update(span<unsigned char const> input) -> ErrorCode {
    if (1 != EVP_DigestUpdate(m_md_ctx, input.data(), input.size())) {
        return ErrorCode_Failure;
    }
    return ErrorCode_Success;
}

auto EvpDigestContext::digest_final(std::vector<unsigned char>& hash) -> ErrorCode {
    hash.resize(EVP_MD_CTX_size(m_md_ctx));
    unsigned int length{};
    if (1 != EVP_DigestFinal_ex(m_md_ctx, hash.data(), &length)) {
        return ErrorCode_Failure;
    }
    if (hash.size() != length) {
        return ErrorCode_Corrupt;
    }

    if (1 != EVP_DigestInit_ex(m_md_ctx, EVP_get_digestbynid(m_digest_nid), nullptr)) {
        throw OperationFailed(
                ErrorCode_Failure,
                __FILENAME__,
                __LINE__,
                get_openssl_error_string()
        );
    }
    return ErrorCode_Success;
}
}  // namespace

auto convert_to_hex_string(std::span<unsigned char> input) -> string {
    string hex_string;
    for (auto const c : input) {
        hex_string += fmt::format("{:02x}", c);
    }
    return hex_string;
}

auto get_hmac_sha256_hash(
        span<unsigned char const> input,
        span<unsigned char const> key,
        vector<unsigned char>& hash
) -> ErrorCode {
    if (key.size() > INT32_MAX) {
        return ErrorCode_BadParam;
    }

    hash.resize(SHA256_DIGEST_LENGTH);
    unsigned int hash_length{0};
    auto const key_length{static_cast<int>(key.size())};
    if (nullptr
        == HMAC(EVP_sha256(),
                key.data(),
                key_length,
                input.data(),
                input.size(),
                hash.data(),
                &hash_length))
    {
        return ErrorCode_Failure;
    }

    if (hash.size() != hash_length) {
        return ErrorCode_Corrupt;
    }

    return ErrorCode_Success;
}

auto get_sha256_hash(span<unsigned char const> input, vector<unsigned char>& hash) -> ErrorCode {
    unique_ptr<EvpDigestContext> evp_ctx_manager;
    try {
        evp_ctx_manager = make_unique<EvpDigestContext>(EVP_sha256());
    } catch (EvpDigestContext::OperationFailed const& err) {
        return err.get_error_code();
    }
    if (auto const error_code = evp_ctx_manager->digest_update(input);
        ErrorCode_Success != error_code)
    {
        return error_code;
    }

    if (auto const error_code = evp_ctx_manager->digest_final(hash);
        ErrorCode_Success != error_code)
    {
        return error_code;
    }

    return ErrorCode_Success;
}
}  // namespace clp
