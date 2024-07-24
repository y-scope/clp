#ifndef CLP_AWS_AWSAUTHENTICATIONSIGNER_HPP
#define CLP_AWS_AWSAUTHENTICATIONSIGNER_HPP

#include <chrono>
#include <string>
#include <string_view>
#include <vector>

#include <fmt/chrono.h>
#include <fmt/format.h>

#include "../ErrorCode.hpp"
#include "Constants.hpp"
#include "../TraceableException.hpp"

namespace clp::aws {
/**
 * Class for parsing S3 URL. The format of S3 URL is specie
 */
class S3Url {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : OperationFailed(
                error_code,
                filename,
                line_number,
                "S3Url operation failed"
        ) {}

        OperationFailed(
                ErrorCode error_code,
                char const* const filename,
                int line_number,
                std::string message
        )
                : TraceableException(error_code, filename, line_number),
                m_message(std::move(message)) {}

        // Methods
        [[nodiscard]] auto what() const noexcept -> char const* override {
            return m_message.c_str();
        }

    private:
        std::string m_message;
    };

    // Constructor
    S3Url(std::string const& url);

    // Methods
    [[nodiscard]] auto get_host() const -> std::string_view { return m_host; }

    [[nodiscard]] auto get_region() const -> std::string_view { return m_region; }

    [[nodiscard]] auto get_path() const-> std::string_view { return m_path; }

    [[nodiscard]] auto get_compression_path() const -> std::string;

private:
    std::string m_host;
    std::string m_bucket;
    std::string m_region;
    std::string m_path;
};

/**
 * Class for signing AWS requests. The class is based on AWS Signature Version 4.
 * A detailed explanation on how the signature is generated can be found at
 * https://docs.aws.amazon.com/AmazonS3/latest/API/sigv4-query-string-auth.html
 */
class AwsAuthenticationSigner {
public:
    // Default expire time of presigned URL in seconds
    static constexpr int cDefaultExpireTime = 86'400;  // 24 hours

    // Types
    enum class HttpMethod : uint8_t {
        GET,
        PUT,
        POST,
        DELETE
    };

    // Constructors
    AwsAuthenticationSigner(std::string_view access_key_id, std::string_view secret_access_key)
            : m_access_key_id{access_key_id},
              m_secret_access_key{secret_access_key} {}

    // Methods
    /**
     * Generates a presigned URL using AWS Signature Version 4
     * @param s3_url S3 URL
     * @param presigned_url Returns the generated presigned url
     * @param method HTTP method
     * @return ErrorCode_Success on success.
     * On failure, same as get_string_to_sign and AwsAuthenticationSigner::get_signature
     *
     */
    [[nodiscard]] auto generate_presigned_url(
            S3Url const& s3_url,
            std::string& presigned_url,
            HttpMethod method = HttpMethod::GET
    ) -> ErrorCode;

private:
    /**
     * Generates the canonical query string
     * @param scope
     * @param timestamp_string
     * @return the canonical query string
     */
    [[nodiscard]] auto generate_canonical_query_string(
            std::string_view scope,
            std::string_view timestamp_string
    ) -> std::string;

    /**
     * Gets the signature signing key for the request
     * @param region
     * @param date_string
     * @param signing_key Returns the signing key
     * @return ErrorCode_Success on success.
     * Same as get_hmac_sha256_hash on Failure.
     */
    [[nodiscard]] auto get_signing_key(
            std::string_view region,
            std::string_view date_string,
            std::vector<unsigned char>& signing_key
    ) -> ErrorCode;

    /**
     * Signs the string_to_sign and returns the request signature by reference
     * @param region
     * @param date_string
     * @param string_to_sign StringToSign specified by the AWS Signature Version 4
     * @param signature Returns the signature
     * @return ErrorCode_Success on success.
     * Same as get_hmac_sha256_hash on Failure.
     */
    [[nodiscard]] ErrorCode get_signature(
            std::string_view region,
            std::string_view date_string,
            std::string_view string_to_sign,
            std::vector<unsigned char>& signature
    );

    // Variables
    std::string m_access_key_id;
    std::string m_secret_access_key;
};
}  // namespace clp::aws

#endif  // CLP_AWS_AWSAUTHENTICATIONSIGNER_HPP
