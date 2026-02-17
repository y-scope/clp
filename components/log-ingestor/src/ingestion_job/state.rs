use async_trait::async_trait;
use clp_rust_utils::s3::ObjectMetadata;

/// An abstract layer for managing ingestion job states.
#[async_trait]
pub trait IngestionJobState:
    SqsListenerState + S3ScannerState + Send + Sync + Clone + 'static {
    async fn start(&self) -> anyhow::Result<()>;

    async fn end(&self) -> anyhow::Result<()>;
}

/// An abstract layer for managing [`crate::ingestion_job::SqsListener`] states.
#[async_trait]
pub trait SqsListenerState: Send + Sync + Clone + 'static {
    async fn ingest(&self, objects: &[ObjectMetadata]) -> anyhow::Result<()>;
}

/// An abstract layer for managing [`crate::ingestion_job::S3Scanner`] states.
#[async_trait]
pub trait S3ScannerState: Send + Sync + Clone + 'static {
    async fn ingest(
        &self,
        objects: &[ObjectMetadata],
        last_ingested_key: &str,
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
    async fn ingest(
        &self,
        _objects: &[ObjectMetadata],
        _last_ingested_key: &str,
    ) -> anyhow::Result<()> {
        Ok(())
    }
}
