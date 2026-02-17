use async_trait::async_trait;
use clp_rust_utils::s3::ObjectMetadata;

/// An abstract layer for managing ingestion job states.
#[async_trait]
pub trait IngestionJobState:
    SqsListenerState + S3ScannerState + Send + Sync + Clone + 'static {
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
    async fn ingest(&self, objects: &[ObjectMetadata]) -> anyhow::Result<()>;
}

/// An abstract layer for managing [`crate::ingestion_job::S3Scanner`] states.
#[async_trait]
pub trait S3ScannerState: Send + Sync + Clone + 'static {
    /// Ingests the given object metadata into CLP and marks them as `Buffered`.
    ///
    /// # Lifetimes
    ///
    /// * `'object_metadata_lifetime`: Both `objects` and `last_ingested_key` must remain valid for
    ///   the same lifetime.
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
    async fn ingest<'object_metadata_lifetime>(
        &self,
        objects: &'object_metadata_lifetime [ObjectMetadata],
        last_ingested_key: &'object_metadata_lifetime str,
    ) -> anyhow::Result<()>;
}

/// An ingestion job state implementation that does nothing.
#[derive(Clone, Default)]
pub struct NoopIngestionJobState {}

#[async_trait]
impl IngestionJobState for NoopIngestionJobState {
    async fn start(&self) -> anyhow::Result<()> {
        Ok(())
    }

    async fn end(&self) -> anyhow::Result<()> {
        Ok(())
    }
}

#[async_trait]
impl SqsListenerState for NoopIngestionJobState {
    async fn ingest(&self, _objects: &[ObjectMetadata]) -> anyhow::Result<()> {
        Ok(())
    }
}

#[async_trait]
impl S3ScannerState for NoopIngestionJobState {
    async fn ingest<'object_metadata_lifetime>(
        &self,
        _objects: &'object_metadata_lifetime [ObjectMetadata],
        _last_ingested_key: &'object_metadata_lifetime str,
    ) -> anyhow::Result<()> {
        Ok(())
    }
}
