//! Serving a connection from a query task: the results it streams after its handshake, then the
//! trailer that the connection's acknowledgement answers.

use std::sync::Arc;

use bytes::BufMut;
use bytes::BytesMut;
use tokio::io::AsyncRead;
use tokio::io::AsyncReadExt;
use tokio::io::AsyncWrite;
use tokio::io::AsyncWriteExt;

use crate::cursor::Claim;
use crate::cursor::TaskCursor;
use crate::encoding::ResultEncoder;
use crate::encoding::ResultEncoding;
use crate::error::ConnectionError;
use crate::error::ProtocolError;
use crate::protocol::Ack;
use crate::protocol::Frame;
use crate::protocol::Handshake;
use crate::protocol::ResultFrame;
use crate::protocol::Trailer;
use crate::session::SessionStats;
use crate::session::State;

/// A connection that joined a session, streaming the results of one attempt of a query task.
///
/// # Type Parameters
///
/// * `EncodingType` - The encoding of the results the session delivers.
struct Connection<EncodingType: ResultEncoding> {
    session: Arc<State<EncodingType>>,
    cursor: Arc<TaskCursor>,
    encoder: EncodingType::Encoder,
    num_pending_results: u64,
    num_results: u64,
    trailer: Option<Trailer>,
    stats: SessionStats,
}

impl<EncodingType: ResultEncoding> Connection<EncodingType> {
    /// Factory function.
    ///
    /// Joins the connection to the task cursor of `session` that `handshake` names, creating the
    /// cursor if this is the task's first connection.
    ///
    /// # Returns
    ///
    /// The joined connection on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`ProtocolError::ArchiveIdMismatch`] if the cursor of the handshake's task is bound to
    ///   another archive.
    fn new(
        session: Arc<State<EncodingType>>,
        handshake: &Handshake,
    ) -> Result<Self, ProtocolError> {
        let cursor = Arc::clone(
            session
                .cursors
                .entry(handshake.task_index)
                .or_insert_with(|| Arc::new(TaskCursor::new(handshake.archive_id)))
                .value(),
        );
        let archive_id = cursor.archive_id();
        if archive_id != handshake.archive_id {
            return Err(ProtocolError::ArchiveIdMismatch {
                task_index: handshake.task_index,
                expected: archive_id,
                received: handshake.archive_id,
            });
        }
        Ok(Self {
            encoder: session.encoding.encoder(archive_id),
            session,
            cursor,
            num_pending_results: 0,
            num_results: 0,
            trailer: None,
            stats: SessionStats::default(),
        })
    }

    /// Claims the results the connection streams, encodes the results claimed from each socket
    /// read into one batch, and sends the batch to the session's consumer, or discards it once the
    /// consumer has dropped its receiver. Once the connection has sent its trailer and EOF, and
    /// its last batch has been sent or discarded, acknowledges its results.
    ///
    /// # Type Parameters
    ///
    /// * `StreamType` - The type of the connection's stream.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`ConnectionError::MissingTrailer`] if the connection closes between two frames before
    ///   sending its trailer.
    /// * [`ConnectionError::TruncatedFrame`] if the connection closes in the middle of a frame.
    /// * Forwards [`Connection::claim_buffered_frames`]'s return values on failure.
    /// * Forwards [`AsyncReadExt::read_buf`]'s return values on failure.
    /// * Forwards [`AsyncWriteExt::write_all`]'s return values on failure.
    async fn run<StreamType: AsyncRead + AsyncWrite + Unpin>(
        &mut self,
        stream: &mut StreamType,
        mut buffered: BytesMut,
    ) -> Result<(), ConnectionError> {
        const MAX_READ_SIZE: usize = 64 * 1024;

        loop {
            let claimed = self.claim_buffered_frames(&mut buffered);
            if 0 != self.num_pending_results {
                self.send_pending_batch().await;
            }
            claimed?;

            buffered.reserve(MAX_READ_SIZE);
            let mut read_window = (&mut buffered).limit(MAX_READ_SIZE);
            let num_bytes_read = stream.read_buf(&mut read_window).await?;
            if 0 != num_bytes_read {
                continue;
            }
            if let Some(trailer) = self.trailer {
                stream
                    .write_all(
                        &Ack {
                            num_results: trailer.num_results,
                        }
                        .encode(),
                    )
                    .await?;
                return Ok(());
            }
            if buffered.is_empty() {
                return Err(ConnectionError::MissingTrailer);
            }
            return Err(ConnectionError::TruncatedFrame);
        }
    }

    /// Decodes each complete frame in `buffer`, encoding the claimed results into the pending
    /// batch, and checks the trailer against the results the connection streamed.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`ProtocolError::DataAfterTrailer`] if any byte follows the trailer.
    /// * [`ProtocolError::ResultCountMismatch`] if the trailer doesn't count the results the
    ///   connection streamed.
    /// * Forwards [`Frame::decode`]'s return values on failure.
    /// * Forwards [`Connection::claim`]'s return values on failure.
    fn claim_buffered_frames(&mut self, buffer: &mut BytesMut) -> Result<(), ProtocolError> {
        loop {
            if self.trailer.is_some() {
                if buffer.is_empty() {
                    return Ok(());
                }
                return Err(ProtocolError::DataAfterTrailer);
            }
            let Some(frame) = Frame::decode(buffer)? else {
                return Ok(());
            };
            match frame {
                Frame::Result(result) => self.claim(&result)?,
                Frame::Trailer(trailer) => {
                    if trailer.num_results != self.num_results {
                        return Err(ProtocolError::ResultCountMismatch {
                            trailer_num_results: trailer.num_results,
                            num_results: self.num_results,
                        });
                    }
                    self.trailer = Some(trailer);
                }
            }
        }
    }

    /// Claims `result` on the task's cursor, encoding it into the pending batch unless an attempt
    /// of the task already emitted it.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`ProtocolError::UnexpectedResultIndex`] if `result` isn't the connection's next result.
    fn claim(&mut self, result: &ResultFrame) -> Result<(), ProtocolError> {
        if result.result_index != self.num_results {
            return Err(ProtocolError::UnexpectedResultIndex {
                expected: self.num_results,
                received: result.result_index,
            });
        }
        self.num_results += 1;
        if Claim::Duplicate == self.cursor.claim(result.result_index) {
            self.stats.num_duplicates_dropped += 1;
            return Ok(());
        }
        self.encoder
            .encode(result.timestamp_millisecs, &result.message);
        self.num_pending_results += 1;
        Ok(())
    }

    /// Sends the pending batch to the session's consumer, or discards it once the consumer has
    /// dropped its receiver.
    async fn send_pending_batch(&mut self) {
        let num_results = std::mem::take(&mut self.num_pending_results);
        if self
            .session
            .batches_sender
            .send(self.encoder.take_batch())
            .await
            .is_ok()
        {
            self.stats.num_results_emitted += num_results;
        } else {
            self.stats.num_results_discarded += num_results;
        }
    }
}

impl<EncodingType: ResultEncoding> Drop for Connection<EncodingType> {
    fn drop(&mut self) {
        self.session.record(&self.stats);
    }
}
