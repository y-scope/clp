use anyhow::Result;
use aws_sdk_sqs::{Client, operation::receive_message::ReceiveMessageOutput};
use clp_rust_utils::{
    s3::ObjectMetadata,
    sqs::event::{Record, S3},
};
use tokio::{select, sync::mpsc};
use tokio_util::sync::CancellationToken;
use uuid::Uuid;

#[derive(Debug, Clone)]
pub struct SqsListenerConfig {
    pub queue_url: String,
    pub bucket_name: String,
    pub prefix: String,
    pub max_num_messages_to_fetch: i32,
    pub init_polling_backoff_sec: i32,
    pub max_polling_backoff_sec: i32,
}

struct SqsListenerTask {
    sqs_client: Client,
    config: SqsListenerConfig,
    sender: mpsc::Sender<ObjectMetadata>,
}

impl SqsListenerTask {
    pub async fn run(self, cancel_token: CancellationToken) -> Result<()> {
        let mut polling_backoff_sec = self.config.init_polling_backoff_sec;
        loop {
            select! {
                // Cancellation requested.
                () = cancel_token.cancelled() => {
                    return Ok(());
                }

                // Listen to SQS messages.
                result = self.sqs_client
                    .receive_message()
                    .queue_url(self.config.queue_url.as_str())
                    .max_number_of_messages(self.config.max_num_messages_to_fetch)
                    .wait_time_seconds(polling_backoff_sec).send() => {
                    polling_backoff_sec = if self.process_sqs_response(result?).await? {
                        self.config.init_polling_backoff_sec
                    } else { std::cmp::min(
                        polling_backoff_sec * 2,
                        self.config.max_polling_backoff_sec
                    ) };
                }
            }
        }
    }

    async fn process_sqs_response(&self, response: ReceiveMessageOutput) -> Result<bool> {
        if response.messages.is_none() {
            return Ok(false);
        }
        let mut ingested = false;
        for msg in response.messages.unwrap() {
            if msg.body.is_none() {
                continue;
            }

            let event = match serde_json::from_str::<S3>(msg.body.as_ref().unwrap()) {
                Ok(event) => event,
                Err(_e) => {
                    continue;
                }
            };

            for record in event.records {
                if let Some(object_metadata) = self.extract_object_metadata(record) {
                    self.sender.send(object_metadata).await?;
                    ingested = true;
                }
            }
            if let Some(receipt_handle) = msg.receipt_handle() {
                self.sqs_client
                    .delete_message()
                    .queue_url(self.config.queue_url.as_str())
                    .receipt_handle(receipt_handle)
                    .send()
                    .await?;
            }
        }
        Ok(ingested)
    }

    fn extract_object_metadata(&self, record: Record) -> Option<ObjectMetadata> {
        if false == record.event_name.starts_with("ObjectCreated:")
            || self.config.bucket_name != record.s3.bucket.name.as_str()
            || false == self.is_relevant_object(record.s3.object.key.as_str())
        {
            return None;
        }
        Some(ObjectMetadata {
            bucket: record.s3.bucket.name,
            key: record.s3.object.key,
            size: record.s3.object.size,
        })
    }

    fn is_relevant_object(&self, object_key: &str) -> bool {
        false == object_key.ends_with('/') && object_key.starts_with(self.config.prefix.as_str())
    }
}

pub struct SqsListener {
    id: Uuid,
    cancel_token: CancellationToken,
    handle: tokio::task::JoinHandle<Result<()>>,
}

impl SqsListener {
    #[must_use]
    pub fn spawn(
        id: Uuid,
        sqs_client: Client,
        config: SqsListenerConfig,
        sender: mpsc::Sender<ObjectMetadata>,
    ) -> Self {
        let task = SqsListenerTask {
            sqs_client,
            config,
            sender,
        };
        let cancel_token = CancellationToken::new();
        let child_cancel_token = cancel_token.clone();
        let handle = tokio::spawn(async move { task.run(child_cancel_token).await });
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
    /// * Forwards the underlying task's return values on failure ([`SqsListenerTask::run`]).
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
