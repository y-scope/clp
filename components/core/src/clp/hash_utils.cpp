#include "hash_utils.hpp"

#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <fmt/format.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>

#include "ErrorCode.hpp"
#include "spdlog_with_specializations.hpp"
#include "type_utils.hpp"

using clp::size_checked_pointer_cast;
using std::span;
using std::string;
using std::string_view;
using std::vector;

namespace clp {
auto EvpDigestContext::digest_update(std::span<unsigned char const> input) -> bool {
    if (m_is_digest_finalized) {
        throw OperationFailed(ErrorCode_Unsupported, __FILENAME__, __LINE__);
    }
    return static_cast<bool>(EVP_DigestUpdate(m_md_ctx, input.data(), input.size()));
}

auto EvpDigestContext::digest_final_ex(std::vector<unsigned char>& hash, unsigned int& length)
        -> bool {
    if (1 != EVP_DigestFinal_ex(m_md_ctx, hash.data(), &length)) {
        return false;
    }
    m_is_digest_finalized = true;
    return true;
}

auto convert_hash_to_hex_string(span<unsigned char> input) -> string {
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
    hash.resize(SHA256_DIGEST_LENGTH);
    unsigned int hash_length{0};

    if (key.size() > INT32_MAX) {
        SPDLOG_ERROR("Input key exceeds maximum length");
        return ErrorCode_BadParam;
    }
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
        SPDLOG_ERROR("Failed to get HMAC hashes");
        return ErrorCode_Failure;
    }

    if (hash_length != SHA256_DIGEST_LENGTH) {
        SPDLOG_ERROR("Unexpected hash length");
        return ErrorCode_Failure;
    }

    return ErrorCode_Success;
}

/**
 * Gets the SHA256 hash
 * @param input
 * @return The SHA256 hash
 */
auto get_sha256_hash(span<unsigned char const> input, vector<unsigned char>& hash) -> ErrorCode {
    EvpDigestContext evp_ctx_manager{EVP_sha256(), nullptr};

    if (false == evp_ctx_manager.digest_update(input)) {
        SPDLOG_ERROR("Failed to digest input");
        return ErrorCode_Failure;
    }

    unsigned int digest_len{0};
    hash.resize(SHA256_DIGEST_LENGTH);

    if (false == evp_ctx_manager.digest_final_ex(hash, digest_len)) {
        SPDLOG_ERROR("Failed to Finalize digest");
        return ErrorCode_Failure;
    }

    return ErrorCode_Success;
}
}  // namespace clp
