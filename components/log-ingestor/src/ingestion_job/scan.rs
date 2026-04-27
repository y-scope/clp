use anyhow::Result;
use aws_sdk_s3::Client;
use clp_rust_utils::s3::ObjectMetadata;
use non_empty_string::NonEmptyString;

/// Scans all objects under the given S3 prefix using continuation tokens and invokes a callback
/// for each non-empty scanned page.
///
/// This function applies the following filters to the S3 objects:
/// * Entries with a missing key or size are skipped.
/// * Keys ending in `/` are skipped.
/// * Empty keys cause an error.
///
/// # Returns
///
/// The last scanned object key, if any non-directory object was scanned.
///
/// # Errors
///
/// * Forwards
///   [`aws_sdk_s3::operation::list_objects_v2::builders::ListObjectsV2FluentBuilder::send`]'s
///   return values on failure.
/// * Forwards [`i64::try_into`]'s return values when failing to convert object size to [`u64`].
/// * Forwards `on_page` callback return values on failure.
/// * Returns an [`anyhow::Error`] if S3 returns an empty object key.
pub async fn scan_prefix<CallbackType: AsyncFnMut(Vec<ObjectMetadata>) -> Result<()>>(
    client: &Client,
    bucket_name: &NonEmptyString,
    key_prefix: &NonEmptyString,
    start_after: Option<&NonEmptyString>,
    mut on_page: CallbackType,
) -> Result<Option<NonEmptyString>> {
    let mut continuation_token: Option<String> = None;
    let mut last_scanned_key: Option<NonEmptyString> = None;
    let mut use_start_after = start_after.map(std::string::ToString::to_string);

    loop {
        let response = client
            .list_objects_v2()
            .bucket(bucket_name.as_str())
            .prefix(key_prefix.as_str())
            .set_start_after(use_start_after.take())
            .set_continuation_token(continuation_token)
            .send()
            .await?;

        let Some(contents) = response.contents else {
            continuation_token = response.next_continuation_token;
            if continuation_token.is_none() {
                break;
            }
            continue;
        };

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
            last_scanned_key = Some(non_empty_key.clone());
            object_metadata_to_ingest.push(ObjectMetadata {
                bucket: bucket_name.clone(),
                key: non_empty_key,
                size: size.try_into()?,
            });
        }

        if !object_metadata_to_ingest.is_empty() {
            on_page(object_metadata_to_ingest).await?;
        }

        continuation_token = response.next_continuation_token;
        if continuation_token.is_none() {
            break;
        }
    }

    Ok(last_scanned_key)
}
