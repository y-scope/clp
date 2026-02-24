use tracing_appender::{
    non_blocking::WorkerGuard,
    rolling::{RollingFileAppender, Rotation},
};
use tracing_subscriber::{self, fmt::writer::MakeWriterExt};

/// Initializes the global tracing subscriber with JSON-formatted output to stdout.
///
/// If the `CLP_LOGS_DIR` environment variable is set, logs are also written to a
/// rolling file (`{log_filename}`) in that directory. The returned [`WorkerGuard`]
/// must be held for the lifetime of the program to ensure file logs are flushed.
pub fn set_up_logging(log_filename: &str) -> Option<WorkerGuard> {
    if let Ok(logs_directory) = std::env::var("CLP_LOGS_DIR") {
        let logs_directory = std::path::Path::new(logs_directory.as_str());
        let file_appender =
            RollingFileAppender::new(Rotation::HOURLY, logs_directory, log_filename);
        let (non_blocking_writer, guard) = tracing_appender::non_blocking(file_appender);
        tracing_subscriber::fmt()
            .event_format(
                tracing_subscriber::fmt::format()
                    .with_level(true)
                    .with_target(false)
                    .with_file(true)
                    .with_line_number(true)
                    .json(),
            )
            .with_env_filter(tracing_subscriber::EnvFilter::from_default_env())
            .with_ansi(false)
            .with_writer(std::io::stdout.and(non_blocking_writer))
            .init();
        return Some(guard);
    }

    tracing_subscriber::fmt()
        .event_format(
            tracing_subscriber::fmt::format()
                .with_level(true)
                .with_target(false)
                .with_file(true)
                .with_line_number(true)
                .json(),
        )
        .with_env_filter(tracing_subscriber::EnvFilter::from_default_env())
        .with_ansi(false)
        .with_writer(std::io::stdout)
        .init();
    None
}
