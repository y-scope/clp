use std::time::Duration;
use anyhow::Result;
use aws_sdk_s3::Client;
use clp_rust_utils::s3::ObjectMetadata;
use tokio::{select, sync::mpsc};
use tokio_util::sync::CancellationToken;
use crate::aws_client_manager::AwsClientManagerType;

/// Configuration for a S3 scanner job.
#[derive(Debug, Clone)]
pub struct S3ScannerConfig {
    pub bucket_name: String,
    pub prefix: String,
    pub scanning_interval: Duration,
    pub start_after: Option<String>,
}

/// Represents a S3 scanning task that periodically scans a given prefix under the bucket and fetch
/// object metadata.
///
/// # Type Parameters
///
/// * [`S3Client`]: The type of the AWS S3 client manager.
struct Task<S3Client: AwsClientManagerType<Client>> {
    s3_client_manager: S3Client,
    config: S3ScannerConfig,
    sender: mpsc::Sender<ObjectMetadata>,
}

impl<S3Client: AwsClientManagerType<Client> + 'static> Task<S3Client> {
    pub async fn run(mut self, cancel_token: CancellationToken) -> Result<()> {
        select! {
            // Cancellation requested.
            () = cancel_token.cancelled() => {
                return Ok(());
            }

            // Scanner execution
            result = self.scan() => {
                result
            }
        }
    }

    pub async fn scan(&mut self) -> Result<()> {
        loop {
            let client = self.s3_client_manager.get().await?;
            let response = client.list_objects_v2()
                .bucket(self.config.bucket_name.as_str())
                .prefix(self.config.prefix.as_str())
                .set_start_after(self.config.start_after.clone())
                .send().await?;
            let Some(contents) = response.contents else {
                self.sleep().await;
                continue
            };

            for content in contents {
                let (Some(key), Some(size)) = (content.key, content.size) else {
                    continue;
                };
                if (key.ends_with("/")) {
                    continue;
                }
                self.sender.send(ObjectMetadata {
                    bucket: self.config.bucket_name.clone(),
                    key: key.clone(),
                    size: size as usize,
                }).await?;
                self.config.start_after = Some(key);
            }

            if response.is_truncated.unwrap_or(false) {
                // The results are truncated. Keep going until all objects are listed.
                // Ideally, we can use the continuation token to continue listing objects, but since
                // we may refresh the client in the next scan cycle, we will use `start_after` to
                // send a new request for simplicity.
                continue;
            }
            self.sleep().await;
        }
    }

    pub async fn sleep(&self) {
        tokio::time::sleep(self.config.scanning_interval).await;
    }
}
