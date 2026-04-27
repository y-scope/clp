mod aws_config;
mod test_utils;

use std::{sync::Arc, time::Duration};

use anyhow::{Context, Result};
use async_trait::async_trait;
use aws_config::AwsConfig;
use clp_rust_utils::{
    clp_config::{AwsAuthentication, AwsCredentials},
    job_config::ingestion::s3::{
        BaseConfig,
        BufferConfig,
        S3ScannerConfig,
        SqsListenerConfig,
        ValidatedSqsListenerConfig,
    },
    s3::ObjectMetadata,
    types::non_empty_string::ExpectedNonEmpty,
};
use log_ingestor::{
    aws_client_manager::{S3ClientWrapper, SqsClientWrapper},
    ingestion_job::{
        IngestionJobId,
        IngestionJobState,
        S3Scanner,
        S3ScannerState,
        SqsListener,
        SqsListenerState,
    },
};
use non_empty_string::NonEmptyString;
use test_utils::{get_testing_prefix_as_non_empty_string, upload_test_objects};
use tokio::sync::Mutex;
use uuid::Uuid;

const WAIT_FOR_INGESTED_OBJECTS_TIMEOUT_SEC: u64 = 30;
const INGESTED_OBJECT_POLL_INTERVAL_MS: u64 = 100;
const NUM_TEST_OBJECTS: usize = 100;
const NUM_NOISE_OBJECTS: usize = 100;

#[derive(Clone, Default)]
struct SqsListenerTestState {
    shared_ingested_buffer: Arc<Mutex<Vec<ObjectMetadata>>>,
}

impl SqsListenerTestState {
    fn new() -> Self {
        Self::default()
    }

    fn get_shared_buffer(&self) -> Arc<Mutex<Vec<ObjectMetadata>>> {
        self.shared_ingested_buffer.clone()
    }
}

#[async_trait]
impl IngestionJobState for SqsListenerTestState {
    async fn start(&self) -> Result<()> {
        Ok(())
    }

    async fn end(&self) -> Result<()> {
        Ok(())
    }

    async fn fail(&self, msg: String) {
        panic!("SqsListenerTestState::fail should be unreachable: {msg}");
    }
}

#[async_trait]
impl SqsListenerState for SqsListenerTestState {
    async fn ingest(&self, objects: Vec<ObjectMetadata>) -> Result<()> {
        self.shared_ingested_buffer.lock().await.extend(objects);
        Ok(())
    }
}

#[derive(Clone, Default)]
struct S3ScannerTestState {
    shared_ingested_buffer: Arc<Mutex<Vec<ObjectMetadata>>>,
}

impl S3ScannerTestState {
    fn new() -> Self {
        Self::default()
    }

    fn get_shared_buffer(&self) -> Arc<Mutex<Vec<ObjectMetadata>>> {
        self.shared_ingested_buffer.clone()
    }
}

#[async_trait]
impl IngestionJobState for S3ScannerTestState {
    async fn start(&self) -> Result<()> {
        Ok(())
    }

    async fn end(&self) -> Result<()> {
        Ok(())
    }

    async fn fail(&self, msg: String) {
        panic!("S3ScannerTestState::fail should be unreachable: {msg}");
    }
}

#[async_trait]
impl S3ScannerState for S3ScannerTestState {
    async fn ingest(
        &self,
        objects: Vec<ObjectMetadata>,
        _last_ingested_key: &str,
    ) -> anyhow::Result<()> {
        self.shared_ingested_buffer.lock().await.extend(objects);
        Ok(())
    }
}

/// Uploads noise S3 objects that do not match any testing prefix.
///
/// The keys are formatted as `{uuid}.log`, where `uuid` is a randomly generated v4 UUID.
///
/// # Errors
///
/// * Forwards [`test_utils::create_s3_objects`]'s return values on failure.
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

    test_utils::create_s3_objects(s3_client, objects_to_create).await
}

/// Waits until the shared buffer has at least `min_num_objects`.
///
/// # Returns
///
/// A vector of ingested S3 object metadata on success.
async fn wait_for_ingested_objects(
    shared_buffer: Arc<Mutex<Vec<ObjectMetadata>>>,
    min_num_objects: usize,
) -> Vec<ObjectMetadata> {
    tokio::time::timeout(
        Duration::from_secs(WAIT_FOR_INGESTED_OBJECTS_TIMEOUT_SEC),
        async {
            loop {
                let ingested_objects = shared_buffer.lock().await;
                if ingested_objects.len() >= min_num_objects {
                    return ingested_objects.clone();
                }
                drop(ingested_objects);
                tokio::time::sleep(Duration::from_millis(INGESTED_OBJECT_POLL_INTERVAL_MS)).await;
            }
        },
    )
    .await
    .expect("Timed out while waiting for ingested objects")
}

/// Runs SQS listener test with the given job config.
async fn run_sqs_listener_test(
    job_id: IngestionJobId,
    prefix: NonEmptyString,
    aws_config: AwsConfig,
    sqs_listener_config: SqsListenerConfig,
) -> Result<()> {
    let aws_auth = AwsAuthentication::Credentials {
        credentials: AwsCredentials {
            access_key_id: aws_config.access_key_id.clone(),
            secret_access_key: aws_config.secret_access_key.clone(),
        },
    };
    let sqs_client = clp_rust_utils::sqs::create_new_client(
        aws_config.region.as_str(),
        Some(&aws_config.endpoint),
        &aws_auth,
    )
    .await;

    let state = SqsListenerTestState::new();
    let shared_buffer = state.get_shared_buffer();

    let sqs_listener = SqsListener::spawn(
        job_id,
        &SqsClientWrapper::from(sqs_client),
        &ValidatedSqsListenerConfig::validate_and_create(sqs_listener_config)
            .expect("invalid SQS listener config"),
        state,
    );

    let s3_client = clp_rust_utils::s3::create_new_client(
        aws_config.region.as_str(),
        Some(&aws_config.endpoint),
        &aws_auth,
    )
    .await;

    let upload_handle = tokio::spawn(upload_test_objects(
        s3_client.clone(),
        aws_config.bucket_name.clone(),
        prefix.clone(),
        NUM_TEST_OBJECTS,
    ));
    let noise_upload_handle = tokio::spawn(upload_noise_objects(
        s3_client.clone(),
        aws_config.bucket_name.clone(),
        NUM_NOISE_OBJECTS,
    ));

    noise_upload_handle
        .await
        .context("Error while awaiting noise upload")??;
    let mut created_objects = upload_handle
        .await
        .context("Error while awaiting test object upload")??;
    let mut received_objects =
        wait_for_ingested_objects(shared_buffer, created_objects.len()).await;

    sqs_listener.shutdown_and_join().await;

    created_objects.sort();
    received_objects.sort();
    assert_eq!(received_objects, created_objects);

    Ok(())
}

#[tokio::test]
#[serial_test::serial]
#[ignore = "Requires LocalStack or AWS environment"]
async fn test_sqs_listener() -> Result<()> {
    let aws_config = AwsConfig::from_env()?;
    let job_id = Uuid::new_v4().as_u64_pair().0;
    let prefix = get_testing_prefix_as_non_empty_string(job_id);

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
                    buffer_config: BufferConfig::default(),
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
    let job_id = Uuid::new_v4().as_u64_pair().0;
    let prefix = get_testing_prefix_as_non_empty_string(job_id);

    let aws_config = AwsConfig::from_env()?;

    let aws_auth = AwsAuthentication::Credentials {
        credentials: AwsCredentials {
            access_key_id: aws_config.access_key_id.clone(),
            secret_access_key: aws_config.secret_access_key.clone(),
        },
    };
    let s3_client = clp_rust_utils::s3::create_new_client(
        aws_config.region.as_str(),
        Some(&aws_config.endpoint),
        &aws_auth,
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
            buffer_config: BufferConfig::default(),
        },
        scanning_interval_sec: 1,
        start_after: None,
    };

    let state = S3ScannerTestState::new();
    let shared_buffer = state.get_shared_buffer();

    let s3_scanner = S3Scanner::spawn(
        job_id,
        S3ClientWrapper::from(s3_client.clone()),
        s3_scanner_config,
        state,
    );

    let test_upload_handle = tokio::spawn(upload_test_objects(
        s3_client.clone(),
        aws_config.bucket_name.clone(),
        prefix.clone(),
        NUM_TEST_OBJECTS,
    ));
    let noise_upload_handle = tokio::spawn(upload_noise_objects(
        s3_client.clone(),
        aws_config.bucket_name.clone(),
        NUM_NOISE_OBJECTS,
    ));

    noise_upload_handle
        .await
        .context("Error while awaiting noise upload")??;
    let mut created_objects = test_upload_handle
        .await
        .context("Error while awaiting test object upload")??;
    let mut received_objects =
        wait_for_ingested_objects(shared_buffer, created_objects.len()).await;
    s3_scanner.shutdown_and_join().await;

    created_objects.sort();
    received_objects.sort();
    assert_eq!(received_objects, created_objects);

    Ok(())
}
