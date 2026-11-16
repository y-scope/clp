pub mod client;
mod error;
pub mod routes;
#[cfg_attr(
    not(test),
    expect(
        dead_code,
        reason = "the streaming query endpoint that uses the module comes in a later change"
    )
)]
mod sse;
pub mod webui_client;
