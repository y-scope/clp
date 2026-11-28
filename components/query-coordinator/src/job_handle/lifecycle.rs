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
pub(super) struct JobLifecycle {
    db_pool: MySqlPool,
    job_id: QueryJobId,
}

impl JobLifecycle {
    /// Creates the lifecycle for the given job in the CLP database.
    ///
    /// # Returns
    ///
    /// A newly created [`JobLifecycle`].
    pub(super) const fn new(db_pool: MySqlPool, job_id: QueryJobId) -> Self {
        Self { db_pool, job_id }
    }

    /// # Returns
    ///
    /// The ID of the query job.
    pub(super) const fn job_id(&self) -> QueryJobId {
        self.job_id
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
    /// * Forwards [`resolve_terminal_transition`]'s return values on failure.
    /// * Forwards [`sqlx::query::Query::execute`]'s return values on failure.
    /// * Forwards [`sqlx::Transaction::commit`]'s return values on failure.
    pub(super) async fn terminate(&self, outcome: JobOutcome) -> Result<(), Error> {
        const UPDATE_QUERY: &str = formatcp!(
            "UPDATE `{QUERY_JOBS_TABLE_NAME}` SET `status` = ?, `status_msg` = ? WHERE `id` = ?"
        );

        let mut tx = self.db_pool.begin().await?;
        let current_status = self.lock_and_get_status(&mut tx).await?;
        let transition = resolve_terminal_transition(outcome, current_status)?;

        match &transition {
            TerminalTransition::CancelledAfterCompletion => {
                tracing::info!(
                    query_job_id = % self.job_id,
                    "{CANCELLED_AFTER_COMPLETION_MESSAGE}",
                );
            }
            TerminalTransition::UnexpectedCancellation => {
                tracing::error!(
                    query_job_id = % self.job_id,
                    "{CANCELLED_UNEXPECTED_MESSAGE}",
                );
            }
            TerminalTransition::Succeeded
            | TerminalTransition::Cancelled
            | TerminalTransition::Failed { .. } => {}
        }

        sqlx::query(UPDATE_QUERY)
            .bind(transition.status())
            .bind(transition.into_message())
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

/// A valid transition to a terminal job status.
#[derive(Debug, Eq, PartialEq)]
enum TerminalTransition {
    Succeeded,
    Cancelled,
    CancelledAfterCompletion,
    UnexpectedCancellation,
    Failed { error_message: String },
}

impl TerminalTransition {
    /// # Returns
    ///
    /// The terminal status to persist.
    const fn status(&self) -> QueryJobStatus {
        match self {
            Self::Succeeded => QueryJobStatus::Succeeded,
            Self::Cancelled | Self::CancelledAfterCompletion => QueryJobStatus::Cancelled,
            Self::UnexpectedCancellation | Self::Failed { .. } => QueryJobStatus::Failed,
        }
    }

    /// # Returns
    ///
    /// The status message to persist, if any.
    fn into_message(self) -> Option<String> {
        const CANCELLED_MESSAGE: &str = "The query job was cancelled.";

        match self {
            Self::Succeeded => None,
            Self::Cancelled => Some(CANCELLED_MESSAGE.to_owned()),
            Self::CancelledAfterCompletion => Some(CANCELLED_AFTER_COMPLETION_MESSAGE.to_owned()),
            Self::UnexpectedCancellation => Some(CANCELLED_UNEXPECTED_MESSAGE.to_owned()),
            Self::Failed { error_message } => Some(error_message),
        }
    }
}

impl From<&JobOutcome> for QueryJobStatus {
    fn from(outcome: &JobOutcome) -> Self {
        match outcome {
            JobOutcome::Succeeded => Self::Succeeded,
            JobOutcome::Failed { .. } => Self::Failed,
            JobOutcome::Cancelled => Self::Cancelled,
        }
    }
}

/// Resolves the terminal transition for a job with the given outcome and current status.
///
/// For non-terminal jobs, failure takes precedence over cancellation, and a cancellation request
/// takes precedence over success. A reported cancellation for a job still marked
/// [`QueryJobStatus::Running`] resolves to [`QueryJobStatus::Failed`].
///
/// # Returns
///
/// The selected [`TerminalTransition`] on success.
///
/// # Errors
///
/// Returns an error if:
///
/// * [`Error::InvalidQueryJobStatusTransition`] if the current status doesn't permit the outcome.
fn resolve_terminal_transition(
    outcome: JobOutcome,
    current_status: QueryJobStatus,
) -> Result<TerminalTransition, Error> {
    match (outcome, current_status) {
        (JobOutcome::Succeeded, QueryJobStatus::Pending | QueryJobStatus::Running) => {
            Ok(TerminalTransition::Succeeded)
        }
        (JobOutcome::Cancelled, QueryJobStatus::CancelRequested | QueryJobStatus::Cancelling) => {
            Ok(TerminalTransition::Cancelled)
        }
        (JobOutcome::Succeeded, QueryJobStatus::CancelRequested | QueryJobStatus::Cancelling) => {
            Ok(TerminalTransition::CancelledAfterCompletion)
        }
        (JobOutcome::Cancelled, QueryJobStatus::Running) => {
            Ok(TerminalTransition::UnexpectedCancellation)
        }
        (JobOutcome::Failed { error_message }, from) if !from.is_terminal() => {
            Ok(TerminalTransition::Failed { error_message })
        }
        (outcome, from) => Err(Error::InvalidQueryJobStatusTransition {
            from,
            to: QueryJobStatus::from(&outcome),
        }),
    }
}

#[cfg(test)]
mod tests {
    use clp_rust_utils::job_config::QueryJobStatus;

    use super::TerminalTransition;
    use crate::spider_job_submitter::JobOutcome;

    #[test]
    fn resolve_success_from_every_status() {
        let cancelled_after_completion = Ok(&TerminalTransition::CancelledAfterCompletion);
        let succeeded = Ok(&TerminalTransition::Succeeded);
        let rejected = Err(QueryJobStatus::Succeeded);

        for (status, expected) in [
            (QueryJobStatus::Pending, succeeded),
            (QueryJobStatus::Running, succeeded),
            (QueryJobStatus::Succeeded, rejected),
            (QueryJobStatus::Failed, rejected),
            (QueryJobStatus::CancelRequested, cancelled_after_completion),
            (QueryJobStatus::Cancelling, cancelled_after_completion),
            (QueryJobStatus::Cancelled, rejected),
            (QueryJobStatus::Killed, rejected),
        ] {
            assert_terminal_transition(JobOutcome::Succeeded, status, expected);
        }
    }

    #[test]
    fn resolve_failure_from_every_status() {
        const ERROR_MESSAGE: &str = "the test job failed";
        let failed = Ok(&TerminalTransition::Failed {
            error_message: ERROR_MESSAGE.to_owned(),
        });
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
            assert_terminal_transition(
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
        let cancelled = Ok(&TerminalTransition::Cancelled);
        let unexpected_cancellation = Ok(&TerminalTransition::UnexpectedCancellation);
        let rejected = Err(QueryJobStatus::Cancelled);

        for (status, expected) in [
            (QueryJobStatus::Pending, rejected),
            (QueryJobStatus::Running, unexpected_cancellation),
            (QueryJobStatus::Succeeded, rejected),
            (QueryJobStatus::Failed, rejected),
            (QueryJobStatus::CancelRequested, cancelled),
            (QueryJobStatus::Cancelling, cancelled),
            (QueryJobStatus::Cancelled, rejected),
            (QueryJobStatus::Killed, rejected),
        ] {
            assert_terminal_transition(JobOutcome::Cancelled, status, expected);
        }
    }

    #[test]
    fn terminal_transitions_map_to_status_and_message() {
        const ERROR_MESSAGE: &str = "the test job failed";

        for (transition, status, message) in [
            (
                TerminalTransition::Succeeded,
                QueryJobStatus::Succeeded,
                None,
            ),
            (
                TerminalTransition::Cancelled,
                QueryJobStatus::Cancelled,
                Some("The query job was cancelled."),
            ),
            (
                TerminalTransition::CancelledAfterCompletion,
                QueryJobStatus::Cancelled,
                Some("The query job completed, but it had already been requested to be cancelled."),
            ),
            (
                TerminalTransition::UnexpectedCancellation,
                QueryJobStatus::Failed,
                Some("The query job was cancelled unexpectedly without a cancellation request."),
            ),
            (
                TerminalTransition::Failed {
                    error_message: ERROR_MESSAGE.to_owned(),
                },
                QueryJobStatus::Failed,
                Some(ERROR_MESSAGE),
            ),
        ] {
            assert_eq!(transition.status(), status, "{transition:?}");
            assert_eq!(transition.into_message().as_deref(), message);
        }
    }

    /// Asserts the terminal transition or the rejected transition's source and target.
    ///
    /// # Panics
    ///
    /// Panics if the resolved result doesn't match the expected transition or rejection.
    fn assert_terminal_transition(
        outcome: JobOutcome,
        current_status: QueryJobStatus,
        expected: Result<&TerminalTransition, QueryJobStatus>,
    ) {
        use super::resolve_terminal_transition;
        use crate::Error;

        match (
            resolve_terminal_transition(outcome, current_status),
            expected,
        ) {
            (Ok(transition), Ok(expected)) => {
                assert_eq!(&transition, expected, "from {current_status:?}");
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
