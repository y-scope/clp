#ifndef CLP_AWS_HASH_UTILS_HPP
#define CLP_AWS_HASH_UTILS_HPP

#include <string>

#include <fmt/format.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>

#include "../type_utils.hpp"

using clp::size_checked_pointer_cast;

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
 * @param hex_output Whether to output the hash as a hex string
 * @return The HMAC SHA256 hash
 */
inline std::string
get_hmac_sha256_hash(std::string const& key, std::string const& input, bool hex_output = false) {

    std::array<unsigned char, SHA256_DIGEST_LENGTH> hash{};
    unsigned int hash_length {0};
    int key_length{0};
    if(key.size() > INT32_MAX) {

    }
    auto* res = HMAC(
         EVP_sha256(),
         key.c_str(),
         key.size(),
         size_checked_pointer_cast<const unsigned char>(input.c_str()),
         input.size(),
         hash.data(),
         &hash_length
    );

    std::string result;
    result.assign(size_checked_pointer_cast<char>(hash.data()), SHA256_DIGEST_LENGTH);

    if (hex_output) {
        return char_array_to_string(hash.data(), SHA256_DIGEST_LENGTH);
    }

    return result;
}

/**
 * Gets the SHA256 hash
 * @param input
 * @return The SHA256 hash
 */
inline std::string get_sha256_hash(std::string const& input) {

    EVP_MD_CTX * mdctx = EVP_MD_CTX_new();

    EVP_DigestInit_ex(mdctx, EVP_sha256(), nullptr);

    EVP_DigestUpdate(mdctx, input.c_str(), input.size());

    std::array<unsigned char, SHA256_DIGEST_LENGTH> hash{};
    unsigned int digest_len{0};
    EVP_DigestFinal_ex(mdctx, hash.data(), &digest_len);

    EVP_MD_CTX_free(mdctx);

    return char_array_to_string(hash.data(), digest_len);
}

}  // namespace clp::aws
#endif  // CLP_AWS_HASH_UTILS_HPP
