#ifndef CLP_AWS_CONSTANTS_HPP
#define CLP_AWS_CONSTANTS_HPP

#include <string_view>

namespace clp::aws {
// Query String Parameter Names
constexpr char const* const cXAmzAlgorithm = "X-Amz-Algorithm";
constexpr char const* const cXAmzCredential = "X-Amz-Credential";
constexpr char const* const cXAmzContentSha256 = "X-Amz-Content-Sha256";
constexpr char const* const cXAmzDate = "X-Amz-Date";
constexpr char const* const cXAmzExpires = "X-Amz-Expires";
constexpr char const* const cXAmzSignature = "X-Amz-Signature";
constexpr char const* const cXAmzSignedHeaders = "X-Amz-SignedHeaders";

// Other Constants
constexpr char const* const cAws4 = "AWS4";
constexpr std::string_view cAws4Request {"aws4_request"};
constexpr char const* const cAws4HmacSha256 = "AWS4-HMAC-SHA256";
constexpr char const* const cDefaultSignedHeaders = "host";
constexpr char const* const cDefaultRegion = "us-east-1";
constexpr char const* const cEmptyStringSha256
        = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";
constexpr std::string_view cS3Service {"s3"};
constexpr char const* const cUnsignedPayload = "UNSIGNED-PAYLOAD";
}  // namespace clp::aws

#endif  // CLP_AWS_CONSTANTS_HPP
