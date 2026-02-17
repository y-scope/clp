mod s3_scanner;
mod sqs_listener;

use anyhow::Result;
pub use s3_scanner::*;
pub use sqs_listener::*;

/// Enum for different types of ingestion jobs.
pub enum IngestionJob {
    S3Scanner(S3Scanner),
    SqsListener(SqsListener),
}

impl IngestionJob {
    /// Shuts down and waits for the job to complete.
    ///
    /// # Returns
    ///
    /// `Ok(())` on success.
    ///
    /// # Errors
    ///
    /// Returns an error if the underlying job fails to shut down properly:
    ///
    /// * Forwards [`S3Scanner::shutdown_and_join`]'s return value on failure.
    pub async fn shutdown_and_join(self) -> Result<()> {
        match self {
            Self::S3Scanner(s3_scanner) => s3_scanner.shutdown_and_join().await,
            Self::SqsListener(sqs_listener) => {
                let _: () = sqs_listener.shutdown_and_join().await;
                Ok(())
            }
        }
    }

    /// # Returns
    ///
    /// The UUID of the job.
    #[must_use]
    pub fn get_id(&self) -> String {
        match self {
            Self::S3Scanner(scanner) => scanner.get_id(),
            Self::SqsListener(listener) => listener.get_id(),
        }
    }
}

impl From<S3Scanner> for IngestionJob {
    fn from(scanner: S3Scanner) -> Self {
        Self::S3Scanner(scanner)
    }
}

impl From<SqsListener> for IngestionJob {
    fn from(listener: SqsListener) -> Self {
        Self::SqsListener(listener)
    }
}
