use anyhow::{Context, Result};
use clp_rust_utils::{s3::ObjectMetadata, types::non_empty_string::ExpectedNonEmpty};
use log_ingestor::ingestion_job::IngestionJobId;
use non_empty_string::NonEmptyString;
use uuid::Uuid;

/// Creates S3 objects in the given bucket based on the provided metadata. The object content is
/// filled with dummy data.
///
/// # Errors
///
/// * Forwards [`aws_sdk_s3::operation::put_object::builders::PutObjectFluentBuilder::send`]'s
///   return values on failure.
/// * Returns an [`anyhow::Error`] if the object size cannot be converted to [`usize`].
pub async fn create_s3_objects(
    s3_client: aws_sdk_s3::Client,
    objects: Vec<ObjectMetadata>,
) -> Result<()> {
    for object in objects {
        let body =
            vec![b'0'; usize::try_from(object.size).context("object size does not fit in usize")?];

        s3_client
            .put_object()
            .bucket(object.bucket.as_str())
            .key(object.key.as_str())
            .body(body.into())
            .send()
            .await?;
    }
    Ok(())
}

/// # Returns
///
/// A unique testing prefix for S3 object keys. The prefix is formatted as `test-{job_id}`.
#[must_use]
pub fn get_testing_prefix_as_non_empty_string(job_id: IngestionJobId) -> NonEmptyString {
    NonEmptyString::from_string(format!("test-{job_id}"))
}

/// Uploads test S3 objects.
///
/// Objects are created with keys formatted as `{prefix}/{i}.log` where `i` is the object index.
///
/// # Returns
///
/// A vector of created S3 object metadata.
///
/// # Errors
///
/// * Forwards [`create_s3_objects`]'s return values on failure.
pub async fn upload_test_objects(
    s3_client: aws_sdk_s3::Client,
    bucket: NonEmptyString,
    prefix: NonEmptyString,
    num_objects_to_create: usize,
) -> Result<Vec<ObjectMetadata>> {
    let objects_to_create: Vec<_> = (0..num_objects_to_create)
        .map(|idx| ObjectMetadata {
            bucket: bucket.clone(),
            key: NonEmptyString::from_string(format!("{prefix}/{idx:05}.log")),
            size: 16,
        })
        .collect();

    create_s3_objects(s3_client, objects_to_create.clone()).await?;
    Ok(objects_to_create)
}

/// Uploads noise S3 objects that do not match any testing prefix.
///
/// The keys are formatted as `{uuid}.log`, where `uuid` is a randomly generated v4 UUID.
///
/// # Errors
///
/// * Forwards [`create_s3_objects`]'s return values on failure.
pub async fn upload_noise_objects(
    s3_client: aws_sdk_s3::Client,
    bucket: NonEmptyString,
    num_objects_to_create: usize,
) -> Result<()> {
    let objects_to_create: Vec<_> = (0..num_objects_to_create)
        .map(|_| ObjectMetadata {
            bucket: bucket.clone(),
            key: NonEmptyString::from_string(format!("{}.log", Uuid::new_v4())),
            size: 16,
        })
        .collect();

    create_s3_objects(s3_client, objects_to_create).await
}
