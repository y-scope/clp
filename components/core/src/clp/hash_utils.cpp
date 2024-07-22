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
auto convert_hash_to_hex_string(std::span<unsigned char> input) -> string {
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
    EvpCtxManager evp_ctx_manager{};

    if (1 != evp_ctx_manager.digest_init_ex(EVP_sha256(), nullptr)) {
        SPDLOG_ERROR("Failed to initialize ctx manager");
        return ErrorCode_Failure;
    }

    if (1 != evp_ctx_manager.digest_update(input.data(), input.size())) {
        SPDLOG_ERROR("Failed to digest input");
        return ErrorCode_Failure;
    }

    unsigned int digest_len{0};
    hash.resize(SHA256_DIGEST_LENGTH);

    if (1 != evp_ctx_manager.digest_final_ex(hash.data(), &digest_len)) {
        SPDLOG_ERROR("Failed to Finalize digest");
        return ErrorCode_Failure;
    }

    return ErrorCode_Success;
}
}  // namespace clp
