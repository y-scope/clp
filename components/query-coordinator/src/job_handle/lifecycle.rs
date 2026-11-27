//! Row-locked start and terminal transitions for jobs in the query jobs table.

use clp_rust_utils::job_config::QUERY_JOBS_TABLE_NAME;
use clp_rust_utils::job_config::QueryJobId;
use clp_rust_utils::job_config::QueryJobStatus;
use const_format::formatcp;
use spider_core::types::id::JobId as SpiderJobId;
use sqlx::MySql;
use sqlx::MySqlPool;
use sqlx::Transaction;

use crate::Error;
use crate::spider_job_submitter::JobOutcome;

/// SQL lifecycle transitions for one job.
///
/// # Type Parameters
///
/// * `'pool_lifetime` - The lifetime of the shared database pool.
pub(super) struct JobLifecycle<'pool_lifetime> {
    db_pool: &'pool_lifetime MySqlPool,
    job_id: QueryJobId,
}

impl<'pool_lifetime> JobLifecycle<'pool_lifetime> {
    /// Creates the lifecycle for the given job in the CLP database.
    ///
    /// # Returns
    ///
    /// A newly created [`JobLifecycle`].
    pub(super) const fn new(db_pool: &'pool_lifetime MySqlPool, job_id: QueryJobId) -> Self {
        Self { db_pool, job_id }
    }

    /// Records the Spider job ID, task count, and start time, and moves the job from
    /// [`QueryJobStatus::Pending`] to [`QueryJobStatus::Running`] under a row lock.
    ///
    /// This method also ensures that the job has a valid `dispatch_time`, which the coordinator
    /// uses to mark jobs as dispatched. A coordinator restart may occur before the marker is
    /// persisted, leaving the Spider job running without a valid `dispatch_time`. Therefore, this
    /// method sets the field as part of the row update if it has not already been set by the
    /// coordinator.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`Error::QueryJobCancelled`] if the job is in [`QueryJobStatus::CancelRequested`].
    /// * [`Error::InvalidQueryJobStatusTransition`] if the job's status is neither
    ///   [`QueryJobStatus::Pending`] nor [`QueryJobStatus::CancelRequested`].
    /// * Forwards [`sqlx::Pool::begin`]'s return values on failure.
    /// * Forwards [`Self::lock_and_get_status`]'s return values on failure.
    /// * Forwards [`sqlx::query::Query::execute`]'s return values on failure.
    /// * Forwards [`sqlx::Transaction::commit`]'s return values on failure.
    pub(super) async fn start(
        &self,
        spider_job_id: SpiderJobId,
        num_tasks: i32,
    ) -> Result<(), Error> {
        const UPDATE_QUERY: &str = formatcp!(
            "UPDATE `{QUERY_JOBS_TABLE_NAME}` SET `spider_id` = ?, `status` = ?, `num_tasks` = ?, \
             `start_time` = CURRENT_TIMESTAMP(3), `dispatch_time` = COALESCE(`dispatch_time`, \
             CURRENT_TIMESTAMP()) WHERE `id` = ?"
        );

        let mut tx = self.db_pool.begin().await?;
        match self.lock_and_get_status(&mut tx).await? {
            QueryJobStatus::Pending => {}
            QueryJobStatus::CancelRequested => return Err(Error::QueryJobCancelled),
            from => {
                return Err(Error::InvalidQueryJobStatusTransition {
                    from,
                    to: QueryJobStatus::Running,
                });
            }
        }

        sqlx::query(UPDATE_QUERY)
            .bind(spider_job_id.get())
            .bind(QueryJobStatus::Running)
            .bind(num_tasks)
            .bind(self.job_id)
            .execute(&mut *tx)
            .await?;
        tx.commit().await?;

        Ok(())
    }

    /// Persists the job's terminal status and message in the CLP database.
    ///
    /// The current status is read under a row lock so a concurrent cancellation cannot change it
    /// before the update is committed.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`sqlx::Pool::begin`]'s return values on failure.
    /// * Forwards [`Self::lock_and_get_status`]'s return values on failure.
    /// * Forwards [`resolve_terminal_status`]'s return values on failure.
    /// * Forwards [`sqlx::query::Query::execute`]'s return values on failure.
    /// * Forwards [`sqlx::Transaction::commit`]'s return values on failure.
    pub(super) async fn terminate(&self, outcome: JobOutcome) -> Result<(), Error> {
        const UPDATE_QUERY: &str = formatcp!(
            "UPDATE `{QUERY_JOBS_TABLE_NAME}` SET `status` = ?, `status_msg` = ? WHERE `id` = ?"
        );

        let mut tx = self.db_pool.begin().await?;
        let current_status = self.lock_and_get_status(&mut tx).await?;
        let outcome_status = QueryJobStatus::from(&outcome);
        let (terminal_status, status_message) = resolve_terminal_status(outcome, current_status)?;

        match (outcome_status, terminal_status) {
            (QueryJobStatus::Succeeded, QueryJobStatus::Cancelled) => {
                tracing::info!(
                    query_job_id = % self.job_id,
                    "{CANCELLED_AFTER_COMPLETION_MESSAGE}",
                );
            }
            (QueryJobStatus::Cancelled, QueryJobStatus::Failed) => {
                tracing::error!(
                    query_job_id = % self.job_id,
                    "{CANCELLED_UNEXPECTED_MESSAGE}",
                );
            }
            _ => {}
        }

        sqlx::query(UPDATE_QUERY)
            .bind(terminal_status)
            .bind(status_message)
            .bind(self.job_id)
            .execute(&mut *tx)
            .await?;
        tx.commit().await?;

        Ok(())
    }

    /// Reads the job's current status, locking its row until `tx` ends.
    ///
    /// # Returns
    ///
    /// The job's current status on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`Error::QueryJobMetadataCorrupted`] if the job's row no longer exists.
    /// * Forwards [`sqlx::query::QueryScalar::fetch_optional`]'s return values on failure.
    async fn lock_and_get_status(
        &self,
        tx: &mut Transaction<'_, MySql>,
    ) -> Result<QueryJobStatus, Error> {
        const SELECT_QUERY: &str =
            formatcp!("SELECT `status` FROM `{QUERY_JOBS_TABLE_NAME}` WHERE `id` = ? FOR UPDATE");

        sqlx::query_scalar::<_, QueryJobStatus>(SELECT_QUERY)
            .bind(self.job_id)
            .fetch_optional(&mut **tx)
            .await?
            .ok_or(Error::QueryJobMetadataCorrupted(self.job_id))
    }
}

const CANCELLED_AFTER_COMPLETION_MESSAGE: &str =
    "The query job completed, but it had already been requested to be cancelled.";
const CANCELLED_UNEXPECTED_MESSAGE: &str =
    "The query job was cancelled unexpectedly without a cancellation request.";

impl From<&JobOutcome> for QueryJobStatus {
    fn from(outcome: &JobOutcome) -> Self {
        match outcome {
            JobOutcome::Succeeded => Self::Succeeded,
            JobOutcome::Failed { .. } => Self::Failed,
            JobOutcome::Cancelled => Self::Cancelled,
        }
    }
}

/// Resolves the terminal state for a job with the given outcome and current status.
///
/// A failure takes precedence over a cancellation, and a cancellation request takes precedence
/// over a success. A reported cancellation for a job still marked [`QueryJobStatus::Running`]
/// resolves to [`QueryJobStatus::Failed`].
///
/// # Returns
///
/// On success, a tuple containing:
///
/// * The terminal [`QueryJobStatus`].
/// * The status message, if any.
///
/// # Errors
///
/// Returns an error if:
///
/// * [`Error::InvalidQueryJobStatusTransition`] if the current status doesn't permit the outcome.
fn resolve_terminal_status(
    outcome: JobOutcome,
    current_status: QueryJobStatus,
) -> Result<(QueryJobStatus, Option<String>), Error> {
    const CANCELLED_MESSAGE: &str = "The query job was cancelled.";

    let to = QueryJobStatus::from(&outcome);
    match (outcome, current_status) {
        (JobOutcome::Succeeded, QueryJobStatus::Pending | QueryJobStatus::Running) => {
            Ok((to, None))
        }
        (JobOutcome::Cancelled, QueryJobStatus::CancelRequested | QueryJobStatus::Cancelling) => {
            Ok((to, Some(CANCELLED_MESSAGE.to_owned())))
        }
        (JobOutcome::Succeeded, QueryJobStatus::CancelRequested | QueryJobStatus::Cancelling) => {
            Ok((
                QueryJobStatus::Cancelled,
                Some(CANCELLED_AFTER_COMPLETION_MESSAGE.to_owned()),
            ))
        }
        (JobOutcome::Cancelled, QueryJobStatus::Running) => Ok((
            QueryJobStatus::Failed,
            Some(CANCELLED_UNEXPECTED_MESSAGE.to_owned()),
        )),
        (JobOutcome::Failed { error_message }, from) if !from.is_terminal() => {
            Ok((to, Some(error_message)))
        }
        (_, from) => Err(Error::InvalidQueryJobStatusTransition { from, to }),
    }
}

#[cfg(test)]
mod tests {
    use clp_rust_utils::job_config::QueryJobStatus;

    use super::resolve_terminal_status;
    use crate::Error;
    use crate::spider_job_submitter::JobOutcome;

    #[test]
    fn resolve_success_from_every_status() {
        const COMPLETION_MESSAGE: &str =
            "The query job completed, but it had already been requested to be cancelled.";
        let cancelled = Ok((QueryJobStatus::Cancelled, Some(COMPLETION_MESSAGE)));
        let succeeded = Ok((QueryJobStatus::Succeeded, None));
        let rejected = Err(QueryJobStatus::Succeeded);

        for (status, expected) in [
            (QueryJobStatus::Pending, succeeded),
            (QueryJobStatus::Running, succeeded),
            (QueryJobStatus::Succeeded, rejected),
            (QueryJobStatus::Failed, rejected),
            (QueryJobStatus::CancelRequested, cancelled),
            (QueryJobStatus::Cancelling, cancelled),
            (QueryJobStatus::Cancelled, rejected),
            (QueryJobStatus::Killed, rejected),
        ] {
            assert_terminal_status(JobOutcome::Succeeded, status, expected);
        }
    }

    #[test]
    fn resolve_failure_from_every_status() {
        const ERROR_MESSAGE: &str = "the test job failed";
        let failed = Ok((QueryJobStatus::Failed, Some(ERROR_MESSAGE)));
        let rejected = Err(QueryJobStatus::Failed);

        for (status, expected) in [
            (QueryJobStatus::Pending, failed),
            (QueryJobStatus::Running, failed),
            (QueryJobStatus::Succeeded, rejected),
            (QueryJobStatus::Failed, rejected),
            (QueryJobStatus::CancelRequested, failed),
            (QueryJobStatus::Cancelling, failed),
            (QueryJobStatus::Cancelled, rejected),
            (QueryJobStatus::Killed, rejected),
        ] {
            assert_terminal_status(
                JobOutcome::Failed {
                    error_message: ERROR_MESSAGE.to_owned(),
                },
                status,
                expected,
            );
        }
    }

    #[test]
    fn resolve_cancellation_from_every_status() {
        const CANCELLATION_MESSAGE: &str = "The query job was cancelled.";
        const UNEXPECTED_CANCELLATION_MESSAGE: &str =
            "The query job was cancelled unexpectedly without a cancellation request.";
        let cancelled = Ok((QueryJobStatus::Cancelled, Some(CANCELLATION_MESSAGE)));
        let failed = Ok((
            QueryJobStatus::Failed,
            Some(UNEXPECTED_CANCELLATION_MESSAGE),
        ));
        let rejected = Err(QueryJobStatus::Cancelled);

        for (status, expected) in [
            (QueryJobStatus::Pending, rejected),
            (QueryJobStatus::Running, failed),
            (QueryJobStatus::Succeeded, rejected),
            (QueryJobStatus::Failed, rejected),
            (QueryJobStatus::CancelRequested, cancelled),
            (QueryJobStatus::Cancelling, cancelled),
            (QueryJobStatus::Cancelled, rejected),
            (QueryJobStatus::Killed, rejected),
        ] {
            assert_terminal_status(JobOutcome::Cancelled, status, expected);
        }
    }

    /// Asserts the terminal status and message or the rejected transition's source and target.
    ///
    /// # Panics
    ///
    /// Panics if the resolved result doesn't match the expected transition or rejection.
    fn assert_terminal_status(
        outcome: JobOutcome,
        current_status: QueryJobStatus,
        expected: Result<(QueryJobStatus, Option<&str>), QueryJobStatus>,
    ) {
        match (resolve_terminal_status(outcome, current_status), expected) {
            (Ok((status, message)), Ok(expected)) => {
                assert_eq!(
                    (status, message.as_deref()),
                    expected,
                    "from {current_status:?}"
                );
            }
            (Err(Error::InvalidQueryJobStatusTransition { from, to }), Err(expected_to)) => {
                assert_eq!((from, to), (current_status, expected_to));
            }
            (actual, expected) => {
                panic!(
                    "unexpected transition from {current_status:?}: {actual:?}, expected \
                     {expected:?}"
                );
            }
        }
    }
}
