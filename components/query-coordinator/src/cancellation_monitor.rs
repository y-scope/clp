//! The monitor that relays query job cancellations requested through the API server to Spider.

use std::time::Duration;

use clp_rust_utils::job_config::QUERY_JOBS_TABLE_NAME;
use clp_rust_utils::job_config::QueryJobId;
use clp_rust_utils::job_config::QueryJobStatus;
use clp_rust_utils::job_config::QueryJobType;
use const_format::formatcp;
use spider_core::types::id::JobId as SpiderJobId;
use sqlx::MySqlPool;
use tokio_util::sync::CancellationToken;

use crate::Error;
use crate::query_job_submitter::QueryJobSubmitter;

/// Relays the cancellation of query jobs in [`QueryJobStatus::CancelRequested`] to Spider.
///
/// # Type Parameters
///
/// * `SubmitterType` - The type of the job submitter used to cancel Spider jobs.
pub struct CancellationMonitor<SubmitterType: QueryJobSubmitter> {
    db_pool: MySqlPool,
    job_submitter: SubmitterType,
    polling_interval: Duration,
}

impl<SubmitterType: QueryJobSubmitter> CancellationMonitor<SubmitterType> {
    /// Factory function.
    ///
    /// # Returns
    ///
    /// A newly created [`CancellationMonitor`] that polls for cancellation requests every
    /// `polling_interval` while it has none to relay.
    pub const fn new(
        db_pool: MySqlPool,
        job_submitter: SubmitterType,
        polling_interval: Duration,
    ) -> Self {
        Self {
            db_pool,
            job_submitter,
            polling_interval,
        }
    }

    /// Runs the monitor until `cancellation_token` is triggered.
    ///
    /// On each iteration, this method relays the cancellation of at most one query job. It sleeps
    /// for the polling interval only when there is no cancellation to relay or the relay failed,
    /// so a backlog of cancellations is drained without delay.
    ///
    /// Jobs are processed one at a time by design, since cancellations are expected to be rare.
    /// Errors are logged and never terminate the monitor.
    pub async fn run(self, cancellation_token: CancellationToken) {
        use tokio::select;

        loop {
            let result = select! {
                () = cancellation_token.cancelled() => break,
                result = self.relay_next_cancellation() => result,
            };

            let has_relayed = result.unwrap_or_else(|e| {
                tracing::error!(error = % e, "Failed to relay a query job cancellation.");
                false
            });
            if has_relayed {
                continue;
            }

            if tokio::time::timeout(self.polling_interval, cancellation_token.cancelled())
                .await
                .is_ok()
            {
                break;
            }
        }

        tracing::info!("Cancellation monitor shutting down.");
    }

    /// Terminates every query job in [`QueryJobStatus::CancelRequested`] that has no recorded
    /// Spider job as [`QueryJobStatus::Cancelled`].
    ///
    /// This is meant to run once at startup, before the coordinator dispatches or recovers any job.
    ///
    /// A job that fails to be terminated is logged and skipped, since [`Self::run`] retries it.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`sqlx::query::QueryScalar::fetch_all`]'s return values on failure.
    pub async fn sweep_unsubmitted_jobs(&self) -> Result<(), Error> {
        const QUERY: &str = formatcp!(
            "SELECT `id` FROM `{QUERY_JOBS_TABLE_NAME}` WHERE `type` = ? AND `status` = ? AND \
             `spider_id` IS NULL ORDER BY `id` ASC"
        );

        let query_job_ids = sqlx::query_scalar::<_, QueryJobId>(QUERY)
            .bind(QueryJobType::SearchOrAggregation)
            .bind(QueryJobStatus::CancelRequested)
            .fetch_all(&self.db_pool)
            .await?;

        for query_job_id in query_job_ids {
            tracing::info!(
                query_job_id = % query_job_id,
                "Cancelling a query job that was never submitted.",
            );
            if let Err(e) = self.cancel_unsubmitted_job(query_job_id).await {
                tracing::error!(
                    error = % e,
                    query_job_id = % query_job_id,
                    "Failed to cancel a query job that was never submitted. Skipping.",
                );
            }
        }

        Ok(())
    }

    /// Relays the cancellation of the oldest query job in [`QueryJobStatus::CancelRequested`], if
    /// any.
    ///
    /// # Returns
    ///
    /// Whether a cancellation was relayed on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`Self::fetch_next_cancel_requested_job`]'s return values on failure.
    /// * Forwards [`Self::relay_cancellation`]'s return values on failure.
    async fn relay_next_cancellation(&self) -> Result<bool, Error> {
        let Some(job) = self.fetch_next_cancel_requested_job().await? else {
            return Ok(false);
        };
        self.relay_cancellation(job).await?;
        Ok(true)
    }

    /// Fetches the oldest query job in [`QueryJobStatus::CancelRequested`].
    ///
    /// This is a non-locking read used only to discover a candidate; every status transition
    /// re-reads the job's status under its row lock.
    ///
    /// # Returns
    ///
    /// The oldest job whose cancellation was requested, or `None` if there is none, on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`sqlx::query::QueryAs::fetch_optional`]'s return values on failure.
    async fn fetch_next_cancel_requested_job(
        &self,
    ) -> Result<Option<CancelRequestedJobRowProjection>, Error> {
        const QUERY: &str = formatcp!(
            "SELECT `id`, `spider_id` FROM `{QUERY_JOBS_TABLE_NAME}` WHERE `type` = ? AND \
             `status` = ? ORDER BY `id` ASC LIMIT 1"
        );

        Ok(sqlx::query_as::<_, CancelRequestedJobRowProjection>(QUERY)
            .bind(QueryJobType::SearchOrAggregation)
            .bind(QueryJobStatus::CancelRequested)
            .fetch_optional(&self.db_pool)
            .await?)
    }

    /// Relays the cancellation of `job` to Spider.
    ///
    /// * If no Spider job is recorded for `job`, there is nothing to cancel, so the job is
    ///   terminated as [`QueryJobStatus::Cancelled`] directly.
    /// * Otherwise, its Spider job is cancelled and the job is moved to
    ///   [`QueryJobStatus::Cancelling`]. The job handle waiting on the Spider job then terminates
    ///   the query job.
    ///
    /// On a failure, the job stays in [`QueryJobStatus::CancelRequested`], so the cancellation is
    /// retried.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`Self::cancel_unsubmitted_job`]'s return values on failure.
    /// * Forwards [`QueryJobSubmitter::cancel_query_job`]'s return values on failure.
    /// * Forwards [`Self::transition_from_cancel_requested`]'s return values on failure.
    async fn relay_cancellation(&self, job: CancelRequestedJobRowProjection) -> Result<(), Error> {
        let Some(spider_job_id) = job.spider_job_id else {
            return self.cancel_unsubmitted_job(job.id).await;
        };

        self.job_submitter.cancel_query_job(spider_job_id).await?;
        tracing::info!(
            query_job_id = % job.id,
            spider_job_id = % spider_job_id,
            "Relayed the query job cancellation to Spider.",
        );

        self.transition_from_cancel_requested(job.id, QueryJobStatus::Cancelling, None)
            .await
    }

    /// Terminates the query job identified by `query_job_id`, which has no recorded Spider job, as
    /// [`QueryJobStatus::Cancelled`].
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`Self::transition_from_cancel_requested`]'s return values on failure.
    async fn cancel_unsubmitted_job(&self, query_job_id: QueryJobId) -> Result<(), Error> {
        const CANCELLED_MESSAGE: &str = "The query job was cancelled before it was submitted.";

        self.transition_from_cancel_requested(
            query_job_id,
            QueryJobStatus::Cancelled,
            Some(CANCELLED_MESSAGE),
        )
        .await
    }

    /// Transitions the query job identified by `query_job_id` from
    /// [`QueryJobStatus::CancelRequested`] to `to`, in a transaction that locks the job's row.
    ///
    /// If the job is no longer in [`QueryJobStatus::CancelRequested`], for example because its job
    /// handle has already terminated it, nothing is persisted.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`Error::QueryJobMetadataCorrupted`] if the query job's row no longer exists.
    /// * Forwards [`sqlx::Pool::begin`]'s return values on failure.
    /// * Forwards [`sqlx::query::QueryScalar::fetch_optional`]'s return values on failure.
    /// * Forwards [`sqlx::query::Query::execute`]'s return values on failure.
    /// * Forwards [`sqlx::Transaction::commit`]'s return values on failure.
    async fn transition_from_cancel_requested(
        &self,
        query_job_id: QueryJobId,
        to: QueryJobStatus,
        status_msg: Option<&str>,
    ) -> Result<(), Error> {
        const SELECT_QUERY: &str =
            formatcp!("SELECT `status` FROM `{QUERY_JOBS_TABLE_NAME}` WHERE `id` = ? FOR UPDATE");
        const UPDATE_QUERY: &str = formatcp!(
            "UPDATE `{QUERY_JOBS_TABLE_NAME}` SET `status` = ?, `status_msg` = ? WHERE `id` = ?"
        );

        let mut tx = self.db_pool.begin().await?;
        let observed_status = sqlx::query_scalar::<_, QueryJobStatus>(SELECT_QUERY)
            .bind(query_job_id)
            .fetch_optional(&mut *tx)
            .await?
            .ok_or(Error::QueryJobMetadataCorrupted(query_job_id))?;
        if QueryJobStatus::CancelRequested != observed_status {
            tracing::info!(
                query_job_id = % query_job_id,
                observed_status = ? observed_status,
                to = ? to,
                "Skipping the cancellation transition: the query job is no longer in \
                 `CancelRequested`.",
            );
            return Ok(());
        }

        sqlx::query(UPDATE_QUERY)
            .bind(to)
            .bind(status_msg)
            .bind(query_job_id)
            .execute(&mut *tx)
            .await?;
        tx.commit().await?;

        Ok(())
    }
}

/// A projection of the columns read from a [`QueryJobStatus::CancelRequested`] query job row.
#[derive(Debug, sqlx::FromRow)]
struct CancelRequestedJobRowProjection {
    id: QueryJobId,
    #[sqlx(rename = "spider_id")]
    spider_job_id: Option<SpiderJobId>,
}
