#ifndef CLP_AWS_HASH_UTILS_HPP
#define CLP_AWS_HASH_UTILS_HPP

#include "../ErrorCode.hpp"

#include <vector>
#include <span>
#include <string>
#include <string_view>

namespace clp::aws {
/**
 * Converts a char array to a string
 * @param a
 * @param size
 * @return The converted string
 */
std::string char_array_to_string(std::span<unsigned char> input);

/**
 * Gets the HMAC-SHA256 hash
 * @param key
 * @param value
 * @param hex_output Whether to output the hash as a hex string
 * @return The HMAC SHA256 hash
 */
ErrorCode get_hmac_sha256_hash(std::span<unsigned char const> key, std::span<unsigned char const> input, std::vector<unsigned char>& hash);

/**
 * Gets the SHA256 hash
 * @param input
 * @return The SHA256 hash
 */
ErrorCode get_sha256_hash(std::string_view, std::vector<unsigned char>& hash);
}  // namespace clp::aws
#endif  // CLP_AWS_HASH_UTILS_HPP
