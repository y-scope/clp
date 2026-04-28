use anyhow::Result;
use aws_sdk_s3::Client;
use clp_rust_utils::s3::ObjectMetadata;
use non_empty_string::NonEmptyString;

/// Scans all objects under the given S3 prefix using continuation tokens and invokes a callback
/// for each non-empty scanned page.
///
///
/// This function applies the following filters to the S3 objects:
/// * Entries with a missing key or size are skipped.
/// * Keys ending in `/` are skipped.
/// * Empty keys cause an error.
///
/// # Type Parameters
///
/// * [`CallbackType`]: The type of the async callback invoked once per non-empty scanned page of
///   object metadata. It must return (`should_continue_scanning`, `processed_last_key`), where
///   `should_continue_scanning` controls whether to fetch more pages, and `processed_last_key` is
///   the last key from that page.
///
/// # Returns
///
/// The last key successfully processed by `page_callback`, if any object was processed.
///
/// # Errors
///
/// * [`anyhow::Error`] if the request returns an empty object key.
/// * Forwards
///   [`aws_sdk_s3::operation::list_objects_v2::builders::ListObjectsV2FluentBuilder::send`]'s
///   return values on failure.
/// * Forwards [`i64::try_into`]'s return values when failing to convert object size to [`u64`].
/// * Forwards `page_callback` callback return values on failure.
pub async fn scan_prefix<
    CallbackType: AsyncFnMut(Vec<ObjectMetadata>) -> Result<(bool, NonEmptyString)>,
>(
    client: &Client,
    bucket_name: &NonEmptyString,
    key_prefix: &NonEmptyString,
    mut start_after: Option<NonEmptyString>,
    mut page_callback: CallbackType,
) -> Result<Option<NonEmptyString>> {
    let mut continuation_token: Option<String> = None;
    let mut last_scanned_key: Option<NonEmptyString> = None;

    loop {
        let mut request = client
            .list_objects_v2()
            .bucket(bucket_name.as_str())
            .prefix(key_prefix.as_str());

        if let Some(continuation_token) = continuation_token.take() {
            request = request.continuation_token(continuation_token);
        } else if let Some(start_after) = start_after.take() {
            request = request.start_after(start_after);
        }

        let response = request.send().await?;

        let contents = response.contents.unwrap_or_default();
        let mut object_metadata_to_ingest = Vec::with_capacity(contents.len());
        for content in contents {
            let (Some(key), Some(size)) = (content.key, content.size) else {
                continue;
            };
            if key.ends_with('/') {
                continue;
            }
            let non_empty_key = NonEmptyString::new(key)
                .map_err(|_| anyhow::anyhow!("received an empty object key from S3"))?;
            object_metadata_to_ingest.push(ObjectMetadata {
                bucket: bucket_name.clone(),
                key: non_empty_key,
                size: size.try_into()?,
            });
        }

        if !object_metadata_to_ingest.is_empty() {
            let (should_continue_scanning, processed_last_key) =
                page_callback(object_metadata_to_ingest).await?;
            last_scanned_key = Some(processed_last_key);
            if !should_continue_scanning {
                break;
            }
        }

        continuation_token = response.next_continuation_token;
        if continuation_token.is_none() {
            break;
        }
    }

    Ok(last_scanned_key)
}
