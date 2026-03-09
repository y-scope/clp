use std::{sync::Arc, time::Duration};

use anyhow::Result;
use async_trait::async_trait;
use clp_rust_utils::s3::S3ObjectMetadataId;
use log_ingestor::compression::{
    Buffer,
    BufferSubmitter,
    CompressionBufferEntry,
    DEFAULT_LISTENER_CAPACITY,
    Listener,
};
use tokio::sync::{Mutex, mpsc};

const TEST_OBJECT_SIZE: u64 = 1024;

/// A test submitter that stores submitted buffers in memory for inspection.
struct TestBufferSubmitter {
    store: Arc<Mutex<Vec<Vec<S3ObjectMetadataId>>>>,
}

impl TestBufferSubmitter {
    fn new() -> Self {
        Self {
            store: Arc::new(Mutex::new(Vec::new())),
        }
    }

    fn shared_store(&self) -> Arc<Mutex<Vec<Vec<S3ObjectMetadataId>>>> {
        self.store.clone()
    }
}

#[async_trait]
impl BufferSubmitter for TestBufferSubmitter {
    async fn submit(&self, buffer: &[S3ObjectMetadataId]) -> Result<()> {
        self.store.lock().await.push(buffer.to_vec());
        Ok(())
    }
}

/// Sends [`CompressionBufferEntry`] values to the listener via the provided sender.
async fn send_to_listener(
    refs: Vec<CompressionBufferEntry>,
    sender: mpsc::Sender<Vec<CompressionBufferEntry>>,
) {
    sender
        .send(refs)
        .await
        .expect("Failed to send refs to listener");
}

/// Creates [`CompressionBufferEntry`] values for testing. IDs start at `id_start`.
/// [`TEST_OBJECT_SIZE`].
///
/// # Returns
///
/// A vector of [`CompressionBufferEntry`] for testing.
fn create_test_refs(id_start: S3ObjectMetadataId, count: usize) -> Vec<CompressionBufferEntry> {
    (id_start..id_start + count as S3ObjectMetadataId)
        .map(|id| CompressionBufferEntry {
            id,
            size: TEST_OBJECT_SIZE,
        })
        .collect()
}

#[tokio::test]
async fn test_compression_listener() -> Result<()> {
    const DEFAULT_TIMEOUT_SECONDS: u64 = 4;

    let submitter = TestBufferSubmitter::new();
    let shared = submitter.shared_store();

    let buffer: Buffer<TestBufferSubmitter> = Buffer::new(submitter, 120 * TEST_OBJECT_SIZE);
    let listener = Listener::spawn(
        buffer,
        Duration::from_secs(DEFAULT_TIMEOUT_SECONDS),
        DEFAULT_LISTENER_CAPACITY,
    );

    let refs1 = create_test_refs(1, 100);
    let refs2 = create_test_refs(101, 100);
    let refs3 = create_test_refs(201, 100);

    // Spawn three tasks that send into the listener concurrently
    let sender1 = listener.get_new_sender();
    let sender2 = listener.get_new_sender();
    let sender3 = listener.get_new_sender();

    let h1 = tokio::spawn(async move { send_to_listener(refs1, sender1).await });
    let h2 = tokio::spawn(async move { send_to_listener(refs2, sender2).await });
    let h3 = tokio::spawn(async move { send_to_listener(refs3, sender3).await });

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

    let mut actual_ids: Vec<S3ObjectMetadataId> =
        submitted_buffers.iter().flatten().copied().collect();
    actual_ids.sort_unstable();

    let expected_ids: Vec<S3ObjectMetadataId> = (1..=300).collect();
    assert_eq!(expected_ids, actual_ids);

    submitted_buffers.clear();
    drop(submitted_buffers);

    // Clean up
    listener.shutdown_and_join().await;
    assert!(shared.lock().await.is_empty());

    Ok(())
}
