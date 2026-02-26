use std::cmp::min;

use anyhow::Result;
use aws_sdk_sqs::{
    Client,
    operation::receive_message::ReceiveMessageOutput,
    types::DeleteMessageBatchRequestEntry,
};
use clp_rust_utils::{
    job_config::ingestion::s3::ValidatedSqsListenerConfig,
    s3::ObjectMetadata,
    sqs::event::{Record, S3},
};
use tokio::select;
use tokio_util::sync::CancellationToken;

use crate::{
    aws_client_manager::AwsClientManagerType,
    ingestion_job::{IngestionJobId, IngestionJobState, SqsListenerState},
};

type TaskId = usize;

/// Represents a SQS listener task that listens to SQS messages and extracts S3 object metadata.
///
/// # Type Parameters
///
/// * [`SqsClientManager`]: The type of the AWS SQS client manager.
/// * [`State`]: The type that implements [`IngestionJobState`] and [`SqsListenerState`] for
///   managing SQS listener states.
struct Task<
    SqsClientManager: AwsClientManagerType<Client>,
    State: IngestionJobState + SqsListenerState,
> {
    sqs_client_manager: SqsClientManager,
    config: ValidatedSqsListenerConfig,
    job_id: IngestionJobId,
    id: TaskId,
    state: State,
}

impl<SqsClientManager: AwsClientManagerType<Client>, State: IngestionJobState + SqsListenerState>
    Task<SqsClientManager, State>
{
    /// Runs the SQS listener task to listen to SQS messages and extract S3 object metadata. The
    /// extracted metadata is ingested into the provided state.
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
        const MAX_WAIT_TIME_SEC: u16 = 20;
        let config = self.config.get();
        let wait_time_sec = i32::from(min(config.wait_time_sec, MAX_WAIT_TIME_SEC));

        loop {
            select! {
                // Cancellation requested.
                () = cancel_token.cancelled() => {
                    return Ok(());
                }

                // Listen to SQS messages.
                result = self.sqs_client_manager.get().await?
                    .receive_message()
                    .queue_url(config.queue_url.as_str())
                    .max_number_of_messages(MAX_NUM_MESSAGES_TO_FETCH)
                    .wait_time_seconds(wait_time_sec).send() => {
                    self.process_sqs_response(result?).await?;
                }
            }
        }
    }

    /// Processes the SQS response to extract S3 object metadata and ingests it into the provided
    /// state.
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
    /// * Forwards [`AwsClientManagerType::get`]'s return values on failure.
    /// * Forwards the following `aws_sdk_sqs` methods' return values on failure:
    ///   * [`DeleteMessageBatchFluentBuilder::send`]
    ///   * [`DeleteMessageBatchRequestEntryBuilder::build`]
    /// * Forwards [`SqsListenerState::ingest`]'s return values on failure.
    async fn process_sqs_response(&self, response: ReceiveMessageOutput) -> Result<bool> {
        let Some(messages) = response.messages else {
            return Ok(false);
        };

        if messages.is_empty() {
            return Ok(false);
        }

        let mut object_metadata_to_ingest = Vec::with_capacity(messages.len());
        let mut delete_message_batch_request_entries = Vec::with_capacity(messages.len());
        for (idx, msg) in messages.into_iter().enumerate() {
            if let Some(receipt_handle) = msg.receipt_handle() {
                delete_message_batch_request_entries.push(
                    DeleteMessageBatchRequestEntry::builder()
                        .id(idx.to_string())
                        .receipt_handle(receipt_handle)
                        .build()?,
                );
            } else {
                tracing::warn!(
                    job_id = ? self.job_id,
                    task_id = ? self.id,
                    msg = ? msg,
                    "Received SQS message without receipt handle."
                );
            }

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
                    object_metadata_to_ingest.push(object_metadata);
                }
            }
        }

        let ingested = !object_metadata_to_ingest.is_empty();
        if ingested {
            self.state.ingest(object_metadata_to_ingest).await?;
        }

        if !delete_message_batch_request_entries.is_empty() {
            // This check is necessary since SQS service requires at least one
            // `DeleteMessageBatchRequestEntry` in the request.
            for failure in self
                .sqs_client_manager
                .get()
                .await?
                .delete_message_batch()
                .queue_url(self.config.get().queue_url.as_str())
                .set_entries(Some(delete_message_batch_request_entries))
                .send()
                .await?
                .failed
            {
                tracing::warn!(
                    job_id = ? self.job_id,
                    task_id = ? self.id,
                    error = ? failure,
                    "Failed to delete SQS message."
                );
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
            || self.config.get().base.bucket_name != record.s3.bucket.name.as_str()
            || !self.is_relevant_object(record.s3.object.key.as_str())
        {
            return None;
        }
        Some(ObjectMetadata {
            bucket: record.s3.bucket.name,
            key: record.s3.object.key,
            size: record.s3.object.size,
            id: None,
        })
    }

    /// # Returns:
    ///
    /// Whether the object key corresponds to a relevant object based on the listener's prefix.
    fn is_relevant_object(&self, object_key: &str) -> bool {
        !object_key.ends_with('/')
            && object_key.starts_with(self.config.get().base.key_prefix.as_str())
    }
}

struct TaskHandle {
    task_id: TaskId,
    cancel_token: CancellationToken,
    join_handle: tokio::task::JoinHandle<Result<()>>,
}

impl TaskHandle {
    /// Spawns the given task and returns a handle to manage its lifecycle.
    ///
    /// # Returns
    ///
    /// A handle to the spawned task.
    fn spawn<
        SqsClientManager: AwsClientManagerType<Client>,
        State: IngestionJobState + SqsListenerState,
    >(
        task: Task<SqsClientManager, State>,
        job_id: IngestionJobId,
        state: State,
    ) -> Self {
        let cancel_token = CancellationToken::new();
        let child_cancel_token = cancel_token.clone();
        let task_id = task.id;
        let join_handle = tokio::spawn(async move {
            let result = task.run(child_cancel_token).await;
            match &result {
                Ok(()) => {}
                Err(err) => {
                    tracing::error!(
                        error = ? err,
                        job_id = ? job_id,
                        task_id = ? task_id,
                        "SQS listener task execution failed."
                    );
                    state
                        .fail(format!(
                            "SQS listener task (ID={task_id}) execution failed: {err}"
                        ))
                        .await;
                }
            }
            result
        });
        Self {
            task_id,
            cancel_token,
            join_handle,
        }
    }
}

/// Represents a SQS listener job that manages the lifecycle of a SQS listener task.
///
/// # Type Parameters
///
/// * [`State`]: The type that implements [`IngestionJobState`] and [`SqsListenerState`] for
///   managing SQS listener states.
pub struct SqsListener<State: IngestionJobState + SqsListenerState> {
    id: IngestionJobId,
    task_handles: Vec<TaskHandle>,
    state: State,
}

impl<State: IngestionJobState + SqsListenerState> SqsListener<State> {
    /// Creates and spawns a new [`SqsListener`] backed by a [`Task`].
    ///
    /// This function spawns a series of [`Task`]. Each spawned task will listen to SQS messages,
    /// extract relevant S3 object metadata, and ingest the metadata into the provided state.
    ///
    /// # Type parameters
    ///
    /// * [`SqsClientManager`]: The type of the AWS SQS client manager.
    ///
    /// # Returns
    ///
    /// A newly created instance of [`SqsListener`].
    ///
    /// # Panics
    ///
    /// Panics if the number of concurrent listener tasks is not a positive integer.
    ///
    /// This method does not perform full configuration validation. It assumes the provided
    /// configuration has already been validated by an upper-layer caller. In particular, enforcing
    /// domain constraints—such as upper bounds on the number of concurrent listener tasks—is the
    /// responsibility of the caller.
    pub fn spawn<SqsClientManager: AwsClientManagerType<Client>>(
        id: IngestionJobId,
        sqs_client_manager: &SqsClientManager,
        config: &ValidatedSqsListenerConfig,
        state: State,
    ) -> Self {
        let num_tasks = usize::from(config.get().num_concurrent_listener_tasks);
        let mut task_handles = Vec::with_capacity(num_tasks);
        for task_id in 0..num_tasks {
            let task = Task {
                sqs_client_manager: sqs_client_manager.clone(),
                config: config.clone(),
                job_id: id,
                id: TaskId::from(task_id),
                state: state.clone(),
            };
            let task_handle = TaskHandle::spawn(task, id, state.clone());
            task_handles.push(task_handle);
            tracing::info!(job_id = ? id, task_id = ? task_id, "Spawned SQS listener task.");
        }

        Self {
            id,
            task_handles,
            state,
        }
    }

    /// Shuts down and waits for the underlying tasks to complete.
    pub async fn shutdown_and_join(self) {
        for task_handle in &self.task_handles {
            task_handle.cancel_token.cancel();
        }

        for task_handle in self.task_handles {
            let task_id = task_handle.task_id;
            match task_handle.join_handle.await {
                Ok(Ok(())) => {
                    tracing::info!(
                        job_id = ? self.id,
                        task_id = ? task_id,
                        "SQS listener task completed successfully."
                    );
                }
                Ok(Err(_)) => {
                    // We don't need to log the error here because the underlying task will log it.
                    tracing::warn!(
                        job_id = ? self.id,
                        task_id = ? task_id,
                        "SQS listener task completed with an error."
                    );
                }
                Err(err) => {
                    tracing::warn!(
                        error = ? err,
                        job_id = ? self.id,
                        task_id = ? task_id,
                        "SQS listener task panicked."
                    );
                    self.state
                        .fail(format!("SQS listener task (ID={task_id}) panicked: {err}"))
                        .await;
                }
            }
        }
        tracing::info!(job_id = ? self.id, "SQS listener job shutdown complete.");
    }

    /// # Returns
    ///
    /// The ID of this listener.
    #[must_use]
    pub const fn get_id(&self) -> IngestionJobId {
        self.id
    }
}
