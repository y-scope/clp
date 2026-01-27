#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "aws/AwsAuthenticationSigner.hpp"
#include "aws/constants.hpp"

namespace clp::aws::test {
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
}  // namespace clp::aws::test
