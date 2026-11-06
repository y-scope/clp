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
/// * `EncoderType` - The encoder of the connection's results.
pub struct Connection<EncoderType: ResultEncoder> {
    session: Arc<State<EncoderType>>,
    cursor: Arc<TaskCursor>,
    encoder: EncoderType,
    num_pending_results: u64,
    num_results: u64,
    stats: SessionStats,
}

impl<EncoderType: ResultEncoder> Connection<EncoderType> {
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
    pub fn new(
        session: Arc<State<EncoderType>>,
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
            encoder: EncoderType::new(archive_id),
            session,
            cursor,
            num_pending_results: 0,
            num_results: 0,
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
    /// * Forwards [`Connection::decode_buffered_frames`]'s return values on failure.
    /// * Forwards [`read_more`]'s return values on failure.
    /// * Forwards [`check_eof`]'s return values on failure.
    /// * Forwards [`AsyncWriteExt::write_all`]'s return values on failure.
    pub async fn run<StreamType: AsyncRead + AsyncWrite + Unpin>(
        &mut self,
        input_stream: &mut StreamType,
        mut buffer: BytesMut,
    ) -> Result<(), ConnectionError> {
        let trailer = loop {
            let decoded = self.decode_buffered_frames(&mut buffer);
            // Hand on the results claimed so far before acting on a protocol error found later in
            // the same read, since no other attempt can claim them again.
            self.send_pending_batch().await;
            match decoded? {
                Decoded::NeedMoreBytes => {
                    if 0 == read_more(input_stream, &mut buffer).await? {
                        return Err(if buffer.is_empty() {
                            ConnectionError::MissingTrailer
                        } else {
                            ConnectionError::TruncatedFrame
                        });
                    }
                }
                Decoded::Trailer(trailer) => break trailer,
            }
        };
        check_eof(input_stream, &mut buffer).await?;
        input_stream
            .write_all(
                &Ack {
                    num_results: trailer.num_results,
                }
                .encode(),
            )
            .await?;
        Ok(())
    }

    /// Decodes the complete frames in `buffer` up to the trailer, encoding the claimed results into
    /// the pending batch, and checks the trailer against the results the connection streamed.
    ///
    /// # Returns
    ///
    /// On success:
    ///
    /// * [`Decoded::Trailer`] if `buffer` holds the connection's trailer, leaving the bytes that
    ///   follow it in `buffer`.
    /// * [`Decoded::NeedMoreBytes`] if `buffer` ends before the next frame does.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`ProtocolError::ResultCountMismatch`] if the trailer doesn't count the results the
    ///   connection streamed.
    /// * Forwards [`Frame::decode`]'s return values on failure.
    /// * Forwards [`Connection::claim`]'s return values on failure.
    fn decode_buffered_frames(&mut self, buffer: &mut BytesMut) -> Result<Decoded, ProtocolError> {
        while let Some(frame) = Frame::decode(buffer)? {
            match frame {
                Frame::Result(result) => self.claim(&result)?,
                Frame::Trailer(trailer) => {
                    if trailer.num_results != self.num_results {
                        return Err(ProtocolError::ResultCountMismatch {
                            trailer_num_results: trailer.num_results,
                            num_results: self.num_results,
                        });
                    }
                    return Ok(Decoded::Trailer(trailer));
                }
            }
        }
        Ok(Decoded::NeedMoreBytes)
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

    /// Sends the pending batch, if it holds any result, to the session's consumer, or discards it
    /// once the consumer has dropped its receiver.
    async fn send_pending_batch(&mut self) {
        if 0 == self.num_pending_results {
            return;
        }
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

impl<EncoderType: ResultEncoder> Drop for Connection<EncoderType> {
    fn drop(&mut self) {
        self.session.record(&self.stats);
    }
}

const MAX_READ_SIZE: usize = 64 * 1024;

/// What decoding a connection's buffered frames found.
enum Decoded {
    /// The buffer ends before the next frame does, so the connection must read more bytes.
    NeedMoreBytes,

    /// The connection sent its trailer, which counts the results it streamed.
    Trailer(Trailer),
}

/// Reads at most [`MAX_READ_SIZE`] more bytes from `input_stream` into `buffer`.
///
/// # Type Parameters
///
/// * `StreamType` - The type of the connection's stream.
///
/// # Returns
///
/// The number of bytes read on success, which is 0 once `input_stream` has reached EOF.
///
/// # Errors
///
/// Returns an error if:
///
/// * Forwards [`AsyncReadExt::read_buf`]'s return values on failure.
async fn read_more<StreamType: AsyncRead + Unpin>(
    input_stream: &mut StreamType,
    buffer: &mut BytesMut,
) -> std::io::Result<usize> {
    buffer.reserve(MAX_READ_SIZE);
    let mut read_window = (&mut *buffer).limit(MAX_READ_SIZE);
    input_stream.read_buf(&mut read_window).await
}

/// Checks that no bytes follow the connection's trailer, either left in `buffer` or read next from
/// `input_stream`.
///
/// # Type Parameters
///
/// * `StreamType` - The type of the connection's stream.
///
/// # Errors
///
/// Returns an error if:
///
/// * [`ConnectionError::Protocol`] wrapping [`ProtocolError::DataAfterTrailer`] if any byte follows
///   the trailer.
/// * Forwards [`read_more`]'s return values on failure.
async fn check_eof<StreamType: AsyncRead + Unpin>(
    input_stream: &mut StreamType,
    buffer: &mut BytesMut,
) -> Result<(), ConnectionError> {
    if !buffer.is_empty() || 0 != read_more(input_stream, buffer).await? {
        return Err(ProtocolError::DataAfterTrailer.into());
    }
    Ok(())
}

#[cfg(test)]
mod tests {
    use std::sync::Arc;
    use std::time::Duration;

    use clp_rust_utils::types::ArchiveId;
    use spider_core::task::TaskIndex;
    use tokio::io::AsyncReadExt;
    use tokio::io::AsyncWriteExt;
    use tokio::io::DuplexStream;
    use tokio::sync::mpsc;
    use tokio::task::JoinHandle;
    use tokio::time::timeout;

    use super::Connection;
    use crate::error::ConnectionError;
    use crate::error::ProtocolError;
    use crate::protocol::Ack;
    use crate::session::SessionStats;
    use crate::session::State;
    use crate::test_utils::QueryResult;
    use crate::test_utils::QueryResultEncoder;
    use crate::test_utils::encode_result;
    use crate::test_utils::encode_trailer;

    const ARCHIVE_ID: &str = "018e90e5-8b2a-4a61-a2fc-cac799936caf";

    /// The number of batches a session's channel holds, unless a test needs it to fill up.
    const CHANNEL_CAPACITY: usize = 1024;

    /// The longest a test waits for a connection handler before failing.
    const TIMEOUT: Duration = Duration::from_secs(10);

    /// A session's state, the receiver of the batches its connections send, and the number of bytes
    /// each direction of its connections' in-memory streams buffers.
    struct Harness {
        state: Arc<State<QueryResultEncoder>>,
        batches_receiver: Option<mpsc::Receiver<Vec<QueryResult>>>,
        stream_capacity: usize,
    }

    impl Harness {
        /// Creates a session whose channel holds `channel_capacity` batches.
        ///
        /// # Returns
        ///
        /// The newly created harness.
        fn new(channel_capacity: usize) -> Self {
            /// The number of bytes each direction of an in-memory stream buffers, unless a test
            /// needs more.
            const STREAM_CAPACITY: usize = 64 * 1024;

            let (batches_sender, batches_receiver) = mpsc::channel(channel_capacity);
            Self {
                state: Arc::new(State::new(batches_sender)),
                batches_receiver: Some(batches_receiver),
                stream_capacity: STREAM_CAPACITY,
            }
        }

        /// Connects an attempt of task `task_index` of archive `archive_id` to the session.
        ///
        /// # Returns
        ///
        /// The attempt, whose connection handler runs in the background.
        ///
        /// # Panics
        ///
        /// Panics if `archive_id` isn't a valid UUID, or the cursor of task `task_index` is bound
        /// to another archive.
        fn connect(&self, task_index: TaskIndex, archive_id: &str) -> Attempt {
            self.connect_with_buffered(task_index, archive_id, &[])
        }

        /// Connects an attempt of task `task_index` of archive `archive_id` to the session, as if
        /// `buffered` had been read along with the attempt's handshake.
        ///
        /// # Returns
        ///
        /// The attempt, whose connection handler runs in the background.
        ///
        /// # Panics
        ///
        /// Panics if `archive_id` isn't a valid UUID, or the cursor of task `task_index` is bound
        /// to another archive.
        fn connect_with_buffered(
            &self,
            task_index: TaskIndex,
            archive_id: &str,
            buffered: &[u8],
        ) -> Attempt {
            use bytes::BytesMut;

            let (stream, mut handler_stream) = tokio::io::duplex(self.stream_capacity);
            let mut connection = self
                .join(task_index, archive_id)
                .expect("the attempt should join the session");
            let buffered = BytesMut::from(buffered);
            let handler =
                tokio::spawn(async move { connection.run(&mut handler_stream, buffered).await });
            Attempt { stream, handler }
        }

        /// Joins a connection of an attempt of task `task_index` of archive `archive_id` to the
        /// session.
        ///
        /// # Returns
        ///
        /// The joined connection on success.
        ///
        /// # Errors
        ///
        /// Returns an error if:
        ///
        /// * Forwards [`Connection::new`]'s return values on failure.
        ///
        /// # Panics
        ///
        /// Panics if `archive_id` isn't a valid UUID.
        fn join(
            &self,
            task_index: TaskIndex,
            archive_id: &str,
        ) -> Result<Connection<QueryResultEncoder>, ProtocolError> {
            use clp_rust_utils::types::SessionToken;

            use crate::protocol::Handshake;

            const SESSION_TOKEN: &str = "6f1d3b52-8a4e-4c1b-9f6e-2d7a5c0b9e13";

            let handshake = Handshake {
                session_token: SESSION_TOKEN
                    .parse::<SessionToken>()
                    .expect("session token fixture should be a valid UUID"),
                task_index,
                archive_id: archive_id
                    .parse::<ArchiveId>()
                    .expect("archive ID fixtures should be valid UUIDs"),
            };
            Connection::new(Arc::clone(&self.state), &handshake)
        }

        /// # Returns
        ///
        /// The next batch the session's connections send.
        ///
        /// # Panics
        ///
        /// Panics if the test has dropped the receiver, or no batch arrives within [`TIMEOUT`].
        async fn next_batch(&mut self) -> Vec<QueryResult> {
            timeout(TIMEOUT, self.batches_receiver().recv())
                .await
                .expect("a batch should arrive")
                .expect("the session's state should keep the channel open")
        }

        /// # Returns
        ///
        /// The batches the session's connections have sent that the test hasn't received yet.
        ///
        /// # Panics
        ///
        /// Panics if the test has dropped the receiver.
        fn take_batches(&mut self) -> Vec<Vec<QueryResult>> {
            let batches_receiver = self.batches_receiver();
            let mut batches = Vec::new();
            while let Ok(batch) = batches_receiver.try_recv() {
                batches.push(batch);
            }
            batches
        }

        /// # Returns
        ///
        /// The receiver of the session's batches.
        ///
        /// # Panics
        ///
        /// Panics if the test has dropped the receiver.
        fn batches_receiver(&mut self) -> &mut mpsc::Receiver<Vec<QueryResult>> {
            self.batches_receiver
                .as_mut()
                .expect("the receiver shouldn't have been dropped")
        }
    }

    /// A simulated attempt of a `clp-s` query task, streaming its results to a connection handler
    /// that runs in the background.
    struct Attempt {
        stream: DuplexStream,
        handler: JoinHandle<Result<(), ConnectionError>>,
    }

    impl Attempt {
        /// Sends `bytes` as they are.
        ///
        /// # Panics
        ///
        /// Panics if the handler has closed the connection.
        async fn send(&mut self, bytes: &[u8]) {
            self.stream
                .write_all(bytes)
                .await
                .expect("the handler should keep the connection open");
        }

        /// Sends the results at `result_indices` of task `task_index` in a single write.
        ///
        /// # Panics
        ///
        /// Panics if the handler has closed the connection.
        async fn send_results(
            &mut self,
            task_index: TaskIndex,
            result_indices: impl IntoIterator<Item = u64>,
        ) {
            self.send(&encode_results(task_index, result_indices)).await;
        }

        /// Shuts down the sending side of the connection, as `clp-s` does after its trailer.
        ///
        /// # Panics
        ///
        /// Panics if the shutdown fails.
        async fn shut_down_write(&mut self) {
            self.stream
                .shutdown()
                .await
                .expect("shutting down an in-memory stream should succeed");
        }

        /// Checks that the handler keeps the connection open without sending anything.
        ///
        /// # Panics
        ///
        /// Panics if the handler closes the connection or sends anything during the check.
        async fn expect_open(&mut self) {
            const OPEN_CHECK_DURATION: Duration = Duration::from_millis(100);

            let mut byte = [0_u8; 1];
            let read_result = timeout(OPEN_CHECK_DURATION, self.stream.read(&mut byte)).await;
            assert!(
                read_result.is_err(),
                "the handler shouldn't end the connection or send anything yet: {read_result:?}"
            );
        }

        /// Ends the stream the way `clp-s` does: sends a trailer counting `num_results` results,
        /// then EOF, and waits until the handler closes the connection.
        ///
        /// # Returns
        ///
        /// How the connection ended.
        ///
        /// # Panics
        ///
        /// Panics if the handler closes the connection before the trailer, doesn't close it within
        /// [`TIMEOUT`] after EOF, or panics.
        async fn finish(mut self, num_results: u64) -> ConnectionEnd {
            self.send(&encode_trailer(num_results)).await;
            self.shut_down_write().await;
            self.wait_closed().await
        }

        /// Waits until the handler closes the connection.
        ///
        /// # Returns
        ///
        /// How the connection ended.
        ///
        /// # Panics
        ///
        /// Panics if the handler doesn't close the connection within [`TIMEOUT`], or panics.
        async fn wait_closed(mut self) -> ConnectionEnd {
            let mut received = Vec::new();
            timeout(TIMEOUT, self.stream.read_to_end(&mut received))
                .await
                .expect("the handler should close the connection")
                .expect("reading from an in-memory stream should succeed");
            let result = timeout(TIMEOUT, self.handler)
                .await
                .expect("the handler should end once it closes the connection")
                .expect("the handler shouldn't panic");
            ConnectionEnd { received, result }
        }
    }

    /// How a connection ended.
    #[derive(Debug)]
    struct ConnectionEnd {
        received: Vec<u8>,
        result: Result<(), ConnectionError>,
    }

    /// # Returns
    ///
    /// The bytes of the result frames at `result_indices` of task `task_index`.
    fn encode_results(
        task_index: TaskIndex,
        result_indices: impl IntoIterator<Item = u64>,
    ) -> Vec<u8> {
        let mut bytes = Vec::new();
        for result_index in result_indices {
            bytes.extend(encode_result(
                result_index,
                timestamp(result_index),
                &message(task_index, result_index),
            ));
        }
        bytes
    }

    /// # Returns
    ///
    /// The timestamp of the result at `result_index`.
    fn timestamp(result_index: u64) -> i64 {
        1_700_000_000_000 + i64::try_from(result_index).expect("test result indices fit in `i64`")
    }

    /// # Returns
    ///
    /// The message of the result at `result_index` of task `task_index`.
    fn message(task_index: TaskIndex, result_index: u64) -> String {
        format!("{{\"task\":{task_index},\"result\":{result_index}}}\n")
    }

    /// # Returns
    ///
    /// The results at `result_indices` of task `task_index` of archive `archive_id`, in index
    /// order.
    ///
    /// # Panics
    ///
    /// Panics if `archive_id` isn't a valid UUID.
    fn expected_results(
        task_index: TaskIndex,
        archive_id: &str,
        result_indices: std::ops::Range<u64>,
    ) -> Vec<QueryResult> {
        let archive_id = archive_id
            .parse::<ArchiveId>()
            .expect("archive ID fixtures should be valid UUIDs");
        result_indices
            .map(|result_index| QueryResult {
                archive_id,
                timestamp_millisecs: timestamp(result_index),
                message: message(task_index, result_index),
            })
            .collect()
    }

    #[tokio::test]
    async fn acknowledgement_waits_until_the_last_batch_is_handed_to_the_consumer()
    -> anyhow::Result<()> {
        let mut harness = Harness::new(1);
        // The batch of the results read with the handshake fills the channel, so the last batch
        // waits to be sent.
        let mut attempt = harness.connect_with_buffered(0, ARCHIVE_ID, &encode_results(0, 0..5));
        attempt.send_results(0, 5..10).await;
        attempt.send(&encode_trailer(10)).await;
        attempt.shut_down_write().await;

        attempt.expect_open().await;
        let first_batch = harness.next_batch().await;
        let end = attempt.wait_closed().await;
        end.result?;

        assert_eq!(first_batch, expected_results(0, ARCHIVE_ID, 0..5));
        assert_eq!(end.received, Ack { num_results: 10 }.encode());
        assert_eq!(
            harness.take_batches(),
            [expected_results(0, ARCHIVE_ID, 5..10)]
        );
        Ok(())
    }

    #[tokio::test]
    async fn retry_of_only_duplicates_is_acknowledged() -> anyhow::Result<()> {
        let mut harness = Harness::new(CHANNEL_CAPACITY);
        let mut first_attempt = harness.connect(0, ARCHIVE_ID);
        first_attempt.send_results(0, 0..3).await;
        first_attempt.finish(3).await.result?;

        let mut retry = harness.connect(0, ARCHIVE_ID);
        retry.send_results(0, 0..3).await;
        let retry_end = retry.finish(3).await;
        retry_end.result?;

        assert_eq!(retry_end.received, Ack { num_results: 3 }.encode());
        assert_eq!(
            harness.take_batches(),
            [expected_results(0, ARCHIVE_ID, 0..3)]
        );
        assert_eq!(
            harness.state.stats(),
            SessionStats {
                num_results_emitted: 3,
                num_duplicates_dropped: 3,
                ..SessionStats::default()
            }
        );
        Ok(())
    }

    #[tokio::test]
    async fn overlapping_attempts_emit_each_result_once_and_are_both_acknowledged()
    -> anyhow::Result<()> {
        const TASK_INDEX: TaskIndex = 3;

        let mut harness = Harness::new(CHANNEL_CAPACITY);
        let mut first_attempt = harness.connect(TASK_INDEX, ARCHIVE_ID);
        let mut second_attempt = harness.connect(TASK_INDEX, ARCHIVE_ID);
        let mut batches = Vec::new();
        first_attempt.send_results(TASK_INDEX, 0..2).await;
        batches.push(harness.next_batch().await);
        second_attempt.send_results(TASK_INDEX, 0..3).await;
        batches.push(harness.next_batch().await);
        first_attempt.send_results(TASK_INDEX, 2..4).await;
        batches.push(harness.next_batch().await);
        second_attempt.send_results(TASK_INDEX, 3..6).await;
        batches.push(harness.next_batch().await);
        first_attempt.send_results(TASK_INDEX, 4..6).await;
        let first_end = first_attempt.finish(6).await;
        let second_end = second_attempt.finish(6).await;
        first_end.result?;
        second_end.result?;
        batches.extend(harness.take_batches());

        assert_eq!(first_end.received, Ack { num_results: 6 }.encode());
        assert_eq!(second_end.received, Ack { num_results: 6 }.encode());
        assert_eq!(
            batches,
            [
                expected_results(TASK_INDEX, ARCHIVE_ID, 0..2),
                expected_results(TASK_INDEX, ARCHIVE_ID, 2..3),
                expected_results(TASK_INDEX, ARCHIVE_ID, 3..4),
                expected_results(TASK_INDEX, ARCHIVE_ID, 4..6),
            ]
        );
        assert_eq!(
            harness.state.stats(),
            SessionStats {
                num_results_emitted: 6,
                num_duplicates_dropped: 6,
                ..SessionStats::default()
            }
        );
        Ok(())
    }

    #[tokio::test]
    async fn two_tasks_in_one_session_each_emit_every_result_and_are_acknowledged()
    -> anyhow::Result<()> {
        const SECOND_TASK_ARCHIVE_ID: &str = "9c3e7a1d-2f5b-4e8c-b6d0-7a1f3e5c9b24";
        const NUM_RESULTS: u64 = 3;

        let mut harness = Harness::new(CHANNEL_CAPACITY);
        let mut first_task_attempt = harness.connect(0, ARCHIVE_ID);
        let mut second_task_attempt = harness.connect(1, SECOND_TASK_ARCHIVE_ID);
        let mut first_task_batches = Vec::new();
        let mut second_task_batches = Vec::new();
        for result_index in 0..NUM_RESULTS {
            first_task_attempt.send_results(0, [result_index]).await;
            first_task_batches.push(harness.next_batch().await);
            second_task_attempt.send_results(1, [result_index]).await;
            second_task_batches.push(harness.next_batch().await);
        }
        let first_task_end = first_task_attempt.finish(NUM_RESULTS).await;
        let second_task_end = second_task_attempt.finish(NUM_RESULTS).await;
        first_task_end.result?;
        second_task_end.result?;

        let ack = Ack {
            num_results: NUM_RESULTS,
        }
        .encode();
        assert_eq!(first_task_end.received, ack);
        assert_eq!(second_task_end.received, ack);
        assert_eq!(
            first_task_batches.concat(),
            expected_results(0, ARCHIVE_ID, 0..NUM_RESULTS)
        );
        assert_eq!(
            second_task_batches.concat(),
            expected_results(1, SECOND_TASK_ARCHIVE_ID, 0..NUM_RESULTS)
        );
        assert_eq!(
            harness.state.stats(),
            SessionStats {
                num_results_emitted: 2 * NUM_RESULTS,
                ..SessionStats::default()
            }
        );
        Ok(())
    }

    #[tokio::test]
    async fn out_of_order_index_closes_the_connection_without_moving_the_cursor()
    -> anyhow::Result<()> {
        for out_of_order_indices in [[0, 2], [0, 0]] {
            let mut harness = Harness::new(CHANNEL_CAPACITY);
            let mut attempt = harness.connect(0, ARCHIVE_ID);
            attempt.send_results(0, out_of_order_indices).await;
            let end = attempt.wait_closed().await;

            let mut retry = harness.connect(0, ARCHIVE_ID);
            retry.send_results(0, 0..4).await;
            retry.finish(4).await.result?;

            assert_eq!(end.received, [] as [u8; 0], "{out_of_order_indices:?}");
            assert!(
                matches!(
                    end.result,
                    Err(ConnectionError::Protocol(ProtocolError::UnexpectedResultIndex {
                        expected: 1,
                        received,
                    })) if out_of_order_indices[1] == received
                ),
                "{out_of_order_indices:?}: unexpected result: {:?}",
                end.result
            );
            assert_eq!(
                harness.take_batches().concat(),
                expected_results(0, ARCHIVE_ID, 0..4),
                "{out_of_order_indices:?}"
            );
            assert_eq!(
                harness.state.stats(),
                SessionStats {
                    num_results_emitted: 4,
                    num_duplicates_dropped: 1,
                    ..SessionStats::default()
                },
                "{out_of_order_indices:?}"
            );
        }
        Ok(())
    }

    #[tokio::test]
    async fn trailer_counting_other_results_is_a_protocol_error() {
        for trailer_num_results in [2, 4] {
            let harness = Harness::new(CHANNEL_CAPACITY);
            let mut attempt = harness.connect(0, ARCHIVE_ID);
            attempt.send_results(0, 0..3).await;
            let end = attempt.finish(trailer_num_results).await;

            assert_eq!(end.received, [] as [u8; 0], "{trailer_num_results}");
            assert!(
                matches!(
                    end.result,
                    Err(ConnectionError::Protocol(ProtocolError::ResultCountMismatch {
                        trailer_num_results: counted,
                        num_results: 3,
                    })) if trailer_num_results == counted
                ),
                "{trailer_num_results}: unexpected result: {:?}",
                end.result
            );
        }
    }

    #[tokio::test]
    async fn bytes_after_the_trailer_are_a_protocol_error() {
        for is_same_read in [true, false] {
            let mut harness = Harness::new(CHANNEL_CAPACITY);
            let mut attempt = harness.connect(0, ARCHIVE_ID);
            let mut bytes = encode_results(0, 0..2);
            bytes.extend(encode_trailer(2));
            if is_same_read {
                bytes.extend(encode_results(0, [2]));
                attempt.send(&bytes).await;
            } else {
                attempt.send(&bytes).await;
                // The batch shows that the handler has read the trailer.
                harness.next_batch().await;
                attempt.send(&[0x00]).await;
            }
            attempt.shut_down_write().await;
            let end = attempt.wait_closed().await;

            assert_eq!(end.received, [] as [u8; 0], "{is_same_read}");
            assert!(
                matches!(
                    end.result,
                    Err(ConnectionError::Protocol(ProtocolError::DataAfterTrailer))
                ),
                "{is_same_read}: unexpected result: {:?}",
                end.result
            );
        }
    }

    #[tokio::test]
    async fn eof_without_a_trailer_is_not_acknowledged() {
        let mut harness = Harness::new(CHANNEL_CAPACITY);
        let mut attempt = harness.connect(0, ARCHIVE_ID);
        attempt.send_results(0, 0..3).await;
        attempt.shut_down_write().await;
        let end = attempt.wait_closed().await;

        assert_eq!(end.received, [] as [u8; 0]);
        assert!(
            matches!(end.result, Err(ConnectionError::MissingTrailer)),
            "unexpected result: {:?}",
            end.result
        );
        assert_eq!(
            harness.take_batches().concat(),
            expected_results(0, ARCHIVE_ID, 0..3)
        );
    }

    #[tokio::test]
    async fn result_cut_off_by_a_disconnect_is_never_claimed() -> anyhow::Result<()> {
        let mut harness = Harness::new(CHANNEL_CAPACITY);
        let mut attempt = harness.connect(0, ARCHIVE_ID);
        attempt.send_results(0, 0..2).await;
        let cut_off_result = encode_result(2, timestamp(2), &message(0, 2));
        attempt
            .send(&cut_off_result[..cut_off_result.len() - 3])
            .await;
        attempt.shut_down_write().await;
        let end = attempt.wait_closed().await;

        let mut retry = harness.connect(0, ARCHIVE_ID);
        retry.send_results(0, 0..4).await;
        retry.finish(4).await.result?;

        assert_eq!(end.received, [] as [u8; 0]);
        assert!(
            matches!(end.result, Err(ConnectionError::TruncatedFrame)),
            "unexpected result: {:?}",
            end.result
        );
        assert_eq!(
            harness.take_batches().concat(),
            expected_results(0, ARCHIVE_ID, 0..4)
        );
        assert_eq!(
            harness.state.stats(),
            SessionStats {
                num_results_emitted: 4,
                num_duplicates_dropped: 2,
                ..SessionStats::default()
            }
        );
        Ok(())
    }

    #[test]
    fn archive_id_mismatch_is_rejected() -> anyhow::Result<()> {
        const OTHER_ARCHIVE_ID: &str = "4b0c2f8e-6d13-4f4e-9a57-1c2d3e4f5a6b";

        let harness = Harness::new(CHANNEL_CAPACITY);
        harness.join(0, ARCHIVE_ID)?;
        let Err(error) = harness.join(0, OTHER_ARCHIVE_ID) else {
            anyhow::bail!("an attempt of another archive shouldn't join the session");
        };

        let archive_id = ARCHIVE_ID.parse::<ArchiveId>()?;
        let other_archive_id = OTHER_ARCHIVE_ID.parse::<ArchiveId>()?;
        assert!(
            matches!(
                error,
                ProtocolError::ArchiveIdMismatch {
                    task_index: 0,
                    expected,
                    received,
                } if archive_id == expected && other_archive_id == received
            ),
            "unexpected error: {error:?}"
        );
        Ok(())
    }

    #[tokio::test]
    async fn malformed_frame_closes_the_connection_after_emitting_the_results_claimed_before_it()
    -> anyhow::Result<()> {
        let mut harness = Harness::new(CHANNEL_CAPACITY);
        let mut attempt = harness.connect(0, ARCHIVE_ID);
        let mut bytes = encode_results(0, 0..2);
        rmp::encode::write_array_len(&mut bytes, 2)?;
        rmp::encode::write_uint(&mut bytes, 2)?;
        rmp::encode::write_str(&mut bytes, "message\n")?;
        attempt.send(&bytes).await;
        let end = attempt.wait_closed().await;

        assert_eq!(end.received, [] as [u8; 0]);
        assert!(
            matches!(
                end.result,
                Err(ConnectionError::Protocol(ProtocolError::MalformedFrame(_)))
            ),
            "unexpected result: {:?}",
            end.result
        );
        assert_eq!(
            harness.take_batches(),
            [expected_results(0, ARCHIVE_ID, 0..2)]
        );
        Ok(())
    }

    #[tokio::test]
    async fn slow_consumer_stops_the_connection_from_reading() -> anyhow::Result<()> {
        const MESSAGE_LEN: usize = 4096;

        /// Far more bytes than the in-memory stream, the handler's read buffer and the channel
        /// hold.
        const NUM_RESULTS: u64 = 256;

        const STALL_CHECK_DURATION: Duration = Duration::from_millis(200);

        let mut harness = Harness::new(1);
        let mut attempt = harness.connect(0, ARCHIVE_ID);
        let expected = expected_results(0, ARCHIVE_ID, 0..NUM_RESULTS)
            .into_iter()
            .map(|result| QueryResult {
                message: format!(
                    "{}{}",
                    "x".repeat(MESSAGE_LEN - result.message.len()),
                    result.message
                ),
                ..result
            })
            .collect::<Vec<_>>();
        let mut bytes = Vec::new();
        for (result_index, result) in (0..).zip(&expected) {
            bytes.extend(encode_result(
                result_index,
                result.timestamp_millisecs,
                &result.message,
            ));
        }
        let mut attempt_task = tokio::spawn(async move {
            attempt.send(&bytes).await;
            attempt.finish(NUM_RESULTS).await
        });

        let stall_check = timeout(STALL_CHECK_DURATION, &mut attempt_task).await;
        assert!(
            stall_check.is_err(),
            "the handler shouldn't read every result while the consumer is stalled: \
             {stall_check:?}"
        );
        let mut results = Vec::new();
        while results.len() < usize::try_from(NUM_RESULTS)? {
            results.extend(harness.next_batch().await);
        }
        let end = timeout(TIMEOUT, attempt_task).await??;
        end.result?;

        assert_eq!(
            end.received,
            Ack {
                num_results: NUM_RESULTS
            }
            .encode()
        );
        assert_eq!(results.len(), expected.len());
        assert_eq!(
            results
                .iter()
                .zip(&expected)
                .position(|(result, expected_result)| result != expected_result),
            None,
            "every result should arrive once, in order, with the attempt's archive ID and its own \
             message"
        );
        Ok(())
    }

    #[tokio::test]
    async fn one_large_write_is_read_at_most_max_read_size_at_a_time() -> anyhow::Result<()> {
        use super::MAX_READ_SIZE;

        /// Far more bytes than [`MAX_READ_SIZE`], and fewer than the in-memory stream holds.
        const NUM_RESULTS: u64 = 10_000;

        let mut harness = Harness {
            stream_capacity: 1024 * 1024,
            ..Harness::new(CHANNEL_CAPACITY)
        };
        let mut attempt = harness.connect(0, ARCHIVE_ID);
        attempt.send_results(0, 0..NUM_RESULTS).await;
        let end = attempt.finish(NUM_RESULTS).await;
        end.result?;

        let batches = harness.take_batches();
        // No frame is shorter than the first, and a batch may also hold the result whose frame the
        // previous read cut off.
        let max_batch_len = MAX_READ_SIZE / encode_results(0, [0]).len() + 1;
        assert_eq!(
            batches
                .iter()
                .map(Vec::len)
                .find(|&batch_len| max_batch_len < batch_len),
            None,
            "no batch should hold more results than one read of {MAX_READ_SIZE} bytes \
             ({max_batch_len} results)"
        );
        assert_eq!(
            batches.concat(),
            expected_results(0, ARCHIVE_ID, 0..NUM_RESULTS)
        );
        Ok(())
    }

    #[tokio::test]
    async fn results_after_the_receiver_is_dropped_are_discarded_and_acknowledged()
    -> anyhow::Result<()> {
        const NUM_RESULTS: u64 = 1000;
        const NUM_RESULTS_RECEIVED: u64 = 2;

        let mut harness = Harness::new(CHANNEL_CAPACITY);
        let mut attempt = harness.connect(0, ARCHIVE_ID);
        attempt.send_results(0, 0..NUM_RESULTS_RECEIVED).await;
        harness.next_batch().await;
        harness.batches_receiver = None;

        attempt
            .send_results(0, NUM_RESULTS_RECEIVED..NUM_RESULTS)
            .await;
        let end = attempt.finish(NUM_RESULTS).await;
        end.result?;

        assert_eq!(
            end.received,
            Ack {
                num_results: NUM_RESULTS
            }
            .encode()
        );
        assert_eq!(
            harness.state.stats(),
            SessionStats {
                num_results_emitted: NUM_RESULTS_RECEIVED,
                num_duplicates_dropped: 0,
                num_results_discarded: NUM_RESULTS - NUM_RESULTS_RECEIVED,
            }
        );
        Ok(())
    }
}
