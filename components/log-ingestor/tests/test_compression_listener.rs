use std::{sync::Arc, time::Duration};

use anyhow::Result;
use async_trait::async_trait;
use clp_rust_utils::{s3::ObjectMetadata, types::non_empty_string::ExpectedNonEmpty};
use log_ingestor::compression::{Buffer, BufferSubmitter, DEFAULT_LISTENER_CAPACITY, Listener};
use non_empty_string::NonEmptyString;
use tokio::sync::{Mutex, mpsc};

const TEST_OBJECT_SIZE: u64 = 1024;

/// A test submitter that stores submitted buffers in memory for inspection.
struct TestBufferSubmitter {
    store: Arc<Mutex<Vec<Vec<ObjectMetadata>>>>,
}

impl TestBufferSubmitter {
    fn new() -> Self {
        Self {
            store: Arc::new(Mutex::new(Vec::new())),
        }
    }

    fn shared_store(&self) -> Arc<Mutex<Vec<Vec<ObjectMetadata>>>> {
        self.store.clone()
    }
}

#[async_trait]
impl BufferSubmitter for TestBufferSubmitter {
    async fn submit(&self, buffer: &[ObjectMetadata]) -> Result<()> {
        let store = self.store.clone();
        let _submitted_results = store.lock().await.push(buffer.to_vec());
        Ok(())
    }
}

/// Sends a list of objects to the listener via the provided sender.
async fn send_to_listener(objects: Vec<ObjectMetadata>, sender: mpsc::Sender<Vec<ObjectMetadata>>) {
    sender
        .send(objects)
        .await
        .expect("Failed to send objects to listener");
}

/// Creates a vector of [`ObjectMetadata`] objects for a given bucket. Each object will have a
/// unique key and a fixed size of [`TEST_OBJECT_SIZE`].
///
/// # Returns
///
/// A vector of created objects for testing.
fn create_test_objects(bucket_name: &str, count: usize) -> Vec<ObjectMetadata> {
    (0..count)
        .map(|i| ObjectMetadata {
            bucket: NonEmptyString::from_string(bucket_name.to_string()),
            key: NonEmptyString::from_string(format!("object-{i}")),
            size: TEST_OBJECT_SIZE,
            id: None,
        })
        .collect()
}

#[tokio::test]
async fn test_compression_listener() -> Result<()> {
    const DEFAULT_TIMEOUT_SECONDS: u64 = 4;
    const BUCKET_0: &str = "bucket-0";
    const BUCKET_1: &str = "bucket-1";

    let submitter = TestBufferSubmitter::new();
    let shared = submitter.shared_store();

    let buffer: Buffer<TestBufferSubmitter> = Buffer::new(submitter, 120 * TEST_OBJECT_SIZE);
    let listener = Listener::spawn(
        buffer,
        Duration::from_secs(DEFAULT_TIMEOUT_SECONDS),
        DEFAULT_LISTENER_CAPACITY,
    );

    let bucket_0_objects = create_test_objects(BUCKET_0, 100);
    let bucket_1_objects = create_test_objects(BUCKET_1, 200);

    // Spawn three tasks that send into the listener concurrently:
    // 1) send all of `bucket_0_objects`
    // 2) send the first 100 objects of `bucket_1_objects`
    // 3) send the last 100 objects of `bucket_1_objects`
    let sender1 = listener.get_new_sender();
    let sender2 = listener.get_new_sender();
    let sender3 = listener.get_new_sender();

    let objs1 = bucket_0_objects.clone();
    let objs2 = bucket_1_objects[..100].to_vec();
    let objs3 = bucket_1_objects[100..].to_vec();

    let h1 = tokio::spawn(async move { send_to_listener(objs1, sender1).await });
    let h2 = tokio::spawn(async move { send_to_listener(objs2, sender2).await });
    let h3 = tokio::spawn(async move { send_to_listener(objs3, sender3).await });

    // Wait for all sender tasks to finish
    h1.await.unwrap();
    h2.await.unwrap();
    h3.await.unwrap();

    // Sleep to trigger timeout-based submission
    tokio::time::sleep(Duration::from_secs(DEFAULT_TIMEOUT_SECONDS + 1)).await;

    // Inspect submitted results and verify total count
    let mut submitted_buffers = shared.lock().await;

    // There should be 3 submissions:
    // The first two triggered by buffer size limit, the last one by timeout.
    assert_eq!(submitted_buffers.len(), 3);

    let mut actual_total_submitted: Vec<ObjectMetadata> =
        submitted_buffers.iter().flatten().cloned().collect();
    actual_total_submitted.sort();

    let mut expected_total_submitted = Vec::new();
    expected_total_submitted.extend_from_slice(&bucket_0_objects);
    expected_total_submitted.extend_from_slice(&bucket_1_objects);
    expected_total_submitted.sort();

    assert_eq!(expected_total_submitted, actual_total_submitted);
    submitted_buffers.clear();
    drop(submitted_buffers);

    // Clean up
    listener.shutdown_and_join().await.unwrap();
    assert!(shared.lock().await.is_empty());

    Ok(())
}
