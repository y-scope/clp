mod aws_config;

use std::time::Duration;

use anyhow::Result;
use aws_config::AwsConfig;
use clp_rust_utils::s3::ObjectMetadata;
use log_ingestor::{
    aws_client_manager::SqsClientWrapper,
    ingestion_job::{SqsListener, SqsListenerConfig},
};
use tokio::sync::mpsc;

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
        let body = vec![b'0'; object.size];

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

#[tokio::test]
#[serial_test::serial]
#[ignore = "Requires LocalStack or AWS environment"]
async fn test_sqs_listener() -> Result<()> {
    const NUM_TEST_OBJECTS: usize = 100;
    const RECEIVER_TIMEOUT_SEC: u64 = 30;
    const TEST_CHANNEL_CAPACITY: usize = 10;

    let aws_config = AwsConfig::from_env()?;

    let secret_access_key =
        secrecy::SecretString::new(aws_config.secret_access_key.clone().into_boxed_str());

    let s3_client = clp_rust_utils::s3::create_new_client(
        aws_config.endpoint.as_str(),
        aws_config.region.as_str(),
        aws_config.access_key_id.as_str(),
        &secret_access_key,
    )
    .await;

    let sqs_client = clp_rust_utils::sqs::create_new_client(
        aws_config.endpoint.as_str(),
        aws_config.region.as_str(),
        aws_config.access_key_id.as_str(),
        &secret_access_key,
    )
    .await;

    let job_id = uuid::Uuid::new_v4();
    let prefix = format!("test-{job_id}/");

    let mut objects_to_create = Vec::new();
    let mut irrelevant_objects = Vec::new();
    for i in 0..NUM_TEST_OBJECTS {
        let object_metadata = ObjectMetadata {
            bucket: aws_config.bucket_name.clone(),
            key: format!("{prefix}{i}.log"),
            size: 16,
        };
        objects_to_create.push(object_metadata);

        let irrelevant_object_metadata = ObjectMetadata {
            bucket: aws_config.bucket_name.clone(),
            key: format!("irrelevant-{prefix}-{i}.log"),
            size: 16,
        };
        irrelevant_objects.push(irrelevant_object_metadata);
    }

    // Spawn a task to PUT new S3 objects
    let _ = tokio::spawn(create_s3_objects(s3_client.clone(), objects_to_create.clone()));
    let _ = tokio::spawn(create_s3_objects(s3_client.clone(), irrelevant_objects));

    // Spawn the SQS listener
    let sqs_listener_config = SqsListenerConfig {
        queue_url: format!(
            "{}/{}/{}",
            aws_config.endpoint.as_str(),
            aws_config.account_id.as_str(),
            aws_config.queue_name.as_str()
        ),
        bucket_name: aws_config.bucket_name.clone(),
        prefix: prefix.clone(),
        max_num_messages_to_fetch: 2,
        init_polling_backoff_sec: 1,
        max_polling_backoff_sec: 1,
    };

    let (sender, receiver) = mpsc::channel::<ObjectMetadata>(TEST_CHANNEL_CAPACITY);
    let sqs_listener = SqsListener::spawn(
        job_id,
        SqsClientWrapper::from(sqs_client),
        sqs_listener_config,
        sender,
    );

    // Join all tasks
    let mut received_objects = tokio::time::timeout(
        Duration::from_secs(RECEIVER_TIMEOUT_SEC),
        receive_object_metadata(receiver, objects_to_create.len()),
    )
    .await
    .expect("Timed out while receiving object metadata");
    sqs_listener.shutdown_and_join().await?;

    objects_to_create.sort();
    received_objects.sort();
    assert_eq!(received_objects, objects_to_create);

    Ok(())
}
