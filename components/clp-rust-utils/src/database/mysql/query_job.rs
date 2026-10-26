//! Operations on the CLP DB's query jobs table.

use const_format::formatcp;

use crate::job_config::Cancellability;
use crate::job_config::QUERY_JOBS_TABLE_NAME;
use crate::job_config::QueryJobId;
use crate::job_config::QueryJobStatus;
use crate::job_config::QueryJobType;
use crate::job_config::SearchJobConfig;

/// Submits a query job by inserting it into the CLP DB's query jobs table.
///
/// # Returns
///
/// The ID of the submitted query job on success.
///
/// # Errors
///
/// Returns an error if:
///
/// * Forwards [`rmp_serde::to_vec_named`]'s return values on failure.
/// * Forwards [`sqlx::query::Query::execute`]'s return values on failure.
///
/// # Panics
///
/// Panics if the inserted row's ID doesn't fit in [`QueryJobId`].
pub async fn submit(
    db_pool: &sqlx::MySqlPool,
    search_job_config: &SearchJobConfig,
) -> Result<QueryJobId, crate::Error> {
    const QUERY: &str =
        formatcp!("INSERT INTO `{QUERY_JOBS_TABLE_NAME}` (`job_config`, `type`) VALUES (?, ?)");

    let query_result = sqlx::query(QUERY)
        .bind(rmp_serde::to_vec_named(search_job_config)?)
        .bind(QueryJobType::SearchOrAggregation)
        .execute(db_pool)
        .await?;

    let query_job_id = query_result.last_insert_id();
    Ok(QueryJobId::try_from(query_job_id)
        .expect("the inserted query job's ID should fit in `QueryJobId`"))
}

/// Requests the cancellation of a query job.
///
/// # Returns
///
/// The query job's cancellability before the request on success, or `None` if the query job
/// doesn't exist. The query job is marked only if it was [`Cancellability::Cancellable`].
///
/// # Errors
///
/// Returns an error if:
///
/// * Forwards [`sqlx::Pool::begin`]'s return values on failure.
/// * Forwards [`sqlx::query::QueryScalar::fetch_optional`]'s return values on failure.
/// * Forwards [`sqlx::query::Query::execute`]'s return values on failure.
/// * Forwards [`sqlx::Transaction::commit`]'s return values on failure.
pub async fn cancel(
    db_pool: &sqlx::MySqlPool,
    query_job_id: QueryJobId,
) -> Result<Option<Cancellability>, crate::Error> {
    const SELECT_QUERY: &str =
        formatcp!("SELECT status FROM `{QUERY_JOBS_TABLE_NAME}` WHERE id = ? FOR UPDATE");
    const UPDATE_QUERY: &str =
        formatcp!("UPDATE `{QUERY_JOBS_TABLE_NAME}` SET status = ? WHERE id = ?");

    let mut tx = db_pool.begin().await?;
    let Some(status) = sqlx::query_scalar::<_, QueryJobStatus>(SELECT_QUERY)
        .bind(query_job_id)
        .fetch_optional(&mut *tx)
        .await?
    else {
        return Ok(None);
    };

    let cancellability = Cancellability::from_status(status);
    if matches!(cancellability, Cancellability::Cancellable) {
        sqlx::query(UPDATE_QUERY)
            .bind(QueryJobStatus::CancelRequested)
            .bind(query_job_id)
            .execute(&mut *tx)
            .await?;
        tx.commit().await?;
    }
    Ok(Some(cancellability))
}

/// Retrieves the status of a previously submitted query job.
///
/// # Returns
///
/// The current status of the query job on success.
///
/// # Errors
///
/// Returns an error if:
///
/// * Forwards [`sqlx::query::QueryScalar::fetch_one`]'s return values on failure.
pub async fn get_status(
    db_pool: &sqlx::MySqlPool,
    query_job_id: QueryJobId,
) -> Result<QueryJobStatus, crate::Error> {
    const QUERY: &str = formatcp!("SELECT status FROM `{QUERY_JOBS_TABLE_NAME}` WHERE id = ?");

    Ok(sqlx::query_scalar::<_, QueryJobStatus>(QUERY)
        .bind(query_job_id)
        .fetch_one(db_pool)
        .await?)
}
