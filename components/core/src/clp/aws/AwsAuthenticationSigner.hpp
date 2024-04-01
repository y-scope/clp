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
    std::string& get_host() { return m_host; }

    std::string& get_bucket() { return m_bucket; }

    std::string& get_region() { return m_region; }

    std::string& get_path() { return m_path; }

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
    using time_point = std::chrono::system_clock::time_point;

    // Default expire time of presigned URL in seconds
    static constexpr int const cDefaultExpireTime = 86'400;  // 24 hours

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

    AwsAuthenticationSigner() {
        m_access_key_id = getenv("AWS_ACCESS_KEY_ID");
        m_secret_access_key = getenv("AWS_SECRET_ACCESS_KEY");

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
     * @param url URL
     * @param method HTTP method
     * @return The generated presigned URL
     */
    std::string generate_presigned_url(std::string const& url, HttpMethod method);

private:
    /**
     * Converts an HttpMethod to a string
     * @param method HTTP method
     * @return The converted string
     */
    static std::string get_method_string(HttpMethod method) {
        switch (method) {
            case HttpMethod::GET:
                return "GET";
            case HttpMethod::PUT:
                return "PUT";
            case HttpMethod::POST:
                return "POST";
            case HttpMethod::DELETE:
                return "DELETE";
            default:
                throw std::runtime_error("Invalid HTTP method");
        }
    }

    /**
     * Gets the string to sign
     * @param scope
     * @param timestamp_string
     * @param canonical_request
     * @return String to sign
     */
    static std::string get_string_to_sign(
            std::string& scope,
            std::string& timestamp_string,
            std::string const& canonical_request
    ) {
        return fmt::format(
                "{}\n{}\n{}\n{}",
                cAws4HmacSha256,
                timestamp_string,
                scope,
                get_sha256_hash(canonical_request)
        );
    }

    /**
     * Gets the canonical request string
     * @param method HTTP method
     * @param url S3 URL
     * @param query_string Query string
     * @return Canonical request
     */
    static std::string
    get_canonical_request(HttpMethod method, S3Url& url, std::string const& query_string) {
        return fmt::format(
                "{}\n{}\n{}\n{}:{}\n\n{}\n{}",
                get_method_string(method),
                get_encoded_uri(url.get_path(), false),
                query_string,
                cDefaultSignedHeaders,
                url.get_host(),
                cDefaultSignedHeaders,
                cUnsignedPayload
        );
    }

    /**
     * Gets the encoded URI
     * @param value
     * @param encode_slash
     * @return The encoded URI
     */
    static std::string get_encoded_uri(std::string const& value, bool encode_slash = true) {
        std::string encoded_uri;

        for (char c : value) {
            if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
                encoded_uri += c;
                continue;
            }

            if (c == '/' && false == encode_slash) {
                encoded_uri += c;
                continue;
            }

            encoded_uri += fmt::format("%{:02X}", static_cast<int>(static_cast<unsigned char>(c)));
        }

        return encoded_uri;
    }

    /**
     * Gets the scope
     * @param date_string
     * @param region
     * @return The scope
     */
    static std::string get_scope(std::string& date_string, std::string const& region) {
        return fmt::format("{}/{}/{}/{}", date_string, region, cS3Service, cAws4Request);
    }

    /**
     * Gets the timestamp string
     * @param timestamp
     * @return The timestamp string
     */
    static std::string get_timestamp_string(time_point& timestamp) {
        return fmt::format("{:%Y%m%dT%H%M%SZ}", timestamp);
    }

    /**
     * Gets the date string
     * @param timestamp
     * @return The date string
     */
    static std::string get_date_string(time_point& timestamp) {
        return fmt::format("{:%Y%m%d}", timestamp);
    }

    /**
     * Gets the default query string
     * @param scope
     * @param timestamp_string
     * @return
     */
    std::string get_default_query_string(std::string& scope, std::string& timestamp_string) {
        return fmt::format(
                "{}={}&{}={}&{}={}&{}={}&{}={}",
                cXAmzAlgorithm,
                cAws4HmacSha256,
                cXAmzCredential,
                get_encoded_uri(m_access_key_id + "/" + scope),
                cXAmzDate,
                timestamp_string,
                cXAmzExpires,
                cDefaultExpireTime,
                cXAmzSignedHeaders,
                cDefaultSignedHeaders
        );
    }

    /**
     * Gets the signature key
     * @param region
     * @param date_string
     * @return
     */
    std::string get_signature_key(std::string const& region, std::string const& date_string) {
        std::string date_key = get_hmac_sha256_hash(cAws4 + m_secret_access_key, date_string);
        std::string date_region_key = get_hmac_sha256_hash(date_key, region);
        std::string date_region_service_key = get_hmac_sha256_hash(date_region_key, cS3Service);
        return get_hmac_sha256_hash(date_region_service_key, cAws4Request);
    }

    /**
     * Gets the signature
     * @param signature_key
     * @param string_to_sign
     * @return
     */
    static std::string
    get_signature(std::string const& signature_key, std::string const& string_to_sign) {
        return get_hmac_sha256_hash(signature_key, string_to_sign, true);
    }

    // Variables
    std::string m_access_key_id;
    std::string m_secret_access_key;
    AwsService m_service{AwsService::S3};
};
}  // namespace clp::aws

#endif  // CLP_AWS_AWSAUTHENTICATIONSIGNER_HPP
