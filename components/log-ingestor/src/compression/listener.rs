use std::{pin::Pin, time::Duration};

use anyhow::Result;
use clp_rust_utils::s3::ObjectMetadata;
use tokio::{
    select,
    sync::mpsc,
    time::{Instant, Sleep, sleep_until},
};
use tokio_util::sync::CancellationToken;

use crate::compression::{Buffer, BufferSubmitter};

/// Represents a listener task that buffers and submits S3 object metadata.
///
/// # Type Parameters
/// * `Submitter`: A type that implements the [`BufferSubmitter`] trait.
struct ListenerTask<Submitter: BufferSubmitter> {
    buffer: Buffer<Submitter>,
    timeout: Duration,
    receiver: mpsc::Receiver<Vec<ObjectMetadata>>,
}

impl<Submitter: BufferSubmitter + Send + 'static> ListenerTask<Submitter> {
    /// Runs the listener task to buffer and submit S3 object metadata. Submission can be triggered
    /// in three ways:
    /// * Receiving a cancellation signal via the provided [`cancel_token`].
    /// * The buffer capacity is reached after receiving a new object metadata.
    /// * A timeout occurs without receiving new object metadata.
    ///
    /// # Returns
    ///
    /// `Ok(())` on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`Buffer::add`]'s return values on failure.
    /// * Forwards [`Buffer::submit`]'s return values on failure.
    pub async fn run(mut self, cancel_token: CancellationToken) -> Result<()> {
        let mut timer: Pin<Box<Sleep>> = Box::pin(sleep_until(Instant::now() + self.timeout));

        loop {
            select! {
                // Cancellation requested.
                () = cancel_token.cancelled() => {
                    // TODO: Log the cancellation event when the logger has been integrated.
                    self.buffer.submit().await?;
                    return Ok(());
                }

                // New object metadata received.
                optional_object_metadata = self.receiver.recv() => {
                    match optional_object_metadata {
                        None => {
                            self.buffer.submit().await?;
                            return Err(
                                anyhow::anyhow!("Listener channel has been closed unexpectedly")
                            );
                        }
                        Some(object_metadata_to_ingest) => {
                            self.buffer.add(object_metadata_to_ingest).await?;
                        }
                    }
                }

                // Timer fired.
                () = &mut timer => {
                    self.buffer.submit().await?;
                }
            }

            timer.as_mut().reset(Instant::now() + self.timeout);
        }
    }
}

/// Represents a listener that accepts S3 object metadata from multiple senders and buffers them
/// for submission.
pub struct Listener {
    sender: mpsc::Sender<Vec<ObjectMetadata>>,
    cancel_token: CancellationToken,
    handle: tokio::task::JoinHandle<Result<()>>,
}

impl Listener {
    /// Creates and spawns a new [`Listener`] backed by a [`ListenerTask`].
    ///
    /// This function spawns a [`ListenerTask`]. The spawned task will buffer incoming
    /// [`ObjectMetadata`] values and call the supplied `Submitter` when either the buffer's
    /// threshold is reached or the configured `timeout` fires.
    ///
    /// # Type parameters
    ///
    /// * [`Submitter`]: A type that implements the [`BufferSubmitter`] trait to submit buffered
    ///   object metadata.
    ///
    /// # Returns
    ///
    /// A newly created instance of [`Listener`].
    pub fn spawn<Submitter: BufferSubmitter + Send + 'static>(
        buffer: Buffer<Submitter>,
        timeout: Duration,
        channel_capacity: usize,
    ) -> Self {
        let (sender, receiver) = mpsc::channel(channel_capacity);
        let task = ListenerTask {
            buffer,
            timeout,
            receiver,
        };
        let cancel_token = CancellationToken::new();
        let child_cancel_token = cancel_token.clone();
        let handle = tokio::spawn(async move {
            task.run(child_cancel_token).await.inspect_err(|err| {
                tracing::error!(error = ? err, "Listener task execution failed.");
            })
        });

        Self {
            sender,
            cancel_token,
            handle,
        }
    }

    /// Shuts down the listener and waits for the underlying task to complete.
    ///
    /// # Returns
    ///
    /// `Ok(())` on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards the underlying task's return values on failure ([`ListenerTask::run`]).
    pub async fn shutdown_and_join(self) -> Result<()> {
        self.cancel_token.cancel();
        self.handle.await?
    }

    /// # Returns
    /// A new `mpsc::Sender<Vec<ObjectMetadata>>` that can be used to send metadata to this
    /// listener.
    ///
    /// The returned sender is a cheap clone of the listener's internal channel sender. It can be
    /// freely cloned and moved to other tasks; multiple senders may concurrently send to the same
    /// listener. Messages sent by a single sender preserve order; messages from different senders
    /// are interleaved in the order they are received by the runtime.
    #[must_use]
    pub fn get_new_sender(&self) -> mpsc::Sender<Vec<ObjectMetadata>> {
        self.sender.clone()
    }
}
