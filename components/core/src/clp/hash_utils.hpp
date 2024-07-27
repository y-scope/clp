#ifndef CLP_HASH_UTILS_HPP
#define CLP_HASH_UTILS_HPP

#include <cstddef>
#include <span>
#include <string>
#include <vector>

#include <openssl/evp.h>

#include "ErrorCode.hpp"

namespace clp {
/**
 * @param input
 * @return `input` as a hex string (without the "0x" prefix).
 */
[[nodiscard]] auto convert_to_hex_string(std::span<unsigned char> input) -> std::string;

/**
 * Gets the HMAC-SHA256 hash of `input` with `key`.
 * @param input
 * @param key
 * @param hash Returns the HMAC.
 * @return ErrorCode_Success on success.
 * @return ErrorCode_BadParam if `key` is longer than `INT32_MAX`.
 * @return ErrorCode_Failure if hash generation fails.
 */
[[nodiscard]] auto get_hmac_sha256_hash(
        std::span<unsigned char const> input,
        std::span<unsigned char const> key,
        std::vector<unsigned char>& hash
) -> ErrorCode;

/**
 * Gets the SHA256 hash of `input`.
 * @param input
 * @param hash Returns the hash.
 * @return ErrorCode_Success on success.
 * @return Same as `digest_final` and `digest_update` on failure.
 * @throw EvpDigestContext::OperationFailed if `EvpDigestContext` cannot be initialized.
 */
[[nodiscard]] auto get_sha256_hash(
        std::span<unsigned char const> input,
        std::vector<unsigned char>& hash
) -> ErrorCode;
}  // namespace clp
#endif  // CLP_HASH_UTILS_HPP
