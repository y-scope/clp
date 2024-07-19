#include "AwsAuthenticationSigner.hpp"

#include "../hash_utils.hpp"
#include "../type_utils.hpp"

using clp::size_checked_pointer_cast;

#include <regex>
#include <span>
#include <vector>

using std::span;
using std::string;
using std::string_view;
using std::vector;

namespace {
/**
 * Gets the timestamp string
 * @param timestamp
 * @return The timestamp string
 */
[[nodiscard]] string get_timestamp_string(std::chrono::system_clock::time_point const& timestamp) {
    return fmt::format("{:%Y%m%dT%H%M%SZ}", timestamp);
}

/**
 * Gets the date string
 * @param timestamp
 * @return The date string
 */
[[nodiscard]] string get_date_string(std::chrono::system_clock::time_point const& timestamp) {
    return fmt::format("{:%Y%m%d}", timestamp);
}

/**
 * Converts an HttpMethod to a string
 * @param method HTTP method
 * @return The converted string
 */
[[nodiscard]] string get_method_string(clp::aws::AwsAuthenticationSigner::HttpMethod method) {
    switch (method) {
        case clp::aws::AwsAuthenticationSigner::HttpMethod::GET:
            return "GET";
        case clp::aws::AwsAuthenticationSigner::HttpMethod::PUT:
            return "PUT";
        case clp::aws::AwsAuthenticationSigner::HttpMethod::POST:
            return "POST";
        case clp::aws::AwsAuthenticationSigner::HttpMethod::DELETE:
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
[[nodiscard]] string
get_string_to_sign(string_view scope, string_view timestamp_string, string_view canonical_request) {
    vector<unsigned char> signed_canonical_request;
    auto error_code = clp::aws::get_sha256_hash(canonical_request, signed_canonical_request);
    auto const signed_canonical_request_str = clp::aws::char_array_to_string(
            {signed_canonical_request.data(), signed_canonical_request.size()}
    );
    return fmt::format(
            "{}\n{}\n{}\n{}",
            clp::aws::cAws4HmacSha256,
            timestamp_string,
            scope,
            signed_canonical_request_str
    );
}

/**
 * Gets the encoded URI
 * @param value
 * @param encode_slash
 * @return The encoded URI
 */
[[nodiscard]] string get_encoded_uri(string_view value, bool encode_slash = true) {
    string encoded_uri;

    for (auto const c : value) {
        if ((std::isalnum(c) != 0) || c == '-' || c == '_' || c == '.' || c == '~') {
            encoded_uri += c;
        } else if (c == '/' && false == encode_slash) {
            encoded_uri += c;
        } else {
            encoded_uri += fmt::format("%{:02X}", c);
        }
    }

    return encoded_uri;
}

/**
 * Gets the scope
 * @param date_string
 * @param region
 * @return The scope
 */
[[nodiscard]] string get_scope(string_view date_string, string_view region) {
    return fmt::format(
            "{}/{}/{}/{}",
            date_string,
            region,
            clp::aws::cS3Service,
            clp::aws::cAws4Request
    );
}

/**
 * Gets the canonical request string
 * @param method HTTP method
 * @param url S3 URL
 * @param query_string Query string
 * @return Canonical request
 */
[[nodiscard]] string get_canonical_request(
        clp::aws::AwsAuthenticationSigner::HttpMethod method,
        clp::aws::S3Url& url,
        string_view query_string
) {
    return fmt::format(
            "{}\n{}\n{}\n{}:{}\n\n{}\n{}",
            get_method_string(method),
            get_encoded_uri(url.get_path(), false),
            query_string,
            clp::aws::cDefaultSignedHeaders,
            url.get_host(),
            clp::aws::cDefaultSignedHeaders,
            clp::aws::cUnsignedPayload
    );
}
}  // namespace

namespace clp::aws {
S3Url::S3Url(string const& url) {
    // Regular expression to match virtual host-style HTTP URL format
    std::regex const host_style_url_regex(
            R"(https://([a-z0-9.-]+)\.s3(\.([a-z0-9-]+))?\.amazonaws\.com(/[^?]+).*)"
    );
    // Regular expression to match path-style HTTP URL format
    std::regex const path_style_url_regex(
            R"(https://s3(\.([a-z0-9-]+))?\.amazonaws\.com/([a-z0-9.-]+)(/[^?]+).*)"
    );

    std::smatch match;
    if (std::regex_match(url, match, host_style_url_regex)) {
        m_bucket = match[1].str();
        m_region = match[3].str();
        m_path = match[4].str();
    } else if (std::regex_match(url, match, path_style_url_regex)) {
        m_region = match[2].str();
        m_bucket = match[3].str();
        m_path = match[4].str();
    } else {
        throw std::invalid_argument("Invalid S3 HTTP URL format");
    }

    if (m_region.empty()) {
        m_region = cDefaultRegion;
    }
    m_host = fmt::format("{}.s3.{}.amazonaws.com", m_bucket, m_region);
}

S3Url::S3Url(string const& s3_uri, string_view region) : m_region{region} {
    // Regular expression to match S3 URI format. But it does not include region.
    std::regex const s3_uri_regex(R"(s3://([a-z0-9.-]+)(/[^?]+).*)");

    std::smatch match;
    if (std::regex_match(s3_uri, match, s3_uri_regex)) {
        m_bucket = match[1].str();
        m_path = match[2].str();
    } else {
        throw std::invalid_argument("S3 URI format");
    }

    m_host = fmt::format("{}.s3.{}.amazonaws.com", m_bucket, m_region);
}

string AwsAuthenticationSigner::generate_presigned_url(S3Url& s3_url, HttpMethod method) {
    if (HttpMethod::GET != method) {
        throw std::runtime_error("Unsupported HTTP method!");
    }

    auto const s3_region = s3_url.get_region();

    // Gets current time
    auto const now = std::chrono::system_clock::now();
    auto const date_string = get_date_string(now);
    auto const timestamp_string = get_timestamp_string(now);

    auto scope = get_scope(date_string, s3_region);
    auto query_string = get_default_query_string(scope, timestamp_string);
    auto canonical_request = get_canonical_request(method, s3_url, query_string);
    auto string_to_sign = get_string_to_sign(scope, timestamp_string, canonical_request);
    vector<unsigned char> signature_key{};
    vector<unsigned char> signature{};
    auto error_code = get_signature_key(s3_region, date_string, signature_key);
    error_code = get_hmac_sha256_hash(
            {size_checked_pointer_cast<unsigned char>(signature_key.data()), signature_key.size()},
            {size_checked_pointer_cast<unsigned char>(string_to_sign.data()), string_to_sign.size()
            },
            signature
    );

    auto const signature_str = char_array_to_string({signature.data(), signature.size()});

    return fmt::format(
            "https://{}{}?{}&{}={}",
            s3_url.get_host(),
            s3_url.get_path(),
            query_string,
            cXAmzSignature,
            signature_str
    );
}

[[nodiscard]] ErrorCode AwsAuthenticationSigner::get_signature_key(
        string_view region,
        string_view date_string,
        vector<unsigned char>& signature_key
) {
    string input_key{cAws4};
    input_key += m_secret_access_key;

    vector<unsigned char> date_key{};
    vector<unsigned char> date_region_key{};
    vector<unsigned char> date_region_service_key{};
    auto error_code = get_hmac_sha256_hash(
            {size_checked_pointer_cast<unsigned char const>(input_key.data()), input_key.size()},
            {size_checked_pointer_cast<unsigned char const>(date_string.data()), date_string.size()
            },
            date_key
    );

    error_code = get_hmac_sha256_hash(
            {size_checked_pointer_cast<unsigned char const>(date_key.data()), date_key.size()},
            {size_checked_pointer_cast<unsigned char const>(region.data()), region.size()},
            date_region_key
    );

    error_code = get_hmac_sha256_hash(
            {size_checked_pointer_cast<unsigned char const>(date_region_key.data()),
             date_region_key.size()},
            {size_checked_pointer_cast<unsigned char const>(cS3Service.data()), cS3Service.size()},
            date_region_service_key
    );

    error_code = get_hmac_sha256_hash(
            {size_checked_pointer_cast<unsigned char const>(date_region_service_key.data()),
             date_region_service_key.size()},
            {size_checked_pointer_cast<unsigned char const>(cAws4Request.data()),
             cAws4Request.size()},
            signature_key
    );

    return ErrorCode_Success;
}

string
AwsAuthenticationSigner::get_default_query_string(string_view scope, string_view timestamp_string) {
    string uri{m_access_key_id + "/"};
    uri += scope;
    return fmt::format(
            "{}={}&{}={}&{}={}&{}={}&{}={}",
            cXAmzAlgorithm,
            cAws4HmacSha256,
            cXAmzCredential,
            get_encoded_uri(uri),
            cXAmzDate,
            timestamp_string,
            cXAmzExpires,
            cDefaultExpireTime,
            cXAmzSignedHeaders,
            cDefaultSignedHeaders
    );
}

}  // namespace clp::aws
