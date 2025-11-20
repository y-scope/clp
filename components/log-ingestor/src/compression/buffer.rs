use anyhow::Result;
use async_trait::async_trait;
use clp_rust_utils::s3::ObjectMetadata;

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
    async fn submit(&self, buffer: &[ObjectMetadata]) -> Result<()>;
}

/// A buffer that accumulates object metadata and submits it when a size threshold is reached.
///
/// # Type Parameters:
///
/// * [`Submitter`]: A type that implements the [`BufferSubmitter`] trait for submitting buffered
///   data.
pub struct Buffer<Submitter: BufferSubmitter> {
    submitter: Submitter,
    buf: Vec<ObjectMetadata>,
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

    /// Adds object metadata to the buffer and submits if the size threshold is reached.
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
    pub async fn add(&mut self, object_metadata: ObjectMetadata) -> Result<()> {
        self.total_size += object_metadata.size;
        self.buf.push(object_metadata);

        if self.total_size >= self.size_threshold {
            self.submit().await?;
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
