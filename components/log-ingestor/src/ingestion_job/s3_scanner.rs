use std::time::Duration;

use anyhow::Result;
use aws_sdk_s3::Client;
use clp_rust_utils::{job_config::ingestion::s3::S3ScannerConfig, s3::ObjectMetadata};
use non_empty_string::NonEmptyString;
use tokio::select;
use tokio_util::sync::CancellationToken;

use crate::{
    aws_client_manager::AwsClientManagerType,
    ingestion_job::{IngestionJobId, IngestionJobState, S3ScannerState},
};

/// Represents a S3 scanner task that periodically scans a given prefix under the bucket to fetch
/// object metadata for newly created objects.
///
/// # Type Parameters
///
/// * [`S3ClientManager`]: The type of the AWS S3 client manager.
/// * [`State`]: The type that implements [`IngestionJobState`] + [`S3ScannerState`] for managing S3
///   scanner states.
struct Task<
    S3ClientManager: AwsClientManagerType<Client>,
    State: IngestionJobState + S3ScannerState,
> {
    s3_client_manager: S3ClientManager,
    scanning_interval: Duration,
    config: S3ScannerConfig,
    start_after: Option<String>,
    state: State,
}

impl<S3ClientManager: AwsClientManagerType<Client>, State: IngestionJobState + S3ScannerState>
    Task<S3ClientManager, State>
{
    /// Runs the S3 scanner task to scan the given bucket.
    ///
    /// This is a wrapper of [`Self::scan_once`] that supports cancellation via the provided
    /// cancellation token.
    ///
    /// # Returns
    ///
    /// `Ok(())` on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`Self::scan_once`]'s return values on failure.
    pub async fn run(mut self, cancel_token: CancellationToken) -> Result<()> {
        loop {
            select! {
                // Cancellation requested.
                () = cancel_token.cancelled() => {
                    return Ok(());
                }

                // Scanner execution
                is_truncated_result = self.scan_once() => {
                    if is_truncated_result? {
                        // The results are truncated. Keep going until all objects are listed.
                        // Ideally, we can use the continuation token to continue listing objects,
                        // but since we may refresh the client in the next scan cycle, we will use
                        // `start_after` to send a new request to resume the scanning progress for
                        // simplicity.
                        continue;
                    }
                }
            }

            // Sleep for the configured interval or until cancellation is requested.
            select! {
                () = cancel_token.cancelled() => {
                    return Ok(());
                }
                () = self.sleep() => {}
            }
        }
    }

    /// Scans the configured S3 bucket and prefix for new objects and ingests their metadata into
    /// the provided state.
    ///
    /// # Note
    ///
    /// The scanner uses the `start_after` parameter to keep track of the last scanned object key,
    /// meaning that it requires the keys of newly inserted objects to be lexicographically larger
    /// than the last successfully ingested key.
    ///
    /// # Returns
    ///
    /// Whether the underlying `list_buckets` call was truncated.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`AwsClientManagerType::get`]'s return values on failure.
    /// * Forwards
    ///   [`aws_sdk_s3::operation::list_objects_v2::builders::ListObjectsV2FluentBuilder::send`]'s
    ///   return values on failure.
    /// * Forwards [`NonEmptyString::new`]'s return values on failure.
    /// * Forwards [`S3ScannerState::ingest`]'s return values on failure.
    /// * Forwards [`i64::try_into`]'s return values when failing to convert object size to [`u64`].
    pub async fn scan_once(&mut self) -> Result<bool> {
        let client = self.s3_client_manager.get().await?;
        let response = client
            .list_objects_v2()
            .bucket(self.config.base.bucket_name.as_str())
            .prefix(self.config.base.key_prefix.as_str())
            .set_start_after(self.start_after.clone())
            .send()
            .await?;
        let Some(contents) = response.contents else {
            return Ok(false);
        };

        let mut object_metadata_to_ingest = Vec::with_capacity(contents.len());
        for content in contents {
            let (Some(key), Some(size)) = (content.key, content.size) else {
                continue;
            };
            if key.ends_with('/') {
                continue;
            }
            let object_metadata = ObjectMetadata {
                bucket: self.config.base.bucket_name.clone(),
                key: NonEmptyString::new(key.clone())
                    .map_err(|_| anyhow::anyhow!("An empty key is received."))?,
                size: size.try_into()?,
                id: None,
            };
            object_metadata_to_ingest.push(object_metadata);
        }

        if object_metadata_to_ingest.is_empty() {
            return Ok(response.is_truncated.unwrap_or(false));
        }

        let last_ingested_key: String = object_metadata_to_ingest
            .last()
            .expect("`object_metadata_to_ingest` should not be empty")
            .key
            .clone()
            .into();
        self.state
            .ingest(object_metadata_to_ingest, last_ingested_key.as_str())
            .await?;
        self.start_after = Some(last_ingested_key);

        Ok(response.is_truncated.unwrap_or(false))
    }

    /// Sleeps for the configured scanning interval.
    pub async fn sleep(&self) {
        tokio::time::sleep(self.scanning_interval).await;
    }
}

/// Represents a S3 scanner job that manages the lifecycle of a S3 scanner task.
///
/// # Type Parameters
///
/// * [`State`]: The type that implements [`IngestionJobState`] + [`S3ScannerState`] for managing S3
///   scanner states.
pub struct S3Scanner<State: IngestionJobState + S3ScannerState> {
    id: IngestionJobId,
    cancel_token: CancellationToken,
    handle: tokio::task::JoinHandle<Result<()>>,
    state: State,
}

impl<State: IngestionJobState + S3ScannerState> S3Scanner<State> {
    /// Creates and spawns a new [`S3Scanner`] backed by a [`Task`].
    ///
    /// This function spawns a [`Task`]. The spawned task will periodically scan the configured S3
    /// bucket and prefix for new objects and ingest their metadata into the provided state.
    ///
    /// # Type parameters
    ///
    /// * [`S3ClientManager`]: The type of the AWS S3 client manager.
    ///
    /// # Returns
    ///
    /// A newly created instance of [`S3Scanner`].
    #[must_use]
    pub fn spawn<S3ClientManager: AwsClientManagerType<Client>>(
        id: IngestionJobId,
        s3_client_manager: S3ClientManager,
        config: S3ScannerConfig,
        state: State,
    ) -> Self {
        let scanning_interval = Duration::from_secs(u64::from(config.scanning_interval_sec));
        let start_after = config.start_after.clone().map(Into::into);
        let task = Task {
            s3_client_manager,
            scanning_interval,
            config,
            start_after,
            state: state.clone(),
        };
        let cancel_token = CancellationToken::new();
        let child_cancel_token = cancel_token.clone();
        let state_copy = state.clone();
        let handle = tokio::spawn(async move {
            let result = task.run(child_cancel_token).await;
            match &result {
                Ok(()) => {}
                Err(err) => {
                    tracing::error!(error = ? err, "S3 scanner task execution failed.");
                    state_copy
                        .fail(format!("S3 scanner task execution failed: {err}"))
                        .await;
                }
            }
            result
        });
        Self {
            id,
            cancel_token,
            handle,
            state,
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
    pub async fn shutdown_and_join(self) {
        self.cancel_token.cancel();
        match self.handle.await {
            Ok(Ok(())) => {
                tracing::info!(
                    job_id = ? self.id,
                    "S3 scanner task completed successfully."
                );
            }
            Ok(Err(_)) => {
                // We don't need to log the error here because the underlying task will log it.
                tracing::warn!(
                    job_id = ? self.id,
                    "S3 scanner task completed with an error."
                );
            }
            Err(err) => {
                tracing::warn!(
                    error = ? err,
                    job_id = ? self.id,
                    "S3 scanner task panicked."
                );
                self.state
                    .fail(format!("S3 scanner task panicked: {err}"))
                    .await;
            }
        }
    }

    /// # Returns
    ///
    /// The ID of this scanner.
    #[must_use]
    pub const fn get_id(&self) -> IngestionJobId {
        self.id
    }
}
