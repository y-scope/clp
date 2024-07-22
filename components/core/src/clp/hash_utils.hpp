#ifndef CLP_AWS_HASH_UTILS_HPP
#define CLP_AWS_HASH_UTILS_HPP

#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "ErrorCode.hpp"

namespace clp::aws {
/**
 * Converts a char array to a string
 * @param a
 * @param size
 * @return The converted string
 */
auto convert_hash_to_string(std::span<unsigned char> input) -> std::string;

/**
 * Gets the HMAC-SHA256 hash
 * @param key
 * @param value
 * @param hex_output Whether to output the hash as a hex string
 * @return The HMAC SHA256 hash
 */
auto get_hmac_sha256_hash(
        std::span<unsigned char const> key,
        std::span<unsigned char const> input,
        std::vector<unsigned char>& hash
) -> ErrorCode;

/**
 * Gets the SHA256 hash
 * @param input
 * @return The SHA256 hash
 */
auto get_sha256_hash(std::string_view, std::vector<unsigned char>& hash) -> ErrorCode;
}  // namespace clp::aws
#endif  // CLP_AWS_HASH_UTILS_HPP
