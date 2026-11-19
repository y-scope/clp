//! A reader of the Server-Sent Events that the API server's streaming query route sends.
//!
//! The reader isn't a general parser of the format. It relies on how the API server lays out its
//! events: each event has at most one `event:` line and one `data:` line, keep-alives are comment
//! lines, and there are no `id:` or `retry:` fields.

use bytes::Buf;
use bytes::Bytes;
use bytes::BytesMut;

/// An event read by a [`Reader`].
#[cfg_attr(test, derive(Debug, Eq, PartialEq))]
pub struct Event {
    /// The event's name from its `event:` line, or `None` for an unnamed event.
    pub name: Option<Bytes>,

    /// The event's data from its `data:` line.
    pub data: Bytes,
}

/// An error returned when the response body holds a line that the API server doesn't send.
#[derive(Debug, thiserror::Error)]
#[error("unexpected line {line:?}")]
pub struct UnexpectedLineError {
    /// The line without its line ending.
    pub line: Bytes,
}

/// An incremental reader of the events in the response body of the API server's streaming query
/// route, which is pushed the body chunk by chunk.
#[derive(Default)]
pub struct Reader {
    /// The pushed bytes that are yet to be read.
    buffer: BytesMut,

    /// The number of bytes at the start of `buffer` that are known to hold no line feed.
    num_scanned_bytes: usize,

    /// Whether the rest of an oversized line, up to its line feed, is to be dropped.
    is_dropping_line: bool,

    /// The name from an `event:` line that is waiting for its `data:` line.
    pending_event_name: Option<Bytes>,
}

impl Reader {
    /// Appends a chunk of the response body to the bytes that are yet to be read.
    pub fn push(&mut self, chunk: &[u8]) {
        self.buffer.extend_from_slice(chunk);
    }

    /// Reads the next event from the pushed bytes.
    ///
    /// The pushed bytes are split into lines, which end with `\n` or `\r\n`, and a partial line is
    /// kept until the rest of it is pushed. Then:
    ///
    /// * An `event:` line names the event of the next `data:` line.
    /// * A `data:` line ends an event.
    /// * Comment lines, which start with `:`, and blank lines are skipped.
    ///
    /// A line longer than [`MAX_LINE_SIZE`] is dropped with a warning, along with the name of the
    /// event that it may belong to.
    ///
    /// # Returns
    ///
    /// On success:
    ///
    /// * The next event.
    /// * `None` if the pushed bytes don't hold the end of another event yet.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`UnexpectedLineError`] if a line isn't one of the above, or is an `event:` line that
    ///   follows another one.
    pub fn next_event(&mut self) -> Result<Option<Event>, UnexpectedLineError> {
        while let Some(line) = self.next_line() {
            if line.is_empty() || line.starts_with(b":") {
                continue;
            }
            if let Some(name) = field_value(&line, b"event") {
                if self.pending_event_name.is_some() {
                    return Err(UnexpectedLineError { line });
                }
                self.pending_event_name = Some(name);
                continue;
            }
            let Some(data) = field_value(&line, b"data") else {
                return Err(UnexpectedLineError { line });
            };
            return Ok(Some(Event {
                name: self.pending_event_name.take(),
                data,
            }));
        }
        Ok(None)
    }

    /// Splits the next complete line, of at most [`MAX_LINE_SIZE`] bytes, off the pushed bytes.
    ///
    /// # Returns
    ///
    /// The line without its line ending, or `None` if the pushed bytes don't hold the end of
    /// another line yet.
    fn next_line(&mut self) -> Option<Bytes> {
        loop {
            let Some(newline_offset) = self.buffer[self.num_scanned_bytes..]
                .iter()
                .position(|byte| *byte == b'\n')
            else {
                if self.buffer.len() > MAX_LINE_SIZE {
                    if !self.is_dropping_line {
                        self.drop_pending_event_of_oversized_line();
                        self.is_dropping_line = true;
                    }
                    self.buffer.clear();
                    self.num_scanned_bytes = 0;
                } else {
                    self.num_scanned_bytes = self.buffer.len();
                }
                return None;
            };
            let line = self
                .buffer
                .split_to(self.num_scanned_bytes + newline_offset)
                .freeze();
            self.buffer.advance(1);
            self.num_scanned_bytes = 0;
            if std::mem::take(&mut self.is_dropping_line) {
                continue;
            }
            if line.len() > MAX_LINE_SIZE {
                self.drop_pending_event_of_oversized_line();
                continue;
            }
            return Some(match line.strip_suffix(b"\r") {
                Some(line_without_carriage_return) => line.slice_ref(line_without_carriage_return),
                None => line,
            });
        }
    }

    /// Drops the name of the event that an oversized line, which is being dropped, may belong to,
    /// and logs a warning, since the line may hold a result.
    fn drop_pending_event_of_oversized_line(&mut self) {
        self.pending_event_name = None;
        tracing::warn!(
            max_line_size = MAX_LINE_SIZE,
            "Dropped a line of the API server's response that exceeds the maximum size, which may \
             hold a result."
        );
    }
}

/// The longest line that a [`Reader`] buffers, so that the API server can't make it buffer without
/// bound.
const MAX_LINE_SIZE: usize = 64 * 1024 * 1024;

/// # Returns
///
/// The value of the field named `field_name` in `line`, without the space that may follow its
/// colon, or `None` if `line` isn't such a field.
fn field_value(line: &Bytes, field_name: &[u8]) -> Option<Bytes> {
    let value = line.strip_prefix(field_name)?.strip_prefix(b":")?;
    Some(line.slice_ref(value.strip_prefix(b" ").unwrap_or(value)))
}

#[cfg(test)]
mod tests {
    use anyhow::Result;

    use super::*;

    const STREAM: &[u8] = b"event: job\ndata: {\"query_job_id\":1}\n\n:\n\ndata: \
        {\"message\":\"first\"}\n\n: a comment\ndata:{\"message\":\"second\"}\n\n:\n\nevent: \
        end\ndata: {\"status\":\"Succeeded\",\"num_results_emitted\":2}\n\n";

    /// # Returns
    ///
    /// The events in [`STREAM`].
    fn stream_events() -> Vec<Event> {
        vec![
            Event {
                name: Some(Bytes::from_static(b"job")),
                data: Bytes::from_static(b"{\"query_job_id\":1}"),
            },
            Event {
                name: None,
                data: Bytes::from_static(b"{\"message\":\"first\"}"),
            },
            Event {
                name: None,
                data: Bytes::from_static(b"{\"message\":\"second\"}"),
            },
            Event {
                name: Some(Bytes::from_static(b"end")),
                data: Bytes::from_static(b"{\"status\":\"Succeeded\",\"num_results_emitted\":2}"),
            },
        ]
    }

    /// Pushes `stream` to a new reader in chunks of `chunk_size` bytes, reading every event after
    /// each chunk.
    ///
    /// # Returns
    ///
    /// The events read on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`Reader::next_event`]'s return values on failure.
    ///
    /// # Panics
    ///
    /// Panics if an event is read after a chunk without a line feed, or if more events are read
    /// than `stream` has bytes.
    fn read_in_chunks(stream: &[u8], chunk_size: usize) -> Result<Vec<Event>, UnexpectedLineError> {
        let mut reader = Reader::default();
        let mut events = Vec::new();
        for chunk in stream.chunks(chunk_size) {
            reader.push(chunk);
            while let Some(event) = reader.next_event()? {
                assert!(
                    chunk.contains(&b'\n'),
                    "an event should only end at the end of a line"
                );
                events.push(event);
                assert!(events.len() <= stream.len(), "reading should consume bytes");
            }
        }
        Ok(events)
    }

    #[test]
    fn reader_reads_events_split_across_chunks() -> Result<()> {
        for chunk_size in [1, 2, 3, 7, 64, STREAM.len()] {
            assert_eq!(
                read_in_chunks(STREAM, chunk_size)?,
                stream_events(),
                "chunk size: {chunk_size}"
            );
        }
        Ok(())
    }

    #[test]
    fn reader_accepts_crlf_line_endings() -> Result<()> {
        let stream: Vec<u8> = STREAM
            .iter()
            .flat_map(|byte| {
                if *byte == b'\n' {
                    b"\r\n".as_slice()
                } else {
                    std::slice::from_ref(byte)
                }
            })
            .copied()
            .collect();

        for chunk_size in [1, stream.len()] {
            assert_eq!(
                read_in_chunks(&stream, chunk_size)?,
                stream_events(),
                "chunk size: {chunk_size}"
            );
        }
        Ok(())
    }

    #[test]
    fn reader_rejects_lines_that_the_api_server_doesnt_send() {
        let cases: [(&[u8], &[u8]); 5] = [
            (b"id: 1\n", b"id: 1"),
            (b"retry: 1000\n", b"retry: 1000"),
            (b"data\n", b"data"),
            (b"Data: 1\n", b"Data: 1"),
            (b"event: job\nevent: end\ndata: {}\n", b"event: end"),
        ];
        for (stream, unexpected_line) in cases {
            let error = read_in_chunks(stream, stream.len())
                .expect_err("an unexpected line should be rejected");
            assert_eq!(error.line, unexpected_line, "stream: {stream:?}");
        }
    }

    #[test]
    fn reader_drops_oversized_lines_and_the_events_they_belong_to() -> Result<()> {
        let oversized_data_line = [b"data: ".as_slice(), &vec![b'x'; MAX_LINE_SIZE]].concat();
        let mut reader = Reader::default();

        reader.push(b"event: end\n");
        reader.push(&oversized_data_line);
        reader.push(b"\nevent: end\n");
        reader.push(&oversized_data_line[..=MAX_LINE_SIZE]);
        assert_eq!(reader.next_event()?, None);
        assert_eq!(reader.buffer, BytesMut::new());
        reader.push(&oversized_data_line[MAX_LINE_SIZE + 1..]);
        reader.push(b"\ndata: 1\n");

        assert_eq!(
            reader.next_event()?,
            Some(Event {
                name: None,
                data: Bytes::from_static(b"1"),
            })
        );
        assert_eq!(reader.next_event()?, None);
        Ok(())
    }
}
