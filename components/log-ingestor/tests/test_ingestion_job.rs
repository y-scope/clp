use anyhow::Result;
use clp_rust_utils::s3::ObjectMetadata;
use log_ingestor::ingestion_job::{SqsListener, SqsListenerConfig};
use tokio::sync::mpsc;

struct AwsConfig {
    pub endpoint: String,
    pub access_key_id: String,
    pub secret_access_key: String,
    pub region: String,
    pub account_id: String,
    pub bucket_name: String,
    pub queue_name: String,
}

/// Default AWS configuration for local testing with `LocalStack`.
const DEFAULT_AWS_ENDPOINT: &str = "http://127.0.0.1:4566";
const DEFAULT_AWS_ACCESS_KEY_ID: &str = "test";
const DEFAULT_AWS_SECRET_ACCESS_KEY: &str = "test";
const DEFAULT_AWS_ACCOUNT_ID: &str = "000000000000";
const DEFAULT_AWS_REGION: &str = "us-east-1";

const TEST_CHANNEL_CAPACITY: usize = 100;

impl AwsConfig {
    fn from_env() -> Option<Self> {
        let endpoint =
            std::env::var("AWS_ENDPOINT").unwrap_or_else(|_| DEFAULT_AWS_ENDPOINT.to_string());
        let access_key_id = std::env::var("AWS_ACCESS_KEY_ID")
            .unwrap_or_else(|_| DEFAULT_AWS_ACCESS_KEY_ID.to_string());
        let secret_access_key = std::env::var("AWS_SECRET_ACCESS_KEY")
            .unwrap_or_else(|_| DEFAULT_AWS_SECRET_ACCESS_KEY.to_string());
        let region = std::env::var("AWS_REGION").unwrap_or_else(|_| DEFAULT_AWS_REGION.to_string());
        let account_id =
            std::env::var("AWS_ACCOUNT_ID").unwrap_or_else(|_| DEFAULT_AWS_ACCOUNT_ID.to_string());

        let Ok(bucket_name) = std::env::var("CLP_LOG_INGESTOR_S3_BUCKET") else {
            return None;
        };

        let Ok(queue_name) = std::env::var("CLP_LOG_INGESTOR_SQS_QUEUE") else {
            return None;
        };

        Some(Self {
            endpoint,
            access_key_id,
            secret_access_key,
            region,
            account_id,
            bucket_name,
            queue_name,
        })
    }
}

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

async fn listen_for_object_metadata(
    mut receiver: mpsc::Receiver<ObjectMetadata>,
    expected_num_objects: usize,
    timeout_sec: u64,
) -> Result<Vec<ObjectMetadata>> {
    let mut received_objects = Vec::new();
    let timeout_duration = tokio::time::Duration::from_secs(timeout_sec);
    let start_time = tokio::time::Instant::now();

    while received_objects.len() < expected_num_objects {
        let elapsed = tokio::time::Instant::now().duration_since(start_time);
        if elapsed >= timeout_duration {
            break;
        }

        match tokio::time::timeout(
            timeout_duration.checked_sub(elapsed).unwrap(),
            receiver.recv(),
        )
        .await
        {
            Ok(Some(object_metadata)) => {
                received_objects.push(object_metadata);
            }
            Ok(None) | Err(_) => {
                break;
            }
        }
    }

    Ok(received_objects)
}

#[tokio::test]
async fn test_sqs_listener() -> Result<()> {
    const NUM_TEST_OBJECTS: usize = 5;
    const RECEIVER_TIMEOUT_SEC: u64 = 15;

    let Some(aws_config) = AwsConfig::from_env() else {
        eprintln!(
            "Skipping `{}`: AWS configuration environment variables are not set.",
            stdext::function_name!()
        );
        return Ok(());
    };

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
    let sqs_listener = SqsListener::spawn(job_id, sqs_client, sqs_listener_config, sender);

    // Spawn a task to listen for ingested object metadata
    let receiver_handle = tokio::spawn(listen_for_object_metadata(
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
