use std::{
    collections::HashSet,
    sync::{
        Arc,
        atomic::{AtomicUsize, Ordering},
    },
};

use anyhow::Result;
use clp_rust_utils::{
    clp_config::{AwsAuthentication, AwsCredentials},
    s3::ObjectMetadata,
    types::non_empty_string::ExpectedNonEmpty,
};
use log_ingestor::ingestion_job::scan_prefix;
use non_empty_string::NonEmptyString;
use tokio::sync::Mutex;
use uuid::Uuid;

use super::aws_config::AwsConfig;
use super::test_utils::{
    create_s3_objects,
    get_testing_prefix_as_non_empty_string,
    upload_test_objects,
};

const NUM_OBJECTS: usize = 250;
// ListObjectsV2 returns at most 1,000 objects from a bucket
const NUM_MULTI_PAGE_OBJECTS: usize = 1_205;
const NUM_FILTERED_OUT_DIRECTORY_MARKERS: usize = 25;

async fn create_test_s3_client(aws_config: &AwsConfig) -> aws_sdk_s3::Client {
    let aws_auth = AwsAuthentication::Credentials {
        credentials: AwsCredentials {
            access_key_id: aws_config.access_key_id.clone(),
            secret_access_key: aws_config.secret_access_key.clone(),
        },
    };

    clp_rust_utils::s3::create_new_client(
        aws_config.region.as_str(),
        Some(&aws_config.endpoint),
        &aws_auth,
    )
    .await
}

#[tokio::test]
#[serial_test::serial]
#[ignore = "Requires LocalStack or AWS environment"]
async fn test_scan_prefix_early_exit() -> Result<()> {
    let aws_config = AwsConfig::from_env()?;
    let s3_client = create_test_s3_client(&aws_config).await;

    let job_id = Uuid::new_v4().as_u64_pair().0;
    let prefix = get_testing_prefix_as_non_empty_string(job_id);
    let created_objects = upload_test_objects(
        s3_client.clone(),
        aws_config.bucket_name.clone(),
        prefix.clone(),
        NUM_OBJECTS,
    )
    .await?;

    let requested_keys: HashSet<NonEmptyString> = [10_usize, 100, 200]
        .into_iter()
        .map(|idx| created_objects[idx].key.clone())
        .collect();

    let remaining_keys = Arc::new(Mutex::new(requested_keys.clone()));
    let found_objects = Arc::new(Mutex::new(Vec::<ObjectMetadata>::new()));
    let remaining_keys_for_scan = remaining_keys.clone();
    let found_objects_for_scan = found_objects.clone();

    let last_scanned_key: Option<NonEmptyString> = scan_prefix(
        &s3_client,
        &aws_config.bucket_name,
        &prefix,
        None::<NonEmptyString>,
        async move |objects: Vec<ObjectMetadata>| -> Result<(bool, NonEmptyString)> {
            let page_last_key = objects
                .last()
                .expect("`objects` should not be empty")
                .key
                .clone();

            let mut remaining_keys = remaining_keys_for_scan.lock().await;
            let mut matched_objects = Vec::new();
            for object in objects {
                if remaining_keys.remove(&object.key) {
                    matched_objects.push(object);
                }
            }
            let should_continue_scanning = !remaining_keys.is_empty();
            drop(remaining_keys);
            found_objects_for_scan.lock().await.extend(matched_objects);

            Ok((should_continue_scanning, page_last_key))
        },
    )
    .await?;

    let remaining_keys = remaining_keys.lock().await.clone();
    let mut received_requested_keys: Vec<_> = found_objects
        .lock()
        .await
        .iter()
        .map(|object| object.key.clone())
        .collect();
    let mut expected_requested_keys: Vec<_> = requested_keys.iter().cloned().collect();
    received_requested_keys.sort();
    expected_requested_keys.sort();

    assert!(remaining_keys.is_empty());
    assert_eq!(received_requested_keys, expected_requested_keys);
    assert_eq!(
        last_scanned_key,
        created_objects.last().map(|obj| obj.key.clone())
    );

    Ok(())
}

#[tokio::test]
#[serial_test::serial]
#[ignore = "Requires LocalStack or AWS environment"]
async fn test_scan_prefix_multi_page_filtering() -> Result<()> {
    let aws_config = AwsConfig::from_env()?;
    let s3_client = create_test_s3_client(&aws_config).await;

    let job_id = Uuid::new_v4().as_u64_pair().0;
    let prefix = get_testing_prefix_as_non_empty_string(job_id);

    let mut expected_objects = upload_test_objects(
        s3_client.clone(),
        aws_config.bucket_name.clone(),
        prefix.clone(),
        NUM_MULTI_PAGE_OBJECTS,
    )
    .await?;

    let filtered_out_objects = (0..NUM_FILTERED_OUT_DIRECTORY_MARKERS)
        .map(|idx| ObjectMetadata {
            bucket: aws_config.bucket_name.clone(),
            key: NonEmptyString::from_string(format!("{prefix}/dir-{idx:05}/")),
            size: 16,
        })
        .collect();
    create_s3_objects(s3_client.clone(), filtered_out_objects).await?;

    let page_count = Arc::new(AtomicUsize::new(0));
    let scanned_objects = Arc::new(Mutex::new(Vec::<ObjectMetadata>::new()));
    let page_count_for_scan = page_count.clone();
    let scanned_objects_for_scan = scanned_objects.clone();
    let last_scanned_key: Option<NonEmptyString> = scan_prefix(
        &s3_client,
        &aws_config.bucket_name,
        &prefix,
        None::<NonEmptyString>,
        async move |objects: Vec<ObjectMetadata>| -> Result<(bool, NonEmptyString)> {
            page_count_for_scan.fetch_add(1, Ordering::Relaxed);
            let last_key = objects
                .last()
                .expect("`objects` should not be empty")
                .key
                .clone();
            scanned_objects_for_scan.lock().await.extend(objects);
            Ok((true, last_key))
        },
    )
    .await?;

    let mut received_objects = scanned_objects.lock().await.clone();
    expected_objects.sort();
    received_objects.sort();

    assert!(page_count.load(Ordering::Relaxed) > 1);
    assert_eq!(received_objects, expected_objects);
    assert_eq!(
        last_scanned_key,
        expected_objects.last().map(|obj| obj.key.clone())
    );
    assert!(
        received_objects
            .iter()
            .all(|obj| !obj.key.as_str().ends_with('/'))
    );

    Ok(())
}
