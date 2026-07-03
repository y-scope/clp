use tracing_appender::{
    non_blocking::{NonBlockingBuilder, WorkerGuard},
    rolling::{RollingFileAppender, Rotation},
};
use tracing_subscriber::{self, fmt::writer::MakeWriterExt};

/// Initializes the global tracing subscriber with JSON-formatted output to stdout.
///
/// If the `CLP_LOGS_DIR` environment variable is set, logs are also written to a
/// rolling file (`{log_filename}`) in that directory. The returned [`WorkerGuard`]s
/// must be held for the lifetime of the program to ensure logs are flushed.
pub fn set_up_logging(log_filename: &str) -> Vec<WorkerGuard> {
    let subscriber = tracing_subscriber::fmt()
        .event_format(
            tracing_subscriber::fmt::format()
                .with_level(true)
                .with_target(false)
                .with_file(true)
                .with_line_number(true)
                .json(),
        )
        .with_env_filter(tracing_subscriber::EnvFilter::from_default_env())
        .with_ansi(false);

    let (stdout_writer, stdout_guard) = NonBlockingBuilder::default()
        .lossy(false)
        .finish(std::io::stdout());

    let mut guards = vec![stdout_guard];

    if let Ok(logs_directory) = std::env::var("CLP_LOGS_DIR") {
        let logs_directory = std::path::Path::new(logs_directory.as_str());
        let file_appender =
            RollingFileAppender::new(Rotation::HOURLY, logs_directory, log_filename);
        let (file_writer, file_guard) = NonBlockingBuilder::default()
            .lossy(false)
            .finish(file_appender);
        
        guards.push(file_guard);

        subscriber
            .with_writer(stdout_writer.and(file_writer))
            .init();
    } else {
        subscriber.with_writer(stdout_writer).init();
    }

    guards
}
