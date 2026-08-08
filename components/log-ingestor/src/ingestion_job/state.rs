use std::future::Future;

use clp_rust_utils::s3::ObjectMetadata;
use tokio::sync::mpsc;

/// An abstract, job-type-agnostic layer for managing ingestion job states.
pub trait IngestionJobState: Send + Sync + Clone + 'static {
    /// Starts the ingestion job.
    ///
    /// # Returns
    ///
    /// `Ok(())` on success.
    ///
    /// # Errors
    ///
    /// Implementations **must document** the specific error variants they may return and the
    /// conditions under which those errors occur.
    fn start(&self) -> impl Future<Output = anyhow::Result<()>> + Send;

    /// Ends the ingestion job.
    ///
    /// # Returns
    ///
    /// `Ok(())` on success.
    ///
    /// # Errors
    ///
    /// Implementations **must document** the specific error variants they may return and the
    /// conditions under which those errors occur.
    fn end(&self) -> impl Future<Output = anyhow::Result<()>> + Send;

    /// Fails the ingestion job.
    ///
    /// # Parameters
    ///
    /// * `msg`: A message describing the failure reason.
    ///
    /// # NOTE
    ///
    /// Implementations should not propagate errors produced while failing the job. If an error
    /// occurs, it should be logged and otherwise ignored, so the caller can prioritize propagating
    /// the *original* error that triggered the failure over any secondary error from this method.
    fn fail(&self, msg: String) -> impl Future<Output = ()> + Send;
}

/// An abstract layer for managing [`crate::ingestion_job::SqsListener`] states.
pub trait SqsListenerState: Send + Sync + Clone + 'static {
    /// Ingests the given object metadata into CLP and marks them as `Buffered`.
    ///
    /// # Parameters
    ///
    /// * `objects`: The object metadata to ingest.
    ///
    /// # Returns
    ///
    /// `Ok(())` on success.
    ///
    /// # Errors
    ///
    /// Implementations **must document** the specific error variants they may return and the
    /// conditions under which those errors occur.
    fn ingest(
        &self,
        objects: Vec<ObjectMetadata>,
    ) -> impl Future<Output = anyhow::Result<()>> + Send;
}

/// An abstract layer for managing [`crate::ingestion_job::S3Scanner`] states.
pub trait S3ScannerState: Send + Sync + Clone + 'static {
    /// Ingests the given object metadata into CLP and marks them as `Buffered`.
    ///
    /// # Parameters
    ///
    /// * `objects`: The object metadata to ingest.
    /// * `last_ingested_key`: The last ingested S3 object key.
    ///
    /// # Returns
    ///
    /// `Ok(())` on success.
    ///
    /// # Errors
    ///
    /// Implementations **must document** the specific error variants they may return and the
    /// conditions under which those errors occur.
    fn ingest(
        &self,
        objects: Vec<ObjectMetadata>,
        last_ingested_key: &str,
    ) -> impl Future<Output = anyhow::Result<()>> + Send;
}

/// An ingestion job state implementation that has no fault-tolerance.
#[derive(Clone)]
pub struct ZeroFaultToleranceIngestionJobState {
    sender: mpsc::Sender<Vec<ObjectMetadata>>,
}

impl ZeroFaultToleranceIngestionJobState {
    #[must_use]
    pub const fn new(sender: mpsc::Sender<Vec<ObjectMetadata>>) -> Self {
        Self { sender }
    }
}

impl IngestionJobState for ZeroFaultToleranceIngestionJobState {
    fn start(&self) -> impl Future<Output = anyhow::Result<()>> + Send {
        std::future::ready(Ok(()))
    }

    fn end(&self) -> impl Future<Output = anyhow::Result<()>> + Send {
        std::future::ready(Ok(()))
    }

    fn fail(&self, _msg: String) -> impl Future<Output = ()> + Send {
        std::future::ready(())
    }
}

impl SqsListenerState for ZeroFaultToleranceIngestionJobState {
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`mpsc::Sender::send`]'s return values on failure.
    async fn ingest(&self, objects: Vec<ObjectMetadata>) -> anyhow::Result<()> {
        self.sender.send(objects).await?;
        Ok(())
    }
}

impl S3ScannerState for ZeroFaultToleranceIngestionJobState {
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`mpsc::Sender::send`]'s return values on failure.
    async fn ingest(
        &self,
        objects: Vec<ObjectMetadata>,
        _last_ingested_key: &str,
    ) -> anyhow::Result<()> {
        self.sender.send(objects).await?;
        Ok(())
    }
}
