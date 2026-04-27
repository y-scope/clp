mod aws_config;
mod test_utils;

use std::sync::{
    Arc,
    atomic::{AtomicUsize, Ordering},
};

use anyhow::Result;
use aws_config::AwsConfig;
use clp_rust_utils::{
    clp_config::{AwsAuthentication, AwsCredentials},
    s3::ObjectMetadata,
    types::non_empty_string::ExpectedNonEmpty,
};
use log_ingestor::ingestion_job::scan_prefix;
use non_empty_string::NonEmptyString;
use test_utils::{create_s3_objects, get_testing_prefix_as_non_empty_string, upload_test_objects};
use tokio::sync::Mutex;
use uuid::Uuid;

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
async fn test_scan_prefix_across_multiple_pages() -> Result<()> {
    let aws_config = AwsConfig::from_env()?;
    let s3_client = create_test_s3_client(&aws_config).await;

    let job_id = Uuid::new_v4().as_u64_pair().0;
    let prefix = get_testing_prefix_as_non_empty_string(job_id);
    let mut created_objects = upload_test_objects(
        s3_client.clone(),
        aws_config.bucket_name.clone(),
        prefix.clone(),
        NUM_MULTI_PAGE_OBJECTS,
    )
    .await?;

    let page_count = Arc::new(AtomicUsize::new(0));
    let scanned_objects = Arc::new(Mutex::new(Vec::<ObjectMetadata>::new()));
    let last_scanned_key = scan_prefix(&s3_client, &aws_config.bucket_name, &prefix, None, {
        let page_count = page_count.clone();
        let scanned_objects = scanned_objects.clone();
        move |objects| {
            let page_count = page_count.clone();
            let scanned_objects = scanned_objects.clone();
            async move {
                page_count.fetch_add(1, Ordering::Relaxed);
                scanned_objects.lock().await.extend(objects);
                Ok(())
            }
        }
    })
    .await?;

    let mut received_objects = scanned_objects.lock().await.clone();
    created_objects.sort();
    received_objects.sort();

    assert!(page_count.load(Ordering::Relaxed) > 1);
    assert_eq!(received_objects, created_objects);
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
    let last_scanned_key = scan_prefix(&s3_client, &aws_config.bucket_name, &prefix, None, {
        let page_count = page_count.clone();
        let scanned_objects = scanned_objects.clone();
        move |objects| {
            let page_count = page_count.clone();
            let scanned_objects = scanned_objects.clone();
            async move {
                page_count.fetch_add(1, Ordering::Relaxed);
                scanned_objects.lock().await.extend(objects);
                Ok(())
            }
        }
    })
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
