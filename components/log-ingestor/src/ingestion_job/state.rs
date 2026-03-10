use async_trait::async_trait;
use clp_rust_utils::s3::ObjectMetadata;
use tokio::sync::mpsc;

/// An abstract, job-type-agnostic layer for managing ingestion job states.
#[async_trait]
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
    async fn start(&self) -> anyhow::Result<()>;

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
    async fn end(&self) -> anyhow::Result<()>;

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
    async fn fail(&self, msg: String);
}

/// An abstract layer for managing [`crate::ingestion_job::SqsListener`] states.
#[async_trait]
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
    async fn ingest(&self, objects: Vec<ObjectMetadata>) -> anyhow::Result<()>;
}

/// An abstract layer for managing [`crate::ingestion_job::S3Scanner`] states.
#[async_trait]
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
    async fn ingest(
        &self,
        objects: Vec<ObjectMetadata>,
        last_ingested_key: &str,
    ) -> anyhow::Result<()>;
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

#[async_trait]
impl IngestionJobState for ZeroFaultToleranceIngestionJobState {
    async fn start(&self) -> anyhow::Result<()> {
        Ok(())
    }

    async fn end(&self) -> anyhow::Result<()> {
        Ok(())
    }

    async fn fail(&self, _msg: String) {}
}

#[async_trait]
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

#[async_trait]
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
