#ifndef CLP_AWS_AWSAUTHENTICATIONSIGNER_HPP
#define CLP_AWS_AWSAUTHENTICATIONSIGNER_HPP

#include <chrono>

#include <fmt/chrono.h>
#include <fmt/format.h>

#include "Constants.hpp"
#include "hash_utils.hpp"

namespace clp::aws {
/**
 * Class for parsing S3 HTTP URL
 */
class S3Url {
public:
    // Constructor
    explicit S3Url(std::string const& url);

    S3Url() = delete;

    // Methods
    [[nodiscard]] std::string_view get_host() { return m_host; }

    [[nodiscard]] std::string_view get_bucket() { return m_bucket; }

    [[nodiscard]] std::string_view get_region() { return m_region; }

    [[nodiscard]] std::string_view get_path() { return m_path; }

private:
    std::string m_host;
    std::string m_bucket;
    std::string m_region;
    std::string m_path;
};

/**
 * Class for signing AWS requests
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

    enum class AwsService : uint8_t {
        S3
    };

    // Constructors
    AwsAuthenticationSigner(
            std::string& access_key_id,
            std::string& secret_access_key,
            AwsService service = AwsService::S3
    )
            : m_access_key_id(access_key_id),
              m_secret_access_key(secret_access_key),
              m_service(service) {
        if (AwsService::S3 != m_service) {
            throw std::invalid_argument("Unsupported service");
        }
    }

    AwsAuthenticationSigner()
            : m_access_key_id(getenv("AWS_ACCESS_KEY_ID")),
              m_secret_access_key(getenv("AWS_SECRET_ACCESS_KEY")) {
        if (m_access_key_id.empty()) {
            throw std::invalid_argument("AWS_ACCESS_KEY_ID environment variable is not set");
        }
        if (m_secret_access_key.empty()) {
            throw std::invalid_argument("AWS_SECRET_ACCESS_KEY environment variable is not set");
        }
    }

    // Methods
    /**
     * Generates a presigned URL using AWS Signature Version 4
     * @param s3_url S3 URL
     * @param method HTTP method
     * @return The generated presigned URL
     */
    [[nodiscard]] std::string
    generate_presigned_url(S3Url& s3_url, HttpMethod method = HttpMethod::GET);

private:
    /**
     * Gets the default query string
     * @param scope
     * @param timestamp_string
     * @return
     */
    [[nodiscard]] std::string
    get_default_query_string(std::string_view scope, std::string_view timestamp_string);

    /**
     * Gets the signature key
     * @param region
     * @param date_string
     * @return
     */
    [[nodiscard]] ErrorCode
    get_signature_key(std::string_view region, std::string_view date_string, std::vector<unsigned char>& signature_key);

    // Variables
    std::string m_access_key_id;
    std::string m_secret_access_key;
    AwsService m_service{AwsService::S3};
};
}  // namespace clp::aws

#endif  // CLP_AWS_AWSAUTHENTICATIONSIGNER_HPP
