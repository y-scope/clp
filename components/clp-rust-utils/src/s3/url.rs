/// Generates the URL of an S3 object.
///
/// When `endpoint_url` is set, a path-style URL rooted at that endpoint is produced and
/// `region_code` is ignored, since the endpoint already fully specifies the host. Otherwise, a
/// virtual-hosted-style URL against the default AWS endpoint is produced, incorporating
/// `region_code` when it is set.
///
/// # Returns
///
/// The generated object URL.
#[must_use]
pub fn generate_s3_url(
    endpoint_url: Option<&str>,
    region_code: Option<&str>,
    bucket: &str,
    object_key: &str,
) -> String {
    endpoint_url.map_or_else(
        || {
            region_code.map_or_else(
                || format!("https://{bucket}.s3.amazonaws.com/{object_key}"),
                |region| format!("https://{bucket}.s3.{region}.amazonaws.com/{object_key}"),
            )
        },
        |endpoint| format!("{}/{bucket}/{object_key}", endpoint.trim_end_matches('/')),
    )
}

#[cfg(test)]
mod tests {
    use super::generate_s3_url;

    #[test]
    fn default_endpoint_without_region() {
        assert_eq!(
            generate_s3_url(None, None, "logs", "a/b.json"),
            "https://logs.s3.amazonaws.com/a/b.json"
        );
    }

    #[test]
    fn default_endpoint_with_region() {
        assert_eq!(
            generate_s3_url(None, Some("us-east-1"), "logs", "a/b.json"),
            "https://logs.s3.us-east-1.amazonaws.com/a/b.json"
        );
    }

    #[test]
    fn custom_endpoint() {
        assert_eq!(
            generate_s3_url(Some("http://minio:9000"), None, "logs", "a/b.json"),
            "http://minio:9000/logs/a/b.json"
        );
    }

    #[test]
    fn custom_endpoint_trailing_slash_trimmed() {
        assert_eq!(
            generate_s3_url(Some("http://minio:9000/"), None, "logs", "a/b.json"),
            "http://minio:9000/logs/a/b.json"
        );
    }

    #[test]
    fn custom_endpoint_ignores_region() {
        assert_eq!(
            generate_s3_url(
                Some("http://minio:9000"),
                Some("us-east-1"),
                "logs",
                "a/b.json"
            ),
            "http://minio:9000/logs/a/b.json"
        );
    }
}
