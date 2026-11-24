//! Types and constants of the API server's routes that the API server and its clients share.

mod sse;
pub mod streaming_query;

pub use sse::SSE_KEEP_ALIVE_INTERVAL;
