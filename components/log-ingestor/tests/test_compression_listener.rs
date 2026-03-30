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
    buffer_entries: Vec<CompressionBufferEntry>,
    sender: mpsc::Sender<Vec<CompressionBufferEntry>>,
) {
    sender
        .send(buffer_entries)
        .await
        .expect("Failed to send buffer entries to listener");
}

/// # Returns
///
/// A vector of [`CompressionBufferEntry`] for testing.
fn create_test_buffer_entries(ids: &[S3ObjectMetadataId]) -> Vec<CompressionBufferEntry> {
    ids.iter()
        .map(|id| CompressionBufferEntry {
            id: *id,
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

    let expected_ids: Vec<S3ObjectMetadataId> = (1..=300).collect();
    let mut handlers = Vec::new();
    for ids in expected_ids.chunks(100) {
        let entries = create_test_buffer_entries(ids);
        let sender = listener.get_new_sender();
        let h = tokio::spawn(async move { send_to_listener(entries, sender).await });
        handlers.push(h);
    }
    // Wait for all sender tasks to finish
    for handler in handlers {
        handler.await.expect("sender task panicked");
    }

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

    assert_eq!(expected_ids, actual_ids);

    submitted_buffers.clear();
    drop(submitted_buffers);

    // Clean up
    listener.shutdown_and_join().await;
    assert!(shared.lock().await.is_empty());

    Ok(())
}
