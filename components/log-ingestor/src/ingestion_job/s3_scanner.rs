use std::time::Duration;

use anyhow::Result;
use aws_sdk_s3::Client;
use clp_rust_utils::{job_config::ingestion::s3::S3ScannerConfig, s3::ObjectMetadata};
use non_empty_string::NonEmptyString;
use tokio::select;
use tokio_util::sync::CancellationToken;

use crate::{
    aws_client_manager::AwsClientManagerType,
    ingestion_job::{IngestionJobId, IngestionJobState, S3ScannerState, scan_prefix},
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
    start_after: Option<NonEmptyString>,
    state: State,
}

impl<S3ClientManager: AwsClientManagerType<Client>, State: IngestionJobState + S3ScannerState>
    Task<S3ClientManager, State>
{
    /// Runs the S3 scanner task to scan the given bucket.
    ///
    /// This is a wrapper of [`Self::scan_until_exhausted`] that supports cancellation via the
    /// provided cancellation token.
    ///
    /// # Returns
    ///
    /// `Ok(())` on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`Self::scan_until_exhausted`]'s return values on failure.
    pub async fn run(mut self, cancel_token: CancellationToken) -> Result<()> {
        loop {
            select! {
                // Cancellation requested.
                () = cancel_token.cancelled() => {
                    return Ok(());
                }

                // Scanner execution
                result = self.scan_until_exhausted() => {
                    result?;
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

    /// Scans the configured S3 bucket and prefix until all pages are exhausted.
    ///
    /// # Note
    ///
    /// The scanner uses the `start_after` parameter to keep track of the last scanned object key,
    /// meaning that it requires the keys of newly inserted objects to be lexicographically larger
    /// than the last successfully ingested key.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`AwsClientManagerType::get`]'s return values on failure.
    /// * Forwards [`scan_prefix`]'s return values on failure.
    pub async fn scan_until_exhausted(&mut self) -> Result<()> {
        let client = self.s3_client_manager.get().await?;
        let state = self.state.clone();
        let last_scanned_key = scan_prefix(
            &client,
            &self.config.base.bucket_name,
            &self.config.base.key_prefix,
            self.start_after.take(),
            async move |page: Vec<ObjectMetadata>| -> Result<(bool, NonEmptyString)> {
                let last_ingested_key =
                    page.last().expect("`page` should not be empty").key.clone();
                state.ingest(page, last_ingested_key.as_str()).await?;
                Ok((true, last_ingested_key))
            },
        )
        .await?;
        self.start_after = last_scanned_key;
        Ok(())
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
        let start_after = config.start_after.clone();
        let task = Task {
            s3_client_manager,
            scanning_interval,
            config,
            start_after,
            state: state.clone(),
        };
        let cancel_token = CancellationToken::new();
        let child_cancel_token = cancel_token.clone();
        let cloned_state = state.clone();
        let handle = tokio::spawn(async move {
            let result = task.run(child_cancel_token).await;
            match &result {
                Ok(()) => {}
                Err(err) => {
                    tracing::error!(
                        error = ? err,
                        job_id = ? id,
                        "S3 scanner task execution failed."
                    );
                    cloned_state
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
        tracing::info!(job_id = ? self.id, "S3 scanner job shutdown complete.");
    }

    /// # Returns
    ///
    /// The ID of this scanner.
    #[must_use]
    pub const fn get_id(&self) -> IngestionJobId {
        self.id
    }
}
