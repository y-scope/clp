use clp_rust_utils::{
    s3::{NormalizedS3ObjectUrls, ParsedS3Url, normalize_s3_object_urls, parse_s3_url},
    types::non_empty_string::ExpectedNonEmpty,
};
use non_empty_string::NonEmptyString;

const AWS_HOST_STYLE_URL: &str = "https://my-bucket.s3.us-east-1.amazonaws.com/logs/app.log";
const AWS_PATH_STYLE_URL: &str = "https://s3.us-east-1.amazonaws.com/my-bucket/logs/app.log";
const CUSTOM_HOST_STYLE_URL: &str = "http://my-bucket.s3.localhost.localstack.cloud/logs/app.log";
const NORMALIZED_URL_1: &str = "https://my-bucket.s3.us-east-1.amazonaws.com/logs/app/0001.log";
const NORMALIZED_URL_2: &str = "https://my-bucket.s3.us-east-1.amazonaws.com/logs/app/0002.log";

fn into_non_empty_string_vec(urls: &[&str]) -> Vec<NonEmptyString> {
    urls.iter()
        .map(|url| NonEmptyString::from_string((*url).to_string()))
        .collect()
}

#[test]
fn test_parse_s3_url_supports_aws_host_and_path_style_urls() {
    let expected = ParsedS3Url {
        endpoint_url: None,
        region: Some(NonEmptyString::from_static_str("us-east-1")),
        bucket_name: NonEmptyString::from_static_str("my-bucket"),
        key: NonEmptyString::from_static_str("logs/app.log"),
    };

    assert_eq!(
        parse_s3_url(AWS_HOST_STYLE_URL).expect("AWS host-style URL should parse"),
        expected
    );
    assert_eq!(
        parse_s3_url(AWS_PATH_STYLE_URL).expect("AWS path-style URL should parse"),
        expected
    );
}

#[test]
fn test_parse_s3_url_preserves_custom_region_and_strips_query_string() {
    let parsed = parse_s3_url("https://my-bucket.s3.nyc3.example.com/logs/app.log?versionId=123")
        .expect("custom-region URL with query string should parse");
    assert_eq!(
        parsed,
        ParsedS3Url {
            endpoint_url: Some(NonEmptyString::from_static_str("https://s3.example.com")),
            region: Some(NonEmptyString::from_static_str("nyc3")),
            bucket_name: NonEmptyString::from_static_str("my-bucket"),
            key: NonEmptyString::from_static_str("logs/app.log"),
        }
    );
}

#[test]
fn test_parse_s3_url_preserves_custom_host_style_endpoint_shape() {
    let parsed = parse_s3_url(CUSTOM_HOST_STYLE_URL).expect("custom host-style URL should parse");
    assert_eq!(
        parsed,
        ParsedS3Url {
            endpoint_url: Some(NonEmptyString::from_static_str(
                "http://s3.localstack.cloud"
            )),
            region: Some(NonEmptyString::from_static_str("localhost")),
            bucket_name: NonEmptyString::from_static_str("my-bucket"),
            key: NonEmptyString::from_static_str("logs/app.log"),
        }
    );
}

#[test]
fn test_normalize_s3_object_urls_returns_sorted_keys_and_common_prefix() {
    let normalized = normalize_s3_object_urls(&into_non_empty_string_vec(&[
        NORMALIZED_URL_2,
        NORMALIZED_URL_1,
    ]))
    .expect("URL normalization should succeed");
    assert_eq!(
        normalized,
        NormalizedS3ObjectUrls {
            endpoint_url: None,
            region: Some(NonEmptyString::from_static_str("us-east-1")),
            bucket_name: NonEmptyString::from_static_str("my-bucket"),
            common_key_prefix: NonEmptyString::from_static_str("logs/app/000"),
            object_keys: vec![
                NonEmptyString::from_static_str("logs/app/0001.log"),
                NonEmptyString::from_static_str("logs/app/0002.log"),
            ],
        }
    );
}

#[test]
fn test_normalize_s3_object_urls_rejects_duplicate_keys() {
    let err = normalize_s3_object_urls(&into_non_empty_string_vec(&[
        AWS_HOST_STYLE_URL,
        AWS_HOST_STYLE_URL,
    ]))
    .expect_err("duplicate keys should fail");
    assert_eq!(err.to_string(), "duplicate S3 key found: logs/app.log");
}

#[test]
fn test_normalize_s3_object_urls_rejects_incompatible_url_sets() {
    let mixed_region = normalize_s3_object_urls(&into_non_empty_string_vec(&[
        "https://my-bucket.s3.us-east-1.amazonaws.com/logs/app/1.log",
        "https://my-bucket.s3.us-west-2.amazonaws.com/logs/app/2.log",
    ]))
    .expect_err("mixed region should fail");
    assert_eq!(
        mixed_region.to_string(),
        "all S3 URLs must be in the same region: found us-east-1 and us-west-2"
    );

    let empty_prefix = normalize_s3_object_urls(&into_non_empty_string_vec(&[
        "https://my-bucket.s3.amazonaws.com/a.log",
        "https://my-bucket.s3.amazonaws.com/b.log",
    ]))
    .expect_err("empty common prefix should fail");
    assert_eq!(
        empty_prefix.to_string(),
        "the given S3 URLs have no common prefix"
    );
}
