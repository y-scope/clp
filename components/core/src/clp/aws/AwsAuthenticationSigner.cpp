#include "AwsAuthenticationSigner.hpp"
#include "../type_utils.hpp"

using clp::size_checked_pointer_cast;

#include <regex>

#include <span>
#include <vector>

using std::span;
using std::vector;
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
    vector<unsigned char> signature_key {};
    vector<unsigned char> signature {};
    auto error_code = get_signature_key(s3_region, date_string, signature_key);
    error_code = get_signature(
        {size_checked_pointer_cast<unsigned char>(signature_key.data()), signature_key.size()},
        {size_checked_pointer_cast<unsigned char>(string_to_sign.data()), string_to_sign.size()},
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

[[nodiscard]] std::string AwsAuthenticationSigner::get_string_to_sign(
        std::string& scope,
        std::string& timestamp_string,
        std::string const& canonical_request
) {
    std::vector<unsigned char> signed_canonical_request;
    auto error_code = get_sha256_hash(canonical_request, signed_canonical_request);
    auto const signed_canonical_request_str = char_array_to_string({signed_canonical_request.data(), signed_canonical_request.size()});
    return fmt::format(
            "{}\n{}\n{}\n{}",
            cAws4HmacSha256,
            timestamp_string,
            scope,
            signed_canonical_request_str
    );
}

[[nodiscard]] ErrorCode
AwsAuthenticationSigner::get_signature_key(std::string const& region, std::string const& date_string, std::vector<unsigned char>& signature_key) {
    std::string input_key {cAws4};
    input_key += m_secret_access_key;

    std::vector<unsigned char> date_key{};
    std::vector<unsigned char> date_region_key{};
    std::vector<unsigned char> date_region_service_key{};
    auto error_code = get_hmac_sha256_hash(
            {size_checked_pointer_cast<unsigned char const>(input_key.data()), input_key.size()},
            {size_checked_pointer_cast<unsigned char const>(date_string.data()), date_string.size()},
            date_key);

    error_code = get_hmac_sha256_hash(
            {size_checked_pointer_cast<unsigned char const>(date_key.data()), date_key.size()},
            {size_checked_pointer_cast<unsigned char const>(region.data()), region.size()},
            date_region_key);

    error_code = get_hmac_sha256_hash(
            {size_checked_pointer_cast<unsigned char const>(date_region_key.data()), date_region_key.size()},
            {size_checked_pointer_cast<unsigned char const>(cS3Service.data()), cS3Service.size()},
            date_region_service_key);

    error_code = get_hmac_sha256_hash(
            {size_checked_pointer_cast<unsigned char const>(date_region_service_key.data()), date_region_service_key.size()},
            {size_checked_pointer_cast<unsigned char const>(cAws4Request.data()), cAws4Request.size()},
            signature_key
            );

    return ErrorCode_Success;
}


}  // namespace clp::aws
