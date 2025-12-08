use std::time::Duration;

use anyhow::Result;
use aws_sdk_s3::Client;
use clp_rust_utils::{job_config::ingestion::s3::S3ScannerConfig, s3::ObjectMetadata};
use tokio::{select, sync::mpsc};
use tokio_util::sync::CancellationToken;
use uuid::Uuid;

use crate::aws_client_manager::AwsClientManagerType;

/// Represents a S3 scanner task that periodically scans a given prefix under the bucket to fetch
/// object metadata for newly created objects.
///
/// # Type Parameters
///
/// * [`S3ClientManager`]: The type of the AWS S3 client manager.
struct Task<S3ClientManager: AwsClientManagerType<Client>> {
    s3_client_manager: S3ClientManager,
    scanning_interval: Duration,
    config: S3ScannerConfig,
    sender: mpsc::Sender<ObjectMetadata>,
}

impl<S3ClientManager: AwsClientManagerType<Client>> Task<S3ClientManager> {
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

    /// Scans the configured S3 bucket and prefix for new objects and sends their metadata to the
    /// underlying channel sender.
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
    pub async fn scan_once(&mut self) -> Result<bool> {
        let client = self.s3_client_manager.get().await?;
        let response = client
            .list_objects_v2()
            .bucket(self.config.base.bucket_name.as_str())
            .prefix(self.config.base.key_prefix.as_str())
            .set_start_after(self.config.start_after.clone())
            .send()
            .await?;
        let Some(contents) = response.contents else {
            return Ok(false);
        };

        for content in contents {
            let (Some(key), Some(size)) = (content.key, content.size) else {
                continue;
            };
            if key.ends_with('/') {
                continue;
            }
            let object_metadata = ObjectMetadata {
                bucket: self.config.base.bucket_name.clone(),
                key: key.clone(),
                size: size.try_into()?,
            };
            tracing::info!(object = ? object_metadata, "Scanned new object metadata on S3.");
            self.sender.send(object_metadata).await?;
            self.config.start_after = Some(key);
        }

        Ok(response.is_truncated.unwrap_or(false))
    }

    /// Sleeps for the configured scanning interval.
    pub async fn sleep(&self) {
        tokio::time::sleep(self.scanning_interval).await;
    }
}

/// Represents a S3 scanner job that manages the lifecycle of a S3 scanner task.
pub struct S3Scanner {
    id: Uuid,
    cancel_token: CancellationToken,
    handle: tokio::task::JoinHandle<Result<()>>,
}

impl S3Scanner {
    /// Creates and spawns a new [`S3Scanner`] backed by a [`Task`].
    ///
    /// This function spawns a [`Task`]. The spawned task will periodically scan the configured S3
    /// bucket and prefix for new objects and send their metadata to the provided channel sender.
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
        id: Uuid,
        s3_client_manager: S3ClientManager,
        config: S3ScannerConfig,
        sender: mpsc::Sender<ObjectMetadata>,
    ) -> Self {
        let scanning_interval = Duration::from_secs(u64::from(config.scanning_interval_sec));
        let task = Task {
            s3_client_manager,
            scanning_interval,
            config,
            sender,
        };
        let cancel_token = CancellationToken::new();
        let child_cancel_token = cancel_token.clone();
        let handle = tokio::spawn(async move {
            task.run(child_cancel_token).await.inspect_err(|err| {
                tracing::error!(error = ? err, "S3 scanner task execution failed.");
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
    /// The UUID of this scanner.
    #[must_use]
    pub fn get_id(&self) -> String {
        self.id.to_string()
    }
}
