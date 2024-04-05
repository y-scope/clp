#include "AwsAuthenticationSigner.hpp"

#include <regex>

namespace clp::aws {
S3Url::S3Url(std::string const& url) {
    // Regular expression to match virtual host-style HTTP URL format
    //    std::regex regex1(R"(https://([a-z0-9.-]+)\.s3(\.([a-z0-9-]+))?\.amazonaws\.com(/.*?)$)");
    std::regex regex1(R"(https://([a-z0-9.-]+)\.s3(\.([a-z0-9-]+))?\.amazonaws\.com(/[^?]+).*)");
    // Regular expression to match path-style HTTP URL format
    //    std::regex regex2(R"(https://s3(\.([a-z0-9-]+))?\.amazonaws\.com/([a-z0-9.-]+)(/.*?)$)");
    std::regex regex2(R"(https://s3(\.([a-z0-9-]+))?\.amazonaws\.com/([a-z0-9.-]+)(/[^?]+).*)");

    std::smatch match;
    if (std::regex_match(url, match, regex1)) {
        m_bucket = match[1].str();
        m_region = match[3].str();
        m_path = match[4].str();
    } else if (std::regex_match(url, match, regex2)) {
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

std::string AwsAuthenticationSigner::generate_presigned_url(S3Url& s3_url, HttpMethod method) {
    if (HttpMethod::GET != method) {
        throw std::runtime_error("Unsupported HTTP method!");
    }

    auto s3_region = s3_url.get_region();

    // Gets current time
    auto now = std::chrono::system_clock::now();
    auto date_string = get_date_string(now);
    auto timestamp_string = get_timestamp_string(now);

    auto scope = get_scope(date_string, s3_region);
    auto query_string = get_default_query_string(scope, timestamp_string);
    auto canonical_request = get_canonical_request(method, s3_url, query_string);
    auto string_to_sign = get_string_to_sign(scope, timestamp_string, canonical_request);
    auto signature_key = get_signature_key(s3_region, date_string);
    auto signature = get_signature(signature_key, string_to_sign);

    return fmt::format(
            "https://{}{}?{}&{}={}",
            s3_url.get_host(),
            s3_url.get_path(),
            query_string,
            cXAmzSignature,
            signature
    );
}

}  // namespace clp::aws
