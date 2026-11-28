//! Polling for job status in the query jobs table.

use std::future::Future;
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
/// * Forwards [`poll_status`]'s return values on failure.
///
/// # Cancel safety
///
/// Dropping the future stops polling without cancelling or modifying the job.
pub async fn wait_for_status<OutputType>(
    sql_pool: &MySqlPool,
    job_id: u64,
    config: &QueryJobPollingConfig,
    job_not_found: impl FnOnce(u64) -> ClientError,
    evaluate_status: impl FnMut(QueryJobStatus) -> ControlFlow<Result<OutputType, ClientError>>,
) -> Result<OutputType, ClientError> {
    poll_status(
        config,
        || query_job::get_status(sql_pool, job_id),
        || job_not_found(job_id),
        evaluate_status,
    )
    .await
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

/// Polls for a job's status, backing off between reads until `evaluate_status` decides to stop.
///
/// # Type Parameters
///
/// * `OutputType` - The value returned when polling ends successfully.
/// * `ReadFutureType` - The future that reads the job's status.
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
/// * Forwards `read_status`'s return values on failure.
async fn poll_status<
    OutputType,
    ReadFutureType: Future<Output = Result<Option<QueryJobStatus>, clp_rust_utils::Error>> + Send,
>(
    config: &QueryJobPollingConfig,
    mut read_status: impl FnMut() -> ReadFutureType,
    job_not_found: impl FnOnce() -> ClientError,
    mut evaluate_status: impl FnMut(QueryJobStatus) -> ControlFlow<Result<OutputType, ClientError>>,
) -> Result<OutputType, ClientError> {
    let mut backoff = Backoff::new(config);
    loop {
        let Some(status) = read_status().await? else {
            return Err(job_not_found());
        };
        if let ControlFlow::Break(result) = evaluate_status(status) {
            return result;
        }
        tokio::time::sleep(backoff.next_delay()).await;
    }
}

#[cfg(test)]
mod tests {
    use std::future::ready;
    use std::ops::ControlFlow;
    use std::time::Duration;

    use clp_rust_utils::clp_config::package::config::QueryJobPollingConfig;

    use super::Backoff;
    use super::poll_status;
    use crate::error::ClientError;

    const POLL_CONFIG: QueryJobPollingConfig = QueryJobPollingConfig {
        initial_backoff_ms: 10,
        max_backoff_ms: 100,
    };
    const POLL_TIMEOUT: Duration = Duration::from_secs(1);

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

    #[tokio::test(start_paused = true)]
    async fn polling_reads_immediately_then_backs_off_until_the_policy_stops() {
        use clp_rust_utils::job_config::QueryJobStatus;
        use tokio::time::Instant;

        let statuses = [
            QueryJobStatus::Pending,
            QueryJobStatus::Running,
            QueryJobStatus::Succeeded,
        ];
        let mut unread_statuses = statuses.into_iter();
        let mut read_times = Vec::new();
        let mut observed_statuses = Vec::new();
        let start = Instant::now();

        let terminal_status = tokio::time::timeout(
            POLL_TIMEOUT,
            poll_status(
                &POLL_CONFIG,
                || {
                    read_times.push(start.elapsed());
                    ready(Ok(Some(
                        unread_statuses
                            .next()
                            .expect("no read should follow a terminal status"),
                    )))
                },
                || panic!("the scripted job should exist"),
                |status| {
                    observed_statuses.push(status);
                    if status == QueryJobStatus::Succeeded {
                        ControlFlow::Break(Ok(status))
                    } else {
                        ControlFlow::Continue(())
                    }
                },
            ),
        )
        .await
        .expect("polling should finish before the test timeout")
        .expect("the terminal status should be returned");

        let initial_delay = Duration::from_millis(POLL_CONFIG.initial_backoff_ms);
        assert_eq!(terminal_status, QueryJobStatus::Succeeded);
        assert_eq!(observed_statuses, statuses);
        assert_eq!(
            read_times,
            [Duration::ZERO, initial_delay, 3 * initial_delay]
        );
        assert_eq!(start.elapsed(), 3 * initial_delay);
    }

    #[tokio::test(start_paused = true)]
    async fn polling_returns_the_supplied_error_for_a_missing_job() {
        const JOB_ID: u64 = 42;

        let mut read_result = Some(Ok(None));
        let result: Result<(), _> = tokio::time::timeout(
            POLL_TIMEOUT,
            poll_status(
                &POLL_CONFIG,
                || {
                    ready(
                        read_result
                            .take()
                            .expect("a missing job should not be read again"),
                    )
                },
                || ClientError::ExtractionJobNotFound(JOB_ID),
                |_| panic!("the policy should not run without a status"),
            ),
        )
        .await
        .expect("polling should finish before the test timeout");

        assert!(
            matches!(result, Err(ClientError::ExtractionJobNotFound(JOB_ID))),
            "unexpected poll result: {result:?}"
        );
    }

    #[tokio::test(start_paused = true)]
    async fn polling_propagates_the_status_read_error() {
        let mut read_result = Some(Err(sqlx::Error::PoolTimedOut.into()));
        let result: Result<(), _> = tokio::time::timeout(
            POLL_TIMEOUT,
            poll_status(
                &POLL_CONFIG,
                || {
                    ready(
                        read_result
                            .take()
                            .expect("a failed read should not be retried"),
                    )
                },
                || panic!("a read error should not be mistaken for a missing job"),
                |_| panic!("the policy should not run after a read error"),
            ),
        )
        .await
        .expect("polling should finish before the test timeout");

        assert!(
            matches!(result, Err(ClientError::Sql(sqlx::Error::PoolTimedOut))),
            "unexpected poll result: {result:?}"
        );
    }
}
