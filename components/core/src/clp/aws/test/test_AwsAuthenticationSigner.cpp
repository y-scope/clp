#include <chrono>
#include <optional>
#include <string>
#include <vector>

#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "../../ErrorCode.hpp"
#include "aws/AwsAuthenticationSigner.hpp"
#include "aws/constants.hpp"

namespace clp::aws::test {
namespace {
/**
 * @param signer
 * @return The `X-Amz-Expires` query parameter of a URL presigned by `signer`, delimited by '&'.
 */
auto get_presigned_url_expires_parameter(AwsAuthenticationSigner const& signer) -> std::string {
    S3Url const s3_url{"https://test-bucket.s3.us-west-1.amazonaws.com/logs/system.log"};
    std::string presigned_url;
    REQUIRE(ErrorCode_Success == signer.generate_presigned_url(s3_url, presigned_url));

    auto const begin{presigned_url.find("&X-Amz-Expires=")};
    REQUIRE(std::string::npos != begin);
    auto const end{presigned_url.find('&', begin + 1)};
    REQUIRE(std::string::npos != end);
    return presigned_url.substr(begin, end - begin + 1);
}
}  // namespace

TEST_CASE("s3_url_parsing", "[aws][AwsAuthenticationSigner]") {
    clp::aws::S3Url virtual_hosted{"http://test-bucket.s3.us-west-1.amazonaws.com/logs/system.log"};
    REQUIRE("test-bucket" == virtual_hosted.get_bucket());
    REQUIRE("test-bucket.s3.us-west-1.amazonaws.com" == virtual_hosted.get_host());
    REQUIRE("logs/system.log" == virtual_hosted.get_key());
    REQUIRE("us-west-1" == virtual_hosted.get_region());
    REQUIRE("http" == virtual_hosted.get_scheme());
    REQUIRE(clp::aws::S3Url::Style::VirtualHost == virtual_hosted.get_style());

    clp::aws::S3Url path_style{"https://s3.us-west-1.amazonaws.com/test-bucket/logs/system.log"};
    REQUIRE("test-bucket" == path_style.get_bucket());
    REQUIRE("s3.us-west-1.amazonaws.com" == path_style.get_host());
    REQUIRE("logs/system.log" == path_style.get_key());
    REQUIRE("us-west-1" == path_style.get_region());
    REQUIRE("https" == path_style.get_scheme());
    REQUIRE(clp::aws::S3Url::Style::Path == path_style.get_style());

    clp::aws::S3Url path_style_without_region{"http://localhost:4566/test-bucket/logs/system.log"};
    REQUIRE("test-bucket" == path_style_without_region.get_bucket());
    REQUIRE("localhost:4566" == path_style_without_region.get_host());
    REQUIRE("logs/system.log" == path_style_without_region.get_key());
    REQUIRE(cDefaultRegion == path_style_without_region.get_region());
    REQUIRE("http" == path_style_without_region.get_scheme());
    REQUIRE(clp::aws::S3Url::Style::Path == path_style_without_region.get_style());
}

TEST_CASE("presigned_url_expire_time", "[aws][AwsAuthenticationSigner]") {
    SECTION("The default expire time is unchanged.") {
        AwsAuthenticationSigner const signer{"access-key-id", "secret-access-key", std::nullopt};
        REQUIRE("&X-Amz-Expires=86400&" == get_presigned_url_expires_parameter(signer));
    }

    SECTION("Expire times within the AWS Signature Version 4 limits are used as given.") {
        std::vector<std::chrono::seconds> const expire_times{
                AwsAuthenticationSigner::cMinExpireTime,
                std::chrono::minutes{15},
                AwsAuthenticationSigner::cMaxExpireTime
        };
        for (auto const expire_time : expire_times) {
            CAPTURE(expire_time.count());
            AwsAuthenticationSigner const
                    signer{"access-key-id", "secret-access-key", "session-token", expire_time};
            REQUIRE("&X-Amz-Expires=" + std::to_string(expire_time.count()) + "&"
                    == get_presigned_url_expires_parameter(signer));
        }
    }

    SECTION("Expire times outside of the AWS Signature Version 4 limits are rejected.") {
        std::vector<std::chrono::seconds> const expire_times{
                std::chrono::seconds{-1},
                std::chrono::seconds{0},
                AwsAuthenticationSigner::cMaxExpireTime + std::chrono::seconds{1}
        };
        for (auto const expire_time : expire_times) {
            CAPTURE(expire_time.count());
            REQUIRE_THROWS_AS(
                    AwsAuthenticationSigner(
                            "access-key-id",
                            "secret-access-key",
                            std::nullopt,
                            expire_time
                    ),
                    AwsAuthenticationSigner::OperationFailed
            );
        }
    }
}
}  // namespace clp::aws::test
