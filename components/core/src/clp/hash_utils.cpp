#include "hash_utils.hpp"

#include <span>
#include <string>
#include <vector>

#include <fmt/format.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>

#include "ErrorCode.hpp"
#include "spdlog_with_specializations.hpp"

using std::span;
using std::string;
using std::vector;

namespace clp {
auto EvpDigestContext::digest_update(std::span<unsigned char const> input) -> ErrorCode {
    if (m_is_digest_finalized) {
        return ErrorCode_Unsupported;
    }
    if (1 != EVP_DigestUpdate(m_md_ctx, input.data(), input.size())) {
        return ErrorCode_Failure;
    }
    return ErrorCode_Success;
}

auto EvpDigestContext::digest_final_ex(std::vector<unsigned char>& hash) -> ErrorCode {
    if (m_is_digest_finalized) {
        return ErrorCode_Unsupported;
    }

    hash.resize(SHA256_DIGEST_LENGTH);
    unsigned int length{};
    if (1 != EVP_DigestFinal_ex(m_md_ctx, hash.data(), &length)) {
        return ErrorCode_Failure;
    }
    if (SHA256_DIGEST_LENGTH != length) {
        return ErrorCode_Corrupt;
    }
    m_is_digest_finalized = true;
    return ErrorCode_Success;
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

auto get_sha256_hash(span<unsigned char const> input, vector<unsigned char>& hash) -> ErrorCode {
    EvpDigestContext evp_ctx_manager{EVP_sha256()};

    if (auto const error_code = evp_ctx_manager.digest_update(input);
        ErrorCode_Success != error_code)
    {
        SPDLOG_ERROR("Failed to digest input");
        return error_code;
    }

    if (auto const error_code = evp_ctx_manager.digest_final_ex(hash);
        ErrorCode_Success != error_code)
    {
        SPDLOG_ERROR("Failed to Finalize digest");
        return error_code;
    }

    return ErrorCode_Success;
}
}  // namespace clp
