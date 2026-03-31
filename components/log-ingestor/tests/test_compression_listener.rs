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
use tokio::{
    sync::{Mutex, mpsc},
    time::Instant,
};

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
/// A vector of [`CompressionBufferEntry`] for testing, each with size [`TEST_OBJECT_SIZE`].
fn create_test_buffer_entries(ids: &[S3ObjectMetadataId]) -> Vec<CompressionBufferEntry> {
    create_test_buffer_entries_with_size(ids, TEST_OBJECT_SIZE)
}

/// # Returns
///
/// A vector of [`CompressionBufferEntry`] with a custom size for each entry.
fn create_test_buffer_entries_with_size(
    ids: &[S3ObjectMetadataId],
    size: u64,
) -> Vec<CompressionBufferEntry> {
    ids.iter()
        .map(|id| CompressionBufferEntry { id: *id, size })
        .collect()
}

/// Waits until the specified instant and asserts the number of submissions matches the expected
/// count.
async fn assert_submission_count_at(
    instant: Instant,
    store: &Arc<Mutex<Vec<Vec<S3ObjectMetadataId>>>>,
    expected: usize,
    context: &str,
) {
    tokio::time::sleep_until(instant).await;
    assert_eq!(
        store.lock().await.len(),
        expected,
        "unexpected submission count: {context}"
    );
}

#[tokio::test]
async fn test_compression_listener() -> Result<()> {
    const DEFAULT_TIMEOUT: Duration = Duration::from_millis(100);
    const SLACK: Duration = Duration::from_millis(2);

    let submitter = TestBufferSubmitter::new();
    let shared = submitter.shared_store();

    let buffer: Buffer<TestBufferSubmitter> = Buffer::new(submitter, 120 * TEST_OBJECT_SIZE);
    let listener = Listener::spawn(buffer, DEFAULT_TIMEOUT, DEFAULT_LISTENER_CAPACITY);

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
    tokio::time::sleep(DEFAULT_TIMEOUT + SLACK).await;

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

#[tokio::test]
async fn test_listener_hard_timeout() -> Result<()> {
    const TIMEOUT: Duration = Duration::from_millis(50);
    const SLACK: Duration = Duration::from_millis(2);
    const WAIT: Duration = Duration::from_millis(20);
    const SIZE_THRESHOLD: u64 = 120 * TEST_OBJECT_SIZE;

    let submitter = TestBufferSubmitter::new();
    let shared = submitter.shared_store();

    let buffer: Buffer<TestBufferSubmitter> = Buffer::new(submitter, SIZE_THRESHOLD);
    let listener = Listener::spawn(buffer, TIMEOUT, DEFAULT_LISTENER_CAPACITY);
    let sender = listener.get_new_sender();

    {
        // Phase 1: Basic hard timeout
        let t_0 = Instant::now();
        tokio::time::sleep(WAIT).await; // t_1
        send_to_listener(create_test_buffer_entries(&[1]), sender.clone()).await;
        let t_2 = Instant::now();
        assert!(t_2 - t_0 < TIMEOUT, "send took longer than timeout");

        // At t_0 + timeout, no submission should have triggered because the timer started at t_2
        // (when the object was ingested), not at t_0 (when the listener started).
        assert_submission_count_at(
            t_0 + TIMEOUT,
            &shared,
            0,
            "timer should not have fired at t_0 + timeout",
        )
        .await;

        // At t_2 + timeout, the buffer submission triggers by timeout.
        assert_submission_count_at(
            t_2 + TIMEOUT + SLACK,
            &shared,
            1,
            "timeout flush of [1] expected",
        )
        .await;
    }

    {
        // Phase 2: Size flush + leftover hard timeout
        let t_3 = Instant::now();

        // Ingest two objects. The first object's size meets the threshold, triggering a size flush.
        // The second object (small) remains in the buffer. The timer resets to t_4 because the
        // flush left the buffer non-empty.
        let mut entries = create_test_buffer_entries_with_size(&[2], SIZE_THRESHOLD);
        entries.extend(create_test_buffer_entries(&[3]));
        send_to_listener(entries, sender.clone()).await;
        // Allow the listener task to process the entries and trigger the size flush.
        tokio::time::sleep(SLACK).await;
        let t_4 = Instant::now();

        assert_eq!(shared.lock().await.len(), 2, "size flush of [2] expected");

        // At t_3 + timeout, no new submission should trigger because the timer was reset at t_4,
        // not t_3.
        assert_submission_count_at(
            t_3 + TIMEOUT,
            &shared,
            2,
            "timer should not have fired at t_3 + timeout",
        )
        .await;

        // At t_4 + timeout, the buffer submission triggers by timeout for the leftover [3].
        assert_submission_count_at(
            t_4 + TIMEOUT + SLACK,
            &shared,
            3,
            "timeout flush of [3] expected",
        )
        .await;
    }

    {
        // Phase 3: Exact threshold flush + subsequent ingest
        let phase3_ids: Vec<S3ObjectMetadataId> = (4..=123).collect();
        send_to_listener(create_test_buffer_entries(&phase3_ids), sender.clone()).await;
        // Allow the listener task to process the entries and trigger the size flush.
        tokio::time::sleep(SLACK).await;
        let t_5 = Instant::now();

        assert_eq!(
            shared.lock().await.len(),
            4,
            "size flush of [4..123] expected"
        );

        tokio::time::sleep(WAIT).await; // t_6

        // Ingest one more object. The buffer was empty, so the timer starts fresh at t_7.
        send_to_listener(create_test_buffer_entries(&[124]), sender.clone()).await;
        let t_7 = Instant::now();

        // At t_5 + timeout, no submission should trigger because the timer was set at t_7, not t_5.
        assert_submission_count_at(
            t_5 + TIMEOUT,
            &shared,
            4,
            "timer should not have fired at t_5 + timeout",
        )
        .await;

        // At t_7 + timeout, the buffer submission triggers by timeout for [124].
        assert_submission_count_at(
            t_7 + TIMEOUT + SLACK,
            &shared,
            5,
            "timeout flush of [124] expected",
        )
        .await;
    }

    // Clean up.
    listener.shutdown_and_join().await;
    Ok(())
}
