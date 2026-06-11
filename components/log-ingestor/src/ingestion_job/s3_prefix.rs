use std::sync::{Arc, Mutex};

use anyhow::Result;
use aws_sdk_s3::Client;
use clp_rust_utils::{job_config::ingestion::s3::S3PrefixConfig, s3::ObjectMetadata};

use crate::{
    aws_client_manager::AwsClientManagerType,
    ingestion_job::{FinalizeOutcome, IngestionJobId, OneTimeIngestionState, scan_prefix},
};

/// Represents a one-time S3 prefix scan task that collects all object metadata under the configured
/// prefix and finalizes them in a single pass.
///
/// # Type Parameters
///
/// * `S3ClientManager`: The type of the AWS S3 client manager.
/// * `State`: The type that implements [`OneTimeIngestionState`] for managing one-time ingestion
///   state.
struct Task<S3ClientManager: AwsClientManagerType<Client>, State: OneTimeIngestionState> {
    s3_client_manager: S3ClientManager,
    config: S3PrefixConfig,
    state: State,
}

impl<S3ClientManager: AwsClientManagerType<Client>, State: OneTimeIngestionState>
    Task<S3ClientManager, State>
{
    /// Runs the one-time prefix scan task by transitioning the job to the running status and
    /// scanning the configured S3 prefix to exhaustion.
    ///
    /// # NOTE
    ///
    /// Persistence is delegated to the provided [`OneTimeIngestionState`] implementation.
    ///
    /// # Returns
    ///
    /// `Ok(())` on success, including the case where the finalize step was cancelled.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`OneTimeIngestionState::start`]'s return values on failure.
    /// * Forwards [`AwsClientManagerType::get`]'s return values on failure.
    /// * Forwards [`scan_prefix`]'s return values on failure.
    /// * Forwards [`OneTimeIngestionState::finalize`]'s return values on failure.
    async fn run(self) -> Result<()> {
        self.state.start().await?;

        let client = self.s3_client_manager.get().await?;
        let all_objects = Arc::new(Mutex::new(Vec::new()));
        let collected_objects = Arc::clone(&all_objects);
        scan_prefix(
            &client,
            &self.config.base.bucket_name,
            &self.config.base.key_prefix,
            None,
            async move |page: Vec<ObjectMetadata>| -> Result<_> {
                let last_key = page.last().expect("`page` should not be empty").key.clone();
                collected_objects
                    .lock()
                    .expect("collected objects mutex should not be poisoned")
                    .extend(page);
                Ok((true, last_key))
            },
        )
        .await?;

        let all_objects = Arc::into_inner(all_objects)
            .expect("collected objects should have no remaining references")
            .into_inner()
            .expect("collected objects mutex should not be poisoned");
        let outcome = self.state.finalize(all_objects).await?;
        match outcome {
            FinalizeOutcome::Finished => {
                tracing::info!("S3 prefix ingestion finalized.");
            }
            FinalizeOutcome::Cancelled => {
                tracing::info!("S3 prefix ingestion finalization cancelled.");
            }
        }

        Ok(())
    }
}

/// Handle for a one-time S3 prefix ingestion job that scans the configured prefix once, collects
/// all object metadata, and submits a single compression job.
pub struct S3PrefixIngestion {
    id: IngestionJobId,
}

impl S3PrefixIngestion {
    /// Creates and spawns a detached one-time S3 prefix ingestion job backed by a [`Task`].
    ///
    /// # Type Parameters
    ///
    /// * `S3ClientManager`: The type of the AWS S3 client manager.
    /// * `State`: The type that implements [`OneTimeIngestionState`] for managing one-time
    ///   ingestion state.
    ///
    /// # Returns
    ///
    /// A newly created instance of [`S3PrefixIngestion`].
    #[must_use]
    pub fn spawn<S3ClientManager: AwsClientManagerType<Client>, State: OneTimeIngestionState>(
        id: IngestionJobId,
        s3_client_manager: S3ClientManager,
        config: S3PrefixConfig,
        state: State,
    ) -> Self {
        let task = Task {
            s3_client_manager,
            config,
            state: state.clone(),
        };
        tokio::spawn(async move {
            if let Err(err) = task.run().await {
                tracing::error!(
                    error = ? err,
                    job_id = ? id,
                    "S3 prefix ingestion task execution failed."
                );
                state
                    .fail(format!("S3 prefix ingestion task execution failed: {err}"))
                    .await;
            }
        });
        Self { id }
    }

    /// # Returns
    ///
    /// The ID of this S3 prefix ingestion job.
    #[must_use]
    pub const fn get_id(&self) -> IngestionJobId {
        self.id
    }
}
