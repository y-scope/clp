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

namespace clp::aws {
auto convert_hash_to_string(span<unsigned char> input) -> string {
    string hex_string;
    for (auto const c : input) {
        hex_string += fmt::format("{:02x}", c);
    }
    return hex_string;
}

auto get_hmac_sha256_hash(
        span<unsigned char const> key,
        span<unsigned char const> input,
        vector<unsigned char>& hash
) -> ErrorCode {
    hash.resize(SHA256_DIGEST_LENGTH);
    unsigned int hash_length{0};

    if (key.size() > INT32_MAX) {
        SPDLOG_ERROR("Key too long");
        return ErrorCode_BadParam;
    }
    int const key_length{static_cast<int>(key.size())};
    auto* res
            = HMAC(EVP_sha256(),
                   key.data(),
                   key_length,
                   input.data(),
                   input.size(),
                   hash.data(),
                   &hash_length);

    if (nullptr == res) {
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
auto get_sha256_hash(string_view input, vector<unsigned char>& hash) -> ErrorCode {
    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();

    EVP_DigestInit_ex(mdctx, EVP_sha256(), nullptr);

    EVP_DigestUpdate(mdctx, input.data(), input.size());

    unsigned int digest_len{0};
    hash.resize(SHA256_DIGEST_LENGTH);
    EVP_DigestFinal_ex(mdctx, hash.data(), &digest_len);
    EVP_MD_CTX_free(mdctx);

    return ErrorCode_Success;
}
}  // namespace clp::aws
