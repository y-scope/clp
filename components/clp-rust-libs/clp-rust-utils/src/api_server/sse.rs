//! Constants of the API server's Server-Sent Events responses.

use std::time::Duration;

/// The interval of the keep-alive comments of the API server's Server-Sent Events responses.
///
/// A response that sends nothing for this long sends a keep-alive comment, so that proxies don't
/// close its idle connection. The same as the interval of
/// `axum::response::sse::KeepAlive::default`.
pub const SSE_KEEP_ALIVE_INTERVAL: Duration = Duration::from_secs(15);
