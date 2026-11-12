pub mod client;
mod error;
pub mod routes;
#[cfg_attr(
    not(test),
    expect(
        dead_code,
        reason = "the streaming query endpoint that uses the encoding comes in a later change"
    )
)]
mod sse;
#[cfg_attr(
    not(test),
    expect(
        dead_code,
        reason = "the streaming query endpoint that uses the event streams comes in a later change"
    )
)]
mod streaming_query;
pub mod webui_client;
