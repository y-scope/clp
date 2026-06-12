use std::{collections::BTreeSet, sync::LazyLock};

use anyhow::Result;
use non_empty_string::NonEmptyString;
use regex::Regex;

use crate::types::non_empty_string::ExpectedNonEmpty;

const AWS_ENDPOINT: &str = "amazonaws.com";
static HOST_STYLE_URL_REGEX: LazyLock<Regex> = LazyLock::new(|| {
    Regex::new(concat!(
        r"^(?P<scheme>https?)://",
        r"(?P<bucket_name>[a-z0-9.-]+)\.",
        r"(?P<s3>s3)\.",
        r"(?:(?P<region_code>[a-z0-9\-]+)\.)?",
        r"(?P<endpoint>[A-Za-z0-9.-]+(?::[0-9]+)?)",
        r"/(?P<key_prefix>[^?]+)(?:\?.*)?$"
    ))
    .expect("host-style S3 URL regex should be valid")
});
static PATH_STYLE_URL_REGEX: LazyLock<Regex> = LazyLock::new(|| {
    Regex::new(concat!(
        r"^(?P<scheme>https?)://",
        r"(?:(?P<s3>s3)\.(?:(?P<region_code>[a-z0-9\-]+)\.)?)?",
        r"(?P<endpoint>[A-Za-z0-9.-]+(?::[0-9]+)?)",
        r"/(?P<bucket_name>[a-z0-9.-]+)",
        r"/(?P<key_prefix>[^?]+)(?:\?.*)?$"
    ))
    .expect("path-style S3 URL regex should be valid")
});

/// Parsed components of an S3 object URL.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ParsedS3Url {
    pub endpoint_url: Option<NonEmptyString>,
    pub region: Option<NonEmptyString>,
    pub bucket_name: NonEmptyString,
    pub key: NonEmptyString,
}

/// Normalized representation of a set of S3 object URLs.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct NormalizedS3ObjectUrls {
    pub endpoint_url: Option<NonEmptyString>,
    pub region: Option<NonEmptyString>,
    pub bucket_name: NonEmptyString,
    pub common_key_prefix: NonEmptyString,
    pub object_keys: Vec<NonEmptyString>,
}

/// Parses an S3 object URL into endpoint, region, bucket, and key components.
///
/// This function supports both host-style and path-style S3 URLs:
/// * For AWS endpoints, `endpoint_url` is normalized to `None`;
/// * For custom S3-compatible endpoints, the returned `endpoint_url` preserves the submitted scheme
///   and host;
///
/// # Returns
///
/// The parsed S3 URL on success.
///
/// # Errors
///
/// Returns an error if:
///
/// * [`anyhow::Error`] if the URL does not match a supported S3 host-style or path-style format.
/// * [`anyhow::Error`] if the parsed bucket or key is empty.
///
/// # Panics
///
/// Panics if the internal host-style or path-style S3 URL regexes do not contain the expected
/// capture groups.
pub fn parse_s3_url(url: &str) -> Result<ParsedS3Url> {
    let captures = HOST_STYLE_URL_REGEX
        .captures(url)
        .or_else(|| PATH_STYLE_URL_REGEX.captures(url))
        .ok_or_else(|| anyhow::anyhow!("unsupported URL format: {url}"))?;

    let scheme = captures
        .name("scheme")
        .expect("S3 URL regex should capture scheme")
        .as_str();
    let endpoint = captures
        .name("endpoint")
        .expect("S3 URL regex should capture endpoint")
        .as_str()
        .to_string();
    let bucket_name = captures
        .name("bucket_name")
        .expect("S3 URL regex should capture bucket name")
        .as_str();
    let key = NonEmptyString::new(
        captures
            .name("key_prefix")
            .expect("S3 URL regex should capture key")
            .as_str()
            .to_string(),
    )
    .map_err(|_| anyhow::anyhow!("parsed empty object key from URL: {url}"))?;
    let region = captures
        .name("region_code")
        .map(|region| region.as_str().to_string());

    let s3_prefix = if captures.name("s3").is_some() {
        "s3."
    } else {
        ""
    };
    let endpoint_url = if endpoint == AWS_ENDPOINT {
        None
    } else {
        Some(NonEmptyString::from_string(format!(
            "{scheme}://{s3_prefix}{endpoint}"
        )))
    };
    let region = region.map(NonEmptyString::from_string);

    Ok(ParsedS3Url {
        endpoint_url,
        region,
        bucket_name: NonEmptyString::from_string(bucket_name.to_string()),
        key,
    })
}

/// Parses and validates a set of S3 object URLs for one-time explicit-keys ingestion.
///
/// * `urls` must be non-empty.
/// * All URLs must share the same endpoint, region, and bucket.
/// * Duplicate object keys are rejected.
/// * The object keys must share a non-empty common prefix.
///
/// # Returns
///
/// The normalized URL set on success.
///
/// # Errors
///
/// Returns an error if:
///
/// * [`anyhow::Error`] if `urls` is empty.
/// * [`anyhow::Error`] if the URLs do not share the same endpoint.
/// * [`anyhow::Error`] if the URLs do not share the same region.
/// * [`anyhow::Error`] if the URLs do not share the same bucket.
/// * [`anyhow::Error`] if duplicate object keys are found.
/// * [`anyhow::Error`] if the object keys have no non-empty common prefix.
/// * Forwards [`parse_s3_url`]'s return values on failure.
pub fn normalize_s3_object_urls(urls: &[NonEmptyString]) -> Result<NormalizedS3ObjectUrls> {
    let Some(first_url) = urls.first() else {
        return Err(anyhow::anyhow!("no S3 URLs provided"));
    };

    let ParsedS3Url {
        endpoint_url,
        region,
        bucket_name,
        key: first_key,
    } = parse_s3_url(first_url.as_str())?;

    let mut key_set = BTreeSet::from([first_key]);
    for url in &urls[1..] {
        let parsed = parse_s3_url(url.as_str())?;
        if endpoint_url != parsed.endpoint_url {
            return Err(anyhow::anyhow!(
                "all S3 URLs must have the same endpoint: found {} and {}",
                format_optional_non_empty_string(endpoint_url.as_ref()),
                format_optional_non_empty_string(parsed.endpoint_url.as_ref())
            ));
        }
        if region != parsed.region {
            return Err(anyhow::anyhow!(
                "all S3 URLs must be in the same region: found {} and {}",
                format_optional_non_empty_string(region.as_ref()),
                format_optional_non_empty_string(parsed.region.as_ref())
            ));
        }
        if bucket_name != parsed.bucket_name {
            return Err(anyhow::anyhow!(
                "all S3 URLs must be in the same bucket: found {} and {}",
                bucket_name,
                parsed.bucket_name
            ));
        }
        if !key_set.insert(parsed.key.clone()) {
            return Err(anyhow::anyhow!("duplicate S3 key found: {}", parsed.key));
        }
    }

    let object_keys: Vec<NonEmptyString> = key_set.into_iter().collect();
    let common_key_prefix = compute_common_prefix(&object_keys)
        .ok_or_else(|| anyhow::anyhow!("the given S3 URLs have no common prefix"))?;

    Ok(NormalizedS3ObjectUrls {
        endpoint_url,
        region,
        bucket_name,
        common_key_prefix,
        object_keys,
    })
}

/// Computes the non-empty common prefix shared by all keys.
///
/// # Returns
///
/// The common prefix if it is non-empty.
fn compute_common_prefix(keys: &[NonEmptyString]) -> Option<NonEmptyString> {
    let mut common_prefix = keys.first()?.as_str().to_string();
    for key in &keys[1..] {
        let mut prefix_len = 0;
        for (left_char, right_char) in common_prefix.chars().zip(key.as_str().chars()) {
            if left_char != right_char {
                break;
            }
            prefix_len += left_char.len_utf8();
        }
        common_prefix.truncate(prefix_len);
        if common_prefix.is_empty() {
            return None;
        }
    }

    Some(NonEmptyString::new(common_prefix).expect("common prefix should not be empty"))
}

/// Formats an optional non-empty string for human-readable validation errors.
///
/// # Returns
///
/// `"None"` if the value is absent, or the inner string otherwise.
fn format_optional_non_empty_string(value: Option<&NonEmptyString>) -> String {
    value.map_or_else(|| "None".to_string(), std::string::ToString::to_string)
}
