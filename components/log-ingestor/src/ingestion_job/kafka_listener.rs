use std::time::Duration;

use backoff::{backoff::Backoff, ExponentialBackoff};
use crate::kafka_consumer_wrapper::KafkaMessageReceiver;
use anyhow::Result;
use clp_rust_utils::{
    job_config::ingestion::s3::KafkaListenerConfig,
    s3::{ObjectMetadata, extract_object_metadata},
    s3_event::S3,
};
use tokio::{select, sync::mpsc, time::sleep};
use tokio_util::sync::CancellationToken;
use uuid::Uuid;

/// Represents a Kafka listener task that listens to Kafka messages and extracts S3 object metadata.
///
/// # Type Parameters
///
/// * `Consumer`: The type of Kafka message receiver. Must implement [`KafkaMessageReceiver`].
struct Task<Consumer: KafkaMessageReceiver> {
    consumer: Consumer,
    config: KafkaListenerConfig,
    sender: mpsc::Sender<ObjectMetadata>,
}

impl<Consumer: KafkaMessageReceiver> Task<Consumer> {
    /// Runs the Kafka listener task to listen to Kafka messages and extract S3 object metadata. The
    /// extracted metadata is sent to the provided channel sender.
    ///
    /// # Returns
    ///
    /// `Ok(())` on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`KafkaMessageReceiver::recv`]'s return values on failure.
    /// * Forwards [`Self::process_kafka_message`]'s return values on failure.
    pub async fn run(self, cancel_token: CancellationToken) -> Result<()> {
        let mut backoff = ExponentialBackoff {
            initial_interval: Duration::from_millis(100),
            max_interval: Duration::from_secs(1),
            max_elapsed_time: Some(Duration::from_secs(2)), 
            ..ExponentialBackoff::default()
        };

        loop {
            select! {
                // Cancellation requested.
                () = cancel_token.cancelled() => {
                    return Ok(());
                }

                // Receive the next Kafka message.
                result = self.consumer.recv() => {
                    match result {
                        Ok(payload) => {
                            backoff.reset();
                            self.process_kafka_message(payload).await?;
                        }
                        Err(e) => {
                            let Some(delay) = backoff.next_backoff() else {
                                tracing::error!(
                                    error = ?e,
                                    "Kafka receive errors exceeded max elapsed time, stopping listener"
                                );
                                return Err(e);
                            };

                            tracing::warn!(
                                error = ?e,
                                delay_ms = delay.as_millis(),
                                "Failed to receive Kafka message, retrying with backoff"
                            );

                            // Sleep with cancellation support.
                            select! {
                                () = cancel_token.cancelled() => {
                                    return Ok(());
                                }
                                () = sleep(delay) => {
                                    continue;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    async fn process_kafka_message(&self, payload: String) -> Result<()> {
        let event = match serde_json::from_str::<S3>(&payload) {
            Ok(event) => event,
            Err(e) => {
                tracing::warn!(error = ?e, "Failed to deserialize Kafka message as S3 event, skipping");
                return Ok(());
            }
        };

        for record in event.records {
            if let Some(object_metadata) = extract_object_metadata(
                record,
                self.config.base.bucket_name.as_str(),
                self.config.base.key_prefix.as_str(),
            ) {
                self.sender.send(object_metadata).await?;
            }
        }

        Ok(())
    }
}

/// Represents a Kafka listener job that manages the lifecycle of a Kafka listener task.
pub struct KafkaListener {
    id: Uuid,
    cancel_token: CancellationToken,
    handle: tokio::task::JoinHandle<Result<()>>,
}

impl KafkaListener {
    /// Creates and spawns a new [`KafkaListener`] backed by a [`Task`].
    ///
    /// This function spawns a [`Task`]. The spawned task will listen to Kafka messages, extract
    /// relevant S3 object metadata, and send the metadata to the provided channel sender.
    ///
    /// # Type Parameters
    ///
    /// * `Consumer`: The type of Kafka message receiver. Must implement [`KafkaMessageReceiver`].
    ///
    /// # Returns
    ///
    /// A newly created instance of [`KafkaListener`].
    #[must_use]
    pub fn spawn<Consumer: KafkaMessageReceiver>(
        id: Uuid,
        consumer: Consumer,
        config: KafkaListenerConfig,
        sender: mpsc::Sender<ObjectMetadata>,
    ) -> Self {
        let task = Task {
            consumer,
            config,
            sender,
        };
        let cancel_token = CancellationToken::new();
        let child_cancel_token = cancel_token.clone();
        let handle = tokio::spawn(async move {
            task.run(child_cancel_token).await.inspect_err(|err| {
                tracing::error!(error = ? err, "Kafka listener task execution failed.");
            })
        });
        Self {
            id,
            cancel_token,
            handle,
        }
    }

    /// Shuts down and waits for the underlying task to complete.
    ///
    /// # Returns
    ///
    /// `Ok(())` on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards the underlying task's return values on failure ([`Task::run`]).
    pub async fn shutdown_and_join(self) -> Result<()> {
        self.cancel_token.cancel();
        self.handle.await?
    }

    /// # Returns
    ///
    /// The UUID of this listener.
    #[must_use]
    pub fn get_id(&self) -> String {
        self.id.to_string()
    }
}
