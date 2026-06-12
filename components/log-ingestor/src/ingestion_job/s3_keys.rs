use std::{
    collections::HashSet,
    sync::{Arc, Mutex},
};

use anyhow::Result;
use aws_sdk_s3::Client;
use clp_rust_utils::{job_config::ingestion::s3::S3KeysConfig, s3::ObjectMetadata};

use crate::{
    aws_client_manager::AwsClientManagerType,
    ingestion_job::{FinalizeOutcome, IngestionJobId, OneTimeIngestionState, scan_prefix},
};

/// Represents a one-time S3 explicit-keys ingestion task that scans a given prefix under the
/// bucket once and finalizes the metadata of the requested objects.
///
/// # Type Parameters
///
/// * [`S3ClientManager`]: The type of the AWS S3 client manager.
/// * [`State`]: The type that implements [`OneTimeIngestionState`] for managing one-time ingestion
///   states.
struct Task<S3ClientManager: AwsClientManagerType<Client>, State: OneTimeIngestionState> {
    s3_client_manager: S3ClientManager,
    config: S3KeysConfig,
    state: State,
}

impl<S3ClientManager: AwsClientManagerType<Client>, State: OneTimeIngestionState>
    Task<S3ClientManager, State>
{
    /// Runs the one-time explicit-keys ingestion task to scan the given bucket and finalize the
    /// metadata of the requested objects.
    ///
    /// # Returns
    ///
    /// `Ok(())` on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`OneTimeIngestionState::start`]'s return values on failure.
    /// * Forwards [`AwsClientManagerType::get`]'s return values on failure.
    /// * Forwards [`scan_prefix`]'s return values on failure.
    /// * [`anyhow::Error`] if one or more requested keys are missing after scanning the configured
    ///   prefix to exhaustion.
    /// * Forwards [`OneTimeIngestionState::finalize`]'s return values on failure.
    async fn run(self) -> Result<()> {
        self.state.start().await?;

        let client = self.s3_client_manager.get().await?;
        let found_objects = Arc::new(Mutex::new(Vec::new()));
        let collected_objects = Arc::clone(&found_objects);
        let remaining_keys = Arc::new(Mutex::new(
            self.config
                .object_keys
                .iter()
                .cloned()
                .collect::<HashSet<_>>(),
        ));
        let pending_keys = Arc::clone(&remaining_keys);
        scan_prefix(
            &client,
            &self.config.base.bucket_name,
            &self.config.base.key_prefix,
            None,
            async move |page: Vec<ObjectMetadata>| -> Result<_> {
                let last_key = page
                    .last()
                    .expect("scanned page should not be empty")
                    .key
                    .clone();
                let mut matched_objects = Vec::new();
                let mut remaining_keys = pending_keys
                    .lock()
                    .expect("remaining keys mutex should not be poisoned");
                for object in page {
                    if remaining_keys.remove(&object.key) {
                        matched_objects.push(object);
                    }
                }
                let should_continue_scanning = !remaining_keys.is_empty();
                drop(remaining_keys);
                if !matched_objects.is_empty() {
                    collected_objects
                        .lock()
                        .expect("collected objects mutex should not be poisoned")
                        .extend(matched_objects);
                }
                Ok((should_continue_scanning, last_key))
            },
        )
        .await?;

        let remaining_keys = Arc::into_inner(remaining_keys)
            .expect("remaining keys should have no remaining references")
            .into_inner()
            .expect("remaining keys mutex should not be poisoned");
        if !remaining_keys.is_empty() {
            let mut remaining_keys: Vec<String> = remaining_keys
                .into_iter()
                .map(|key| key.to_string())
                .collect();
            remaining_keys.sort();
            return Err(anyhow::anyhow!(
                "failed to find requested S3 object keys: {}",
                remaining_keys.join(", ")
            ));
        }

        let all_objects = Arc::into_inner(found_objects)
            .expect("collected objects should have no remaining references")
            .into_inner()
            .expect("collected objects mutex should not be poisoned");
        let outcome = self.state.finalize(all_objects).await?;
        match outcome {
            FinalizeOutcome::Finished => {
                tracing::info!("S3 keys ingestion finalized.");
            }
            FinalizeOutcome::Cancelled => {
                tracing::info!("S3 keys ingestion finalization cancelled.");
            }
        }

        Ok(())
    }
}

/// Represents a one-time S3 explicit-keys ingestion job that manages the lifecycle of a one-time
/// S3 explicit-keys ingestion task.
pub struct S3KeysIngestion {
    id: IngestionJobId,
}

impl S3KeysIngestion {
    /// Creates and spawns a new [`S3KeysIngestion`] backed by a [`Task`].
    ///
    /// This function spawns a [`Task`]. The spawned task will scan the configured S3 bucket and
    /// prefix once, collect the metadata for the requested object keys, and finalize them using
    /// the provided state.
    ///
    /// # Type Parameters
    ///
    /// * [`S3ClientManager`]: The type of the AWS S3 client manager.
    /// * [`State`]: The type that implements [`OneTimeIngestionState`] for managing one-time
    ///   ingestion states.
    ///
    /// # Returns
    ///
    /// A newly created instance of [`S3KeysIngestion`].
    #[must_use]
    pub fn spawn<S3ClientManager: AwsClientManagerType<Client>, State: OneTimeIngestionState>(
        id: IngestionJobId,
        s3_client_manager: S3ClientManager,
        config: S3KeysConfig,
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
                    "S3 keys ingestion task execution failed."
                );
                state
                    .fail(format!("S3 keys ingestion task execution failed: {err}"))
                    .await;
            }
        });
        Self { id }
    }

    /// # Returns
    ///
    /// The ID of this S3 keys ingestion job.
    #[must_use]
    pub const fn get_id(&self) -> IngestionJobId {
        self.id
    }
}
