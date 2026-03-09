use anyhow::Result;
use async_trait::async_trait;
use clp_rust_utils::s3::S3ObjectMetadataId;
use sqlx::FromRow;

/// A reference to a persisted S3 object metadata row
#[derive(Debug, Clone, PartialEq, Eq, FromRow)]
pub struct CompressionBufferEntry {
    pub id: S3ObjectMetadataId,
    pub size: u64,
}

#[async_trait]
/// A trait for submitting buffered object metadata for processing.
pub trait BufferSubmitter {
    /// Submits the buffered object metadata for processing.
    ///
    /// # Returns:
    ///
    /// `Ok(())` on success.
    ///
    /// # Errors:
    ///
    /// Returns an [`anyhow::Error`] on failure.
    async fn submit(&self, buffer: &[S3ObjectMetadataId]) -> Result<()>;
}

/// A buffer that accumulates object metadata IDs and a running total size, and submits when the
/// size threshold is reached.
///
/// # Type Parameters:
///
/// * [`Submitter`]: A type that implements the [`BufferSubmitter`] trait for submitting buffered
///   data.
pub struct Buffer<Submitter: BufferSubmitter> {
    submitter: Submitter,
    buf: Vec<S3ObjectMetadataId>,
    total_size: u64,
    size_threshold: u64,
}

impl<Submitter: BufferSubmitter> Buffer<Submitter> {
    /// Factory function.
    ///
    /// # Returns
    ///
    /// A newly created [`Buffer`] with the given submitter of type `T` and size threshold.
    pub const fn new(submitter: Submitter, size_threshold: u64) -> Self {
        Self {
            submitter,
            buf: Vec::new(),
            total_size: 0,
            size_threshold,
        }
    }

    /// Adds object metadata refs to the buffer and submits if the size threshold is reached.
    ///
    /// # Returns
    ///
    /// `Ok(())` on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`Self::submit`]'s return values on failure.
    pub async fn add(
        &mut self,
        object_metadata_to_ingest: Vec<CompressionBufferEntry>,
    ) -> Result<()> {
        for ref_ in object_metadata_to_ingest {
            self.total_size += ref_.size;
            self.buf.push(ref_.id);

            if self.total_size >= self.size_threshold {
                self.submit().await?;
            }
        }

        Ok(())
    }

    /// Submits the buffered object metadata for processing.
    ///
    /// # Returns
    ///
    /// `Ok(())` on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`Submitter::submit`]'s return values on failure.
    pub async fn submit(&mut self) -> Result<()> {
        if self.buf.is_empty() {
            return Ok(());
        }
        self.submitter.submit(&self.buf).await?;
        self.clear();
        Ok(())
    }

    fn clear(&mut self) {
        self.buf.clear();
        self.total_size = 0;
    }
}
