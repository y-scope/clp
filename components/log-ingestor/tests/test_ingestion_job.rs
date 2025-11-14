mod aws_config;

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

/// Receives object metadata from the given receiver channel until either:
///
/// * the maximum number of objects is reached or,
/// * the timeout is triggered.
///
/// # Returns
///
/// A vector of received S3 object metadata on success.
///
/// # Errors
///
/// * Forwards [`mpsc::Receiver::recv`]'s return values on failure.
async fn receive_object_metadata(
    mut receiver: mpsc::Receiver<ObjectMetadata>,
    max_num_objects: usize,
    timeout_sec: u64,
) -> Result<Vec<ObjectMetadata>> {
    let mut received_objects = Vec::new();
    let timeout_duration = tokio::time::Duration::from_secs(timeout_sec);
    let start_time = tokio::time::Instant::now();

    while received_objects.len() < max_num_objects {
        let elapsed = tokio::time::Instant::now().duration_since(start_time);
        if elapsed >= timeout_duration {
            break;
        }

        match tokio::time::timeout(
            timeout_duration.checked_sub(elapsed).unwrap(),
            receiver.recv(),
        )
        .await?
        {
            Some(object_metadata) => {
                received_objects.push(object_metadata);
            }
            None => {
                break;
            }
        }
    }

    Ok(received_objects)
}

#[tokio::test]
#[serial_test::serial]
#[ignore = "Requires LocalStack or AWS environment"]
async fn test_sqs_listener() -> Result<()> {
    const NUM_TEST_OBJECTS: usize = 100;
    const RECEIVER_TIMEOUT_SEC: u64 = 30;
    const TEST_CHANNEL_CAPACITY: usize = 10;

    let aws_config = AwsConfig::from_env().unwrap_or_else(|| {
        panic!(
            "{}: Required AWS configuration environment variables are not set.",
            stdext::function_name!()
        )
    });

    let s3_client = clp_rust_utils::s3::create_new_client(
        aws_config.endpoint.as_str(),
        aws_config.region.as_str(),
        aws_config.access_key_id.as_str(),
        &secrecy::SecretString::new(aws_config.secret_access_key.clone().into_boxed_str()),
    )
    .await;

    let sqs_client = clp_rust_utils::sqs::create_new_client(
        aws_config.endpoint.as_str(),
        aws_config.region.as_str(),
        aws_config.access_key_id.as_str(),
        &secrecy::SecretString::new(aws_config.secret_access_key.clone().into_boxed_str()),
    )
    .await;

    let job_id = uuid::Uuid::new_v4();
    let prefix = format!("test-{job_id}/");

    let mut objects_to_create = Vec::new();
    for i in 0..NUM_TEST_OBJECTS {
        let object_metadata = ObjectMetadata {
            bucket: aws_config.bucket_name.clone(),
            key: format!("{prefix}{i}.log"),
            size: 16,
        };
        objects_to_create.push(object_metadata);
    }

    // Spawn a task to PUT new S3 objects
    let creation_handle = tokio::spawn(create_s3_objects(s3_client, objects_to_create.clone()));

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

    // Spawn a task to listen for ingested object metadata
    let receiver_handle = tokio::spawn(receive_object_metadata(
        receiver,
        objects_to_create.len(),
        RECEIVER_TIMEOUT_SEC,
    ));

    // Join all tasks
    creation_handle.await??;
    let mut received_objects = receiver_handle.await??;
    sqs_listener.shutdown_and_join().await?;

    objects_to_create.sort();
    received_objects.sort();
    assert_eq!(received_objects, objects_to_create);

    Ok(())
}
