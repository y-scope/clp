mod aws_config;

use std::time::Duration;

use anyhow::{Context, Result};
use aws_config::AwsConfig;
use clp_rust_utils::{
    job_config::ingestion::s3::{
        BaseConfig,
        S3ScannerConfig,
        SqsListenerConfig,
        ValidatedSqsListenerConfig,
    },
    s3::ObjectMetadata,
    types::non_empty_string::ExpectedNonEmpty,
};
use log_ingestor::{
    aws_client_manager::{S3ClientWrapper, SqsClientWrapper},
    ingestion_job::SqsListener,
};
use non_empty_string::NonEmptyString;
use tokio::sync::mpsc;
use uuid::Uuid;

const RECEIVER_TIMEOUT_SEC: u64 = 30;
const NUM_TEST_OBJECTS: usize = 100;
const NUM_NOISE_OBJECTS: usize = 100;
const TEST_CHANNEL_CAPACITY: usize = 10;

/// Creates S3 objects in the given bucket based on the provided metadata. The object content is
/// filled with dummy data.
///
/// # Returns
///
/// `Ok(())` on success.
///
/// # Errors
///
/// Returns an error if:
///
/// * Forwards [`aws_sdk_s3::operation::put_object::builders::PutObjectFluentBuilder::send`]'s
///   return values on failure.
async fn create_s3_objects(
    s3_client: aws_sdk_s3::Client,
    objects: Vec<ObjectMetadata>,
) -> Result<()> {
    for object in objects {
        let body = vec![b'0'; usize::try_from(object.size).expect("size overflow")];

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

/// Receives up to `max_num_objects` object metadata from the given receiver channel.
///
/// # Returns
///
/// A vector of received S3 object metadata on success.
async fn receive_object_metadata(
    mut receiver: mpsc::Receiver<ObjectMetadata>,
    max_num_objects: usize,
) -> Vec<ObjectMetadata> {
    let mut received_objects = Vec::new();

    while received_objects.len() < max_num_objects {
        match receiver.recv().await {
            Some(object_metadata) => {
                received_objects.push(object_metadata);
            }
            None => {
                break;
            }
        }
    }

    received_objects
}

/// Uploads S3 objects and receives their metadata via the provided receiver channel.
///
/// Objects are created with keys formatted as `{prefix}/{i}.log` where `i` is the object index.
///
/// # Returns
///
/// A tuple containing:
///
/// * A vector of created S3 object metadata.
/// * A vector of received S3 object metadata.
async fn upload_and_receive(
    s3_client: aws_sdk_s3::Client,
    bucket: NonEmptyString,
    prefix: NonEmptyString,
    num_objects_to_create: usize,
    receiver: mpsc::Receiver<ObjectMetadata>,
) -> (Vec<ObjectMetadata>, Vec<ObjectMetadata>) {
    let objects_to_create: Vec<_> = (0..num_objects_to_create)
        .map(|idx| ObjectMetadata {
            bucket: bucket.clone(),
            key: NonEmptyString::from_string(format!("{prefix}/{idx:05}.log")),
            size: 16,
        })
        .collect();

    let creation_handler = tokio::spawn(create_s3_objects(
        s3_client.clone(),
        objects_to_create.clone(),
    ));

    let objects_received = tokio::time::timeout(
        Duration::from_secs(RECEIVER_TIMEOUT_SEC),
        receive_object_metadata(receiver, objects_to_create.len()),
    )
    .await
    .expect("Timed out while receiving object metadata");

    creation_handler
        .await
        .expect("Error while awaiting creation")
        .expect("Error during S3 object creation");
    (objects_to_create, objects_received)
}

/// Uploads noise S3 objects that do not match any testing prefix.
///
/// The keys are formatted as `{uuid}.log`, where `uuid` is a randomly generated v4 UUID.
async fn upload_noise_objects(
    s3_client: aws_sdk_s3::Client,
    bucket: NonEmptyString,
    num_objects_to_create: usize,
) {
    let objects_to_create: Vec<_> = (0..num_objects_to_create)
        .map(|_| ObjectMetadata {
            bucket: bucket.clone(),
            key: NonEmptyString::from_string(format!("{}.log", Uuid::new_v4())),
            size: 16,
        })
        .collect();

    create_s3_objects(s3_client.clone(), objects_to_create)
        .await
        .expect("Error during S3 object creation");
}

/// Runs SQS listener test with the given job config.
async fn run_sqs_listener_test(
    job_id: Uuid,
    prefix: NonEmptyString,
    aws_config: AwsConfig,
    sqs_listener_config: SqsListenerConfig,
) -> Result<()> {
    let sqs_client = clp_rust_utils::sqs::create_new_client(
        aws_config.access_key_id.as_str(),
        aws_config.secret_access_key.as_str(),
        aws_config.region.as_str(),
        Some(&aws_config.endpoint),
    )
    .await;

    let (sender, receiver) = mpsc::channel::<ObjectMetadata>(TEST_CHANNEL_CAPACITY);

    let sqs_listener = SqsListener::spawn(
        job_id,
        &SqsClientWrapper::from(sqs_client),
        &ValidatedSqsListenerConfig::validate_and_create(sqs_listener_config)
            .expect("invalid SQS listener config"),
        &sender,
    );

    let s3_client = clp_rust_utils::s3::create_new_client(
        aws_config.access_key_id.as_str(),
        aws_config.secret_access_key.as_str(),
        aws_config.region.as_str(),
        Some(&aws_config.endpoint),
    )
    .await;

    let upload_and_receive_handle = tokio::spawn(upload_and_receive(
        s3_client.clone(),
        aws_config.bucket_name.clone(),
        prefix.clone(),
        NUM_TEST_OBJECTS,
        receiver,
    ));
    let noise_upload_handle = tokio::spawn(upload_noise_objects(
        s3_client.clone(),
        aws_config.bucket_name.clone(),
        NUM_NOISE_OBJECTS,
    ));

    noise_upload_handle
        .await
        .context("Error while awaiting noise upload")?;
    let (mut created_objects, mut received_objects) = upload_and_receive_handle
        .await
        .context("Error while awaiting upload and receive")?;

    sqs_listener.shutdown_and_join().await;

    created_objects.sort();
    received_objects.sort();
    assert_eq!(received_objects, created_objects);

    Ok(())
}

/// # Returns
///
/// A unique testing prefix for S3 object keys. The prefix is formatted as `test-{job_id}`.
fn get_testing_prefix_as_non_empty_string(job_id: &Uuid) -> NonEmptyString {
    NonEmptyString::from_string(format!("test-{job_id}"))
}

#[tokio::test]
#[serial_test::serial]
#[ignore = "Requires LocalStack or AWS environment"]
async fn test_sqs_listener() -> Result<()> {
    let aws_config = AwsConfig::from_env()?;
    let job_id = Uuid::new_v4();
    let prefix = get_testing_prefix_as_non_empty_string(&job_id);

    let num_tasks_to_test = vec![1, 4, 16, 32];

    for num_tasks in num_tasks_to_test {
        // NOTE: There is only one SQS queue for testing, so we need to make sure test cases are
        // running sequentially.
        run_sqs_listener_test(
            job_id,
            prefix.clone(),
            aws_config.clone(),
            SqsListenerConfig {
                queue_url: NonEmptyString::from_string(format!(
                    "{}/{}/{}",
                    aws_config.endpoint,
                    aws_config.account_id.as_str(),
                    aws_config.queue_name.as_str()
                )),
                num_concurrent_listener_tasks: num_tasks,
                wait_time_sec: 0,
                base: BaseConfig {
                    region: Some(aws_config.region.clone()),
                    bucket_name: aws_config.bucket_name.clone(),
                    key_prefix: prefix.clone(),
                    endpoint_url: Some(aws_config.endpoint.clone()),
                    dataset: None,
                    timestamp_key: None,
                    unstructured: false,
                },
            },
        )
        .await
        .unwrap_or_else(|_| panic!("SQS listener test failed. Num tasks: {num_tasks}"));
    }

    Ok(())
}

#[tokio::test]
#[serial_test::serial]
#[ignore = "Requires LocalStack or AWS environment"]
async fn test_s3_scanner() -> Result<()> {
    let job_id = Uuid::new_v4();
    let prefix = get_testing_prefix_as_non_empty_string(&job_id);

    let aws_config = AwsConfig::from_env()?;

    let s3_client = clp_rust_utils::s3::create_new_client(
        aws_config.access_key_id.as_str(),
        aws_config.secret_access_key.as_str(),
        aws_config.region.as_str(),
        Some(&aws_config.endpoint),
    )
    .await;

    let s3_scanner_config = S3ScannerConfig {
        base: BaseConfig {
            region: Some(aws_config.region.clone()),
            bucket_name: aws_config.bucket_name.clone(),
            key_prefix: prefix.clone(),
            endpoint_url: Some(aws_config.endpoint.clone()),
            dataset: None,
            timestamp_key: None,
            unstructured: false,
        },
        scanning_interval_sec: 1,
        start_after: None,
    };

    let (sender, receiver) = mpsc::channel::<ObjectMetadata>(TEST_CHANNEL_CAPACITY);

    let s3_scanner = log_ingestor::ingestion_job::S3Scanner::spawn(
        job_id,
        S3ClientWrapper::from(s3_client.clone()),
        s3_scanner_config,
        sender,
    );

    let upload_and_receive_handle = tokio::spawn(upload_and_receive(
        s3_client.clone(),
        aws_config.bucket_name.clone(),
        prefix.clone(),
        NUM_TEST_OBJECTS,
        receiver,
    ));
    let noise_upload_handle = tokio::spawn(upload_noise_objects(
        s3_client.clone(),
        aws_config.bucket_name.clone(),
        NUM_TEST_OBJECTS,
    ));

    noise_upload_handle
        .await
        .context("Error while awaiting noise upload")?;
    let (mut created_objects, mut received_objects) = upload_and_receive_handle
        .await
        .context("Error while awaiting upload and receive")?;
    s3_scanner.shutdown_and_join().await?;

    created_objects.sort();
    received_objects.sort();
    assert_eq!(received_objects, created_objects);

    Ok(())
}
