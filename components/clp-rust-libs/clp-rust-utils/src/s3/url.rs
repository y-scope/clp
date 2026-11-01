use std::sync::LazyLock;

use const_format::formatcp;
use non_empty_string::NonEmptyString;
use regex::Regex;

use crate::Error;

/// Generates the URL of an S3 object.
///
/// When `endpoint_url` is unset, a virtual-hosted-style URL against the default AWS endpoint
/// (`amazonaws.com`) is produced, with `region_code` inserted as a subdomain when set. When
/// `endpoint_url` is set, it is parsed into a scheme, an optional `s3` prefix, and a host, and a
/// path-style URL is produced; `region_code`, when set, is prepended to the host as a subdomain.
///
/// # Returns
///
/// The generated object URL.
///
/// # Errors
///
/// Returns an error if:
///
/// * [`Error::UnsupportedS3Endpoint`] if `endpoint_url` is set but is not a supported endpoint URL.
///
/// # Panics
///
/// This method panics if:
///
/// * The capture regex pattern is invalid.
/// * The regex pattern does not capture 'scheme' or 'endpoint'.
///
/// However, these cases are unreachable at runtime. They are documented for completeness.
pub fn generate_s3_url(
    endpoint_url: Option<&str>,
    region_code: Option<&str>,
    bucket: &NonEmptyString,
    object_key: &NonEmptyString,
) -> Result<String, Error> {
    const AWS_ENDPOINT: &str = "amazonaws.com";
    const SCHEME_GROUP: &str = "scheme";
    const S3_GROUP: &str = "s3";
    const ENDPOINT_GROUP: &str = "endpoint";
    const ENDPOINT_URL_PATTERN: &str = formatcp!(
        r"^(?P<{scheme}>(http|https))://((?P<{s3}>s3)\.)?(?P<{endpoint}>[a-z0-9.-]+(:[0-9]+)?)/?$",
        scheme = SCHEME_GROUP,
        s3 = S3_GROUP,
        endpoint = ENDPOINT_GROUP,
    );

    static ENDPOINT_URL_REGEX: LazyLock<Regex> =
        LazyLock::new(|| Regex::new(ENDPOINT_URL_PATTERN).expect("invalid regex pattern"));

    let Some(endpoint_url) = endpoint_url else {
        return Ok(region_code.map_or_else(
            || format!("https://{bucket}.s3.{AWS_ENDPOINT}/{object_key}"),
            |region| format!("https://{bucket}.s3.{region}.{AWS_ENDPOINT}/{object_key}"),
        ));
    };

    let Some(captures) = ENDPOINT_URL_REGEX.captures(endpoint_url) else {
        return Err(Error::UnsupportedS3Endpoint(endpoint_url.to_owned()));
    };
    let s3_prefix = captures.name(S3_GROUP).map_or("", |_| "s3.");
    let scheme = captures
        .name(SCHEME_GROUP)
        .expect("scheme should be unconditionally captured")
        .as_str();
    let endpoint = captures
        .name(ENDPOINT_GROUP)
        .expect("endpoint should be unconditionally captured")
        .as_str();

    Ok(region_code.map_or_else(
        || format!("{scheme}://{s3_prefix}{endpoint}/{bucket}/{object_key}"),
        |region| format!("{scheme}://{s3_prefix}{region}.{endpoint}/{bucket}/{object_key}"),
    ))
}

#[cfg(test)]
mod tests {
    use non_empty_string::NonEmptyString;

    use super::generate_s3_url;
    use crate::Error;
    use crate::types::non_empty_string::ExpectedNonEmpty;

    fn to_non_empty_string(value: &'static str) -> NonEmptyString {
        NonEmptyString::from_static_str(value)
    }

    #[test]
    fn default_endpoint_without_region() {
        assert_eq!(
            generate_s3_url(
                None,
                None,
                &to_non_empty_string("logs"),
                &to_non_empty_string("a/b.json")
            )
            .unwrap(),
            "https://logs.s3.amazonaws.com/a/b.json"
        );
    }

    #[test]
    fn default_endpoint_with_region() {
        assert_eq!(
            generate_s3_url(
                None,
                Some("us-east-1"),
                &to_non_empty_string("logs"),
                &to_non_empty_string("a/b.json")
            )
            .unwrap(),
            "https://logs.s3.us-east-1.amazonaws.com/a/b.json"
        );
    }

    #[test]
    fn custom_endpoint() {
        assert_eq!(
            generate_s3_url(
                Some("http://minio:9000"),
                None,
                &to_non_empty_string("logs"),
                &to_non_empty_string("a/b.json")
            )
            .unwrap(),
            "http://minio:9000/logs/a/b.json"
        );
    }

    #[test]
    fn custom_endpoint_trailing_slash_trimmed() {
        assert_eq!(
            generate_s3_url(
                Some("http://minio:9000/"),
                None,
                &to_non_empty_string("logs"),
                &to_non_empty_string("a/b.json")
            )
            .unwrap(),
            "http://minio:9000/logs/a/b.json"
        );
    }

    #[test]
    fn custom_endpoint_prepends_region() {
        assert_eq!(
            generate_s3_url(
                Some("http://minio:9000"),
                Some("us-east-1"),
                &to_non_empty_string("logs"),
                &to_non_empty_string("a/b.json")
            )
            .unwrap(),
            "http://us-east-1.minio:9000/logs/a/b.json"
        );
    }

    #[test]
    fn custom_endpoint_with_s3_prefix() {
        assert_eq!(
            generate_s3_url(
                Some("https://s3.example.com"),
                None,
                &to_non_empty_string("logs"),
                &to_non_empty_string("a/b.json")
            )
            .unwrap(),
            "https://s3.example.com/logs/a/b.json"
        );
    }

    #[test]
    fn unsupported_endpoint_url() {
        assert!(matches!(
            generate_s3_url(
                Some("ftp://example.com"),
                None,
                &to_non_empty_string("logs"),
                &to_non_empty_string("a/b.json")
            ),
            Err(Error::UnsupportedS3Endpoint(_))
        ));
    }
}
