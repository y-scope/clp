#ifndef CLP_HASH_UTILS_HPP
#define CLP_HASH_UTILS_HPP

#include <cstddef>
#include <span>
#include <string>
#include <vector>

#include <openssl/evp.h>

#include "ErrorCode.hpp"
#include "TraceableException.hpp"

namespace clp {
// Types
class HashUtilsOperationFailed : public TraceableException {
public:
    // Constructors
    HashUtilsOperationFailed(ErrorCode error_code, char const* const filename, int line_number)
            : HashUtilsOperationFailed(
                      error_code,
                      filename,
                      line_number,
                      "HashUtils operation failed"
              ) {}

    HashUtilsOperationFailed(
            ErrorCode error_code,
            char const* const filename,
            int line_number,
            std::string message
    )
            : TraceableException(error_code, filename, line_number),
              m_message(std::move(message)) {}

    // Methods
    [[nodiscard]] auto what() const noexcept -> char const* override { return m_message.c_str(); }

private:
    std::string m_message;
};

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
 * @throw HashUtilsOperationFailed if `EvpDigestContext` fails to initialize.
 * @return Same as `digest_final` and `digest_update` on failure.
 */
[[nodiscard]] auto get_sha256_hash(
        std::span<unsigned char const> input,
        std::vector<unsigned char>& hash
) -> ErrorCode;
}  // namespace clp
#endif  // CLP_HASH_UTILS_HPP
