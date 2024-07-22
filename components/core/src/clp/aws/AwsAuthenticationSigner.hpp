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

namespace clp::aws {
/**
 * Class for parsing S3 HTTP URL
 */
class S3Url {
public:
    // Constructor
    S3Url(std::string const& url);
    S3Url(std::string const& s3_uri, std::string_view region);

    // Methods
    [[nodiscard]] auto get_host() -> std::string_view { return m_host; }

    [[nodiscard]] auto get_region() -> std::string_view { return m_region; }

    [[nodiscard]] auto get_path() -> std::string_view { return m_path; }

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

    // Constructors
    AwsAuthenticationSigner(std::string_view access_key_id, std::string_view secret_access_key)
            : m_access_key_id(access_key_id),
              m_secret_access_key(secret_access_key) {}

    // Methods
    /**
     * Generates a presigned URL using AWS Signature Version 4
     * @param s3_url S3 URL
     * @param method HTTP method
     * @return The generated presigned URL
     */
    [[nodiscard]] auto
    generate_presigned_url(S3Url& s3_url, HttpMethod method = HttpMethod::GET) -> std::string;

private:
    /**
     * Gets the default query string
     * @param scope
     * @param timestamp_string
     * @return
     */
    [[nodiscard]] auto get_canonical_query_string(
            std::string_view scope,
            std::string_view timestamp_string
    ) -> std::string;

    /**
     * Gets the signature key
     * @param region
     * @param date_string
     * @return
     */
    [[nodiscard]] auto get_signing_key(
            std::string_view region,
            std::string_view date_string,
            std::vector<unsigned char>& signing_key
    ) -> ErrorCode;

    /**
     *
     */
    /**
     * Gets the signature key
     * @param region
     * @param date_string
     * @return
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
