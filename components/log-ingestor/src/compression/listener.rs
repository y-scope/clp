use std::time::Duration;

use anyhow::Result;
use tokio::{
    select,
    sync::mpsc,
    time::{Instant, sleep_until},
};
use tokio_util::sync::CancellationToken;

use crate::compression::{Buffer, BufferSubmitter, CompressionBufferEntry};

/// Represents a listener task that buffers incoming [`CompressionBufferEntry`] values and submits
/// when a certain size threshold is reached or on timeout.
///
/// # Type Parameters
/// * `Submitter`: A type that implements the [`BufferSubmitter`] trait.
struct ListenerTask<Submitter: BufferSubmitter> {
    buffer: Buffer<Submitter>,
    timeout: Duration,
    receiver: mpsc::Receiver<Vec<CompressionBufferEntry>>,
}

impl<Submitter: BufferSubmitter + Send + 'static> ListenerTask<Submitter> {
    /// Runs the listener task to buffer and submit [`CompressionBufferEntry`] values. Submission
    /// can be triggered if:
    ///
    /// * Receiving a cancellation signal via the provided [`cancel_token`].
    /// * All senders are closed.
    /// * The buffer's size threshold is reached after receiving new entries.
    /// * The oldest buffered entry has remained in the buffer for at least the configured timeout
    ///   duration. The timer starts when entries are first added to an empty buffer and is not
    ///   reset by subsequent entries.
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
        // To avoid reallocating the timer on every reset, we preallocate and pin it. A boolean flag
        // is used to track whether the timer is active, which defaults to false since the buffer is
        // initially empty.
        let timer = sleep_until(Instant::now() + self.timeout);
        tokio::pin!(timer);
        let mut timer_active = false;

        loop {
            select! {
                // Cancellation requested.
                () = cancel_token.cancelled() => {
                    tracing::info!("Listener task cancelled.");
                    self.buffer.submit().await?;
                    return Ok(());
                }

                // New object metadata entries received.
                optional_entries = self.receiver.recv() => {
                    match optional_entries {
                        None => {
                            self.buffer.submit().await?;
                            tracing::info!(
                                "All senders have been dropped. The channel will be closed."
                            );
                            return Ok(());
                        }
                        Some(entries) => {
                            let was_empty = self.buffer.is_empty();
                            let flushed = self.buffer.add(entries).await?;
                            let is_empty = self.buffer.is_empty();

                            if is_empty {
                                timer_active = false;
                            } else if was_empty || flushed {
                                // Enable the timer if new objects are added to an empty buffer, or
                                // a flush is triggered but the buffer is not empty after flushing.
                                // In both cases, the timer should be reset.
                                timer.as_mut().reset(Instant::now() + self.timeout);
                                timer_active = true;
                            }
                        }
                    }
                }

                // Timer fired.
                () = &mut timer, if timer_active => {
                    self.buffer.submit().await?;
                    // The timer will be re-enabled when new entries are received.
                    timer_active = false;
                }
            }
        }
    }
}

/// Represents a listener that accepts [`CompressionBufferEntry`] values from multiple senders and
/// buffers them for submission.
pub struct Listener {
    sender: mpsc::Sender<Vec<CompressionBufferEntry>>,
    cancel_token: CancellationToken,
    handle: tokio::task::JoinHandle<Result<()>>,
}

impl Listener {
    /// Creates and spawns a new [`Listener`] backed by a [`ListenerTask`].
    ///
    /// This function spawns a [`ListenerTask`]. The spawned task will buffer incoming
    /// [`CompressionBufferEntry`] values and call the supplied `Submitter` when either the
    /// buffer's threshold is reached or the configured `timeout` fires.
    ///
    /// # Type parameters
    ///
    /// * [`Submitter`]: A type that implements the [`BufferSubmitter`] trait to submit buffered
    ///   object metadata.
    ///
    /// # Returns
    ///
    /// A newly created instance of [`Listener`].
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`anyhow::Error`] if `channel_capacity` is 0, since [`mpsc::channel`] requires a positive
    ///   capacity.
    pub fn spawn<Submitter: BufferSubmitter + Send + 'static>(
        buffer: Buffer<Submitter>,
        timeout: Duration,
        channel_capacity: usize,
    ) -> Result<Self> {
        if channel_capacity == 0 {
            anyhow::bail!("channel capacity must be greater than 0");
        }
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

        Ok(Self {
            sender,
            cancel_token,
            handle,
        })
    }

    /// Shuts down the listener and waits for the underlying task to complete.
    pub async fn shutdown_and_join(self) {
        self.cancel_token.cancel();
        match self.handle.await {
            Ok(Ok(())) => {
                tracing::info!("Listener shutdown successfully.");
            }
            Ok(Err(_)) => {
                // We don't need to log the error here because the underlying coroutine will log it.
                tracing::warn!("Listener shutdown with an error.");
            }
            Err(err) => {
                tracing::warn!(error = ? err, "Listener panicked.");
            }
        }
    }

    /// # Returns
    /// A new `mpsc::Sender<Vec<CompressionBufferEntry>>` that can be used to send
    /// [`CompressionBufferEntry`] values to this listener.
    ///
    /// The returned sender is a cheap clone of the listener's internal channel sender. It can be
    /// freely cloned and moved to other tasks; multiple senders may concurrently send to the same
    /// listener. Messages sent by a single sender preserve order; messages from different senders
    /// are interleaved in the order they are received by the runtime.
    #[must_use]
    pub fn get_new_sender(&self) -> mpsc::Sender<Vec<CompressionBufferEntry>> {
        self.sender.clone()
    }
}
