use anyhow::Result;
use aws_sdk_sqs::{Client, operation::receive_message::ReceiveMessageOutput};
use clp_rust_utils::{
    job_config::ingestion::s3::SqsListenerConfig,
    s3::ObjectMetadata,
    sqs::event::{Record, S3},
};
use tokio::{select, sync::mpsc};
use tokio_util::sync::CancellationToken;
use uuid::Uuid;

use crate::aws_client_manager::AwsClientManagerType;

/// Represents a SQS listener task that listens to SQS messages and extracts S3 object metadata.
///
/// # Type Parameters
///
/// * [`SqsClientManager`]: The type of the AWS SQS client manager.
struct Task<SqsClientManager: AwsClientManagerType<Client>> {
    sqs_client_manager: SqsClientManager,
    config: SqsListenerConfig,
    sender: mpsc::Sender<ObjectMetadata>,
}

impl<SqsClientManager: AwsClientManagerType<Client>> Task<SqsClientManager> {
    /// Runs the SQS listener task to listen to SQS messages and extract S3 object metadata. The
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
    /// * Forwards [`AwsClientManagerType::get`]'s return values on failure.
    /// * Forwards [`Self::process_sqs_response`]'s return values on failure.
    /// * Forwards
    ///   [`aws_sdk_sqs::operation::receive_message::builders::ReceiveMessageFluentBuilder::send`]'s
    ///   return values on failure.
    pub async fn run(self, cancel_token: CancellationToken) -> Result<()> {
        const MAX_NUM_MESSAGES_TO_FETCH: i32 = 10;
        const MAX_WAIT_TIME_SEC: i32 = 20;

        loop {
            select! {
                // Cancellation requested.
                () = cancel_token.cancelled() => {
                    return Ok(());
                }

                // Listen to SQS messages.
                result = self.sqs_client_manager.get().await?
                    .receive_message()
                    .queue_url(self.config.queue_url.as_str())
                    .max_number_of_messages(MAX_NUM_MESSAGES_TO_FETCH)
                    .wait_time_seconds(MAX_WAIT_TIME_SEC).send() => {
                    self.process_sqs_response(result?).await?;
                }
            }
        }
    }

    /// Processes the SQS response to extract S3 object metadata and send it to the channel sender.
    ///
    /// # NOTE
    ///
    /// The message will be deleted from the SQS queue if it is successfully processed.
    ///
    /// # Returns
    ///
    /// Whether any relevant object metadata was ingested.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`mpsc::Sender::send`]'s return values on failure.
    /// * Forwards
    ///   [`aws_sdk_sqs::operation::delete_message::builders::DeleteMessageFluentBuilder::send`]'s
    ///   return values on failure.
    async fn process_sqs_response(&self, response: ReceiveMessageOutput) -> Result<bool> {
        let Some(messages) = response.messages else {
            return Ok(false);
        };

        let mut ingested = false;
        for msg in messages {
            let Some(body) = msg.body.as_ref() else {
                continue;
            };

            let event = match serde_json::from_str::<S3>(body) {
                Ok(event) => event,
                Err(_e) => {
                    continue;
                }
            };

            for record in event.records {
                if let Some(object_metadata) = self.extract_object_metadata(record) {
                    tracing::info!(
                        object = ? object_metadata,
                        "Received new object metadata from SQS."
                    );
                    self.sender.send(object_metadata).await?;
                    ingested = true;
                }
            }
            if let Some(receipt_handle) = msg.receipt_handle() {
                self.sqs_client_manager
                    .get()
                    .await?
                    .delete_message()
                    .queue_url(self.config.queue_url.as_str())
                    .receipt_handle(receipt_handle)
                    .send()
                    .await?;
            }
        }
        Ok(ingested)
    }

    /// Extracts S3 object metadata from the given SQS record if it corresponds to a relevant
    /// object.
    ///
    /// # Returns
    ///
    /// * `Some(ObjectMetadata)` if the record corresponds to a relevant object.
    /// * `None` if:
    ///   * The event is not an object creation event.
    ///   * The bucket name does not match the listener's configuration.
    ///   * [`Self::is_relevant_object`] evaluates to `false`.
    fn extract_object_metadata(&self, record: Record) -> Option<ObjectMetadata> {
        if !record.event_name.starts_with("ObjectCreated:")
            || self.config.base.bucket_name != record.s3.bucket.name.as_str()
            || !self.is_relevant_object(record.s3.object.key.as_str())
        {
            return None;
        }
        Some(ObjectMetadata {
            bucket: record.s3.bucket.name,
            key: record.s3.object.key,
            size: record.s3.object.size,
        })
    }

    /// # Returns:
    ///
    /// Whether the object key corresponds to a relevant object based on the listener's prefix.
    fn is_relevant_object(&self, object_key: &str) -> bool {
        !object_key.ends_with('/') && object_key.starts_with(self.config.base.key_prefix.as_str())
    }
}

/// Represents a SQS listener job that manages the lifecycle of a SQS listener task.
pub struct SqsListener {
    id: Uuid,
    cancel_token: CancellationToken,
    handle: tokio::task::JoinHandle<Result<()>>,
}

impl SqsListener {
    /// Creates and spawns a new [`SqsListener`] backed by a [`Task`].
    ///
    /// This function spawns a [`Task`]. The spawned task will listen to SQS messages, extract
    /// relevant S3 object metadata, and send the metadata to the provided channel sender.
    ///
    /// # Type parameters
    ///
    /// * [`SqsClientManager`]: The type of the AWS SQS client manager.
    ///
    /// # Returns
    ///
    /// A newly created instance of [`SqsListener`].
    #[must_use]
    pub fn spawn<SqsClientManager: AwsClientManagerType<Client>>(
        id: Uuid,
        sqs_client_manager: SqsClientManager,
        config: SqsListenerConfig,
        sender: mpsc::Sender<ObjectMetadata>,
    ) -> Self {
        let task = Task {
            sqs_client_manager,
            config,
            sender,
        };
        let cancel_token = CancellationToken::new();
        let child_cancel_token = cancel_token.clone();
        let handle = tokio::spawn(async move {
            task.run(child_cancel_token).await.inspect_err(|err| {
                tracing::error!(error = ? err, "SQS listener task execution failed.");
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
