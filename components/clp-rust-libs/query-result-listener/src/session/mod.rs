//! Sessions, each of which collects the results streamed for one query job.

mod cancellation_guard;
mod config;
mod connection;
mod cursor;
mod event;
mod open_session;
mod registry;
mod state;
mod stats;

pub use config::SessionConfig;
pub(crate) use connection::serve;
pub use event::SessionEvent;
pub use event::SessionEventError;
pub use open_session::OpenSession;
pub(crate) use registry::SessionRegistry;
pub use stats::SessionStats;
