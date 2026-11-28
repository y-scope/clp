//! Polling for job status in the query jobs table.

use std::ops::ControlFlow;
use std::time::Duration;

use clp_rust_utils::clp_config::package::config::QueryJobPollingConfig;
use clp_rust_utils::database::mysql::query_job;
use clp_rust_utils::job_config::QueryJobStatus;
use sqlx::MySqlPool;

use crate::error::ClientError;

/// Reads a job's status immediately, then polls with backoff until `evaluate_status` decides to
/// stop.
///
/// # Type Parameters
///
/// * `OutputType` - The value returned when polling ends successfully.
///
/// # Returns
///
/// The value supplied by `evaluate_status` on success.
///
/// # Errors
///
/// Returns an error if:
///
/// * [`ClientError`] as supplied by `job_not_found` if the job doesn't exist.
/// * `evaluate_status` breaks with an error.
/// * Forwards [`query_job::get_status`]'s return values on failure.
///
/// # Cancel safety
///
/// Dropping the future stops polling without cancelling or modifying the job.
pub async fn wait_for_status<OutputType>(
    sql_pool: &MySqlPool,
    job_id: u64,
    config: &QueryJobPollingConfig,
    job_not_found: impl FnOnce(u64) -> ClientError,
    evaluate_status: impl Fn(QueryJobStatus) -> ControlFlow<Result<OutputType, ClientError>>,
) -> Result<OutputType, ClientError> {
    let mut backoff = Backoff::new(config);
    loop {
        let Some(status) = query_job::get_status(sql_pool, job_id).await? else {
            return Err(job_not_found(job_id));
        };
        if let ControlFlow::Break(result) = evaluate_status(status) {
            return result;
        }
        tokio::time::sleep(backoff.next_delay()).await;
    }
}

/// The backoff between status reads. The initial delay is at least one millisecond, and each next
/// delay doubles the previous one up to the configured maximum or the initial delay, whichever is
/// greater.
struct Backoff {
    delay_ms: u64,
    max_delay_ms: u64,
}

impl Backoff {
    /// # Returns
    ///
    /// A newly created backoff, starting at its first delay.
    fn new(config: &QueryJobPollingConfig) -> Self {
        let delay_ms = config.initial_backoff_ms.max(1);
        Self {
            delay_ms,
            max_delay_ms: config.max_backoff_ms.max(delay_ms),
        }
    }

    /// # Returns
    ///
    /// The delay before the next status read. Advances the backoff for subsequent reads.
    fn next_delay(&mut self) -> Duration {
        let delay = Duration::from_millis(self.delay_ms);
        self.delay_ms = self.delay_ms.saturating_mul(2).min(self.max_delay_ms);
        delay
    }
}

#[cfg(test)]
mod tests {
    use std::time::Duration;

    use clp_rust_utils::clp_config::package::config::QueryJobPollingConfig;

    use super::Backoff;

    #[test]
    fn backoff_doubles_and_clamps_its_delays() {
        for (config, expected_delays_ms) in [
            (
                QueryJobPollingConfig {
                    initial_backoff_ms: 100,
                    max_backoff_ms: 500,
                },
                [100, 200, 400, 500, 500],
            ),
            (
                QueryJobPollingConfig {
                    initial_backoff_ms: 0,
                    max_backoff_ms: 4,
                },
                [1, 2, 4, 4, 4],
            ),
            (
                QueryJobPollingConfig {
                    initial_backoff_ms: 100,
                    max_backoff_ms: 50,
                },
                [100; 5],
            ),
            (
                QueryJobPollingConfig {
                    initial_backoff_ms: 0,
                    max_backoff_ms: 0,
                },
                [1; 5],
            ),
            (
                QueryJobPollingConfig {
                    initial_backoff_ms: u64::MAX / 2 + 1,
                    max_backoff_ms: u64::MAX,
                },
                [u64::MAX / 2 + 1, u64::MAX, u64::MAX, u64::MAX, u64::MAX],
            ),
        ] {
            let mut backoff = Backoff::new(&config);
            for expected_delay_ms in expected_delays_ms {
                assert_eq!(
                    backoff.next_delay(),
                    Duration::from_millis(expected_delay_ms),
                    "config: {config:?}"
                );
            }
        }
    }
}
