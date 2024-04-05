#ifndef CLP_AWS_HASH_UTILS_HPP
#define CLP_AWS_HASH_UTILS_HPP

#include <openssl/hmac.h>
#include <openssl/sha.h>

#include <string>

#include <fmt/format.h>

namespace clp::aws {
/**
 * Converts a char array to a string
 * @param a
 * @param size
 * @return The converted string
 */
inline std::string char_array_to_string(unsigned char const* a, size_t size) {
    std::string hex_string;
    for (size_t i = 0; i < size; i++) {
        hex_string += fmt::format("{:02x}", static_cast<int>(a[i]));
    }
    return hex_string;
}

/**
 * Gets the HMAC-SHA256 hash
 * @param key
 * @param value
 * @param hex_output
 * @return The HMAC SHA256 hash
 */
inline std::string
get_hmac_sha256_hash(std::string const& key, std::string const& value, bool hex_output = false) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    HMAC_CTX* hmac = HMAC_CTX_new();
    HMAC_Init_ex(hmac, key.c_str(), key.size(), EVP_sha256(), nullptr);
    HMAC_Update(hmac, reinterpret_cast<unsigned char const*>(value.c_str()), value.size());
    unsigned int len = SHA256_DIGEST_LENGTH;
    HMAC_Final(hmac, hash, &len);
    HMAC_CTX_free(hmac);

    if (hex_output) {
        return char_array_to_string(hash, SHA256_DIGEST_LENGTH);
    } else {
        std::string result;
        result.assign(reinterpret_cast<char*>(hash), SHA256_DIGEST_LENGTH);
        return result;
    }
}

/**
 * Gets the SHA256 hash
 * @param input
 * @return The SHA256 hash
 */
inline std::string get_sha256_hash(std::string const& input) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, input.c_str(), input.size());
    SHA256_Final(hash, &sha256);

    return char_array_to_string(hash, SHA256_DIGEST_LENGTH);
}

/**
 * Initializes SHA256 hash
 * @param sha256
 */
inline void init_sha256_hash(SHA256_CTX* sha256) {
    SHA256_Init(sha256);
}

/**
 * Updates SHA256 hash
 * @param sha256
 * @param input
 * @param length
 */
inline void update_sha256_hash(SHA256_CTX* sha256, void* input, size_t length) {
    SHA256_Update(sha256, input, length);
}

/**
 * Finalizes SHA256 hash
 * @param sha256
 * @return The SHA256 hash
 */
inline std::string finalize_sha256_hash(SHA256_CTX* sha256) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_Final(hash, sha256);

    return char_array_to_string(hash, SHA256_DIGEST_LENGTH);
}

}  // namespace clp::aws
#endif  // CLP_AWS_HASH_UTILS_HPP
