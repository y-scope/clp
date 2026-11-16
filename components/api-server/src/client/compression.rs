//! Compression job usage queries.

use chrono::DateTime;
use chrono::TimeZone;
use chrono::Utc;
use serde::Deserialize;
use serde::Serialize;
use utoipa::IntoParams;
use utoipa::ToSchema;

use super::Client;
use super::ClientError;
use super::CompressionJobStatus;

/// Default job statuses to include when the caller does not specify `job_status`.
/// Covers all terminal states that consumed compute resources (succeeded, failed).
pub const DEFAULT_JOB_STATUSES: &[CompressionJobStatus] = &[
    CompressionJobStatus::Succeeded,
    CompressionJobStatus::Failed,
];

/// Query parameters for the compression usage endpoint.
#[derive(Deserialize, IntoParams)]
#[into_params(parameter_in = Query)]
pub struct CompressionUsageParams {
    /// Start of usage window (epoch milliseconds, inclusive).
    pub time_range_begin_millisecs: i64,
    /// End of usage window (epoch milliseconds, exclusive).
    pub time_range_end_millisecs: i64,
    /// Job statuses to include as a comma-separated list (e.g.
    /// `job_status=succeeded,failed`). Recognized values (case-insensitive):
    /// `PENDING`, `RUNNING`, `SUCCEEDED`, `FAILED`.
    /// Defaults to `SUCCEEDED,FAILED` (all terminal states).
    #[serde(default)]
    pub job_status: Option<String>,
    /// Maximum number of jobs to return. Must be > 0; defaults to 1000.
    #[serde(default = "default_limit")]
    pub limit: i64,
}

/// Validated parameters produced by [`TryFrom<CompressionUsageParams>`].
pub struct ValidatedCompressionUsageParams {
    pub time_range_begin: DateTime<Utc>,
    pub time_range_end: DateTime<Utc>,
    pub job_statuses: Vec<CompressionJobStatus>,
    pub limit: i64,
}

impl TryFrom<CompressionUsageParams> for ValidatedCompressionUsageParams {
    type Error = ClientError;

    /// Validates the parameters and resolves the requested job statuses into
    /// [`CompressionJobStatus`] variants.
    ///
    /// # Errors
    ///
    /// Returns [`ClientError::InvalidInput`] if:
    /// - `time_range_begin_millisecs > time_range_end_millisecs`
    /// - `time_range_begin_millisecs` or `time_range_end_millisecs` is outside the representable
    ///   range
    /// - `limit <= 0`
    /// - `job_status` contains unrecognized, empty, or duplicate values
    fn try_from(value: CompressionUsageParams) -> Result<Self, Self::Error> {
        if value.time_range_begin_millisecs > value.time_range_end_millisecs {
            return Err(ClientError::InvalidInput(
                "time_range_begin_millisecs must be <= time_range_end_millisecs".to_owned(),
            ));
        }
        if value.limit <= 0 {
            return Err(ClientError::InvalidInput("limit must be > 0".to_owned()));
        }
        let time_range_begin = Utc
            .timestamp_millis_opt(value.time_range_begin_millisecs)
            .earliest();
        let time_range_end = Utc
            .timestamp_millis_opt(value.time_range_end_millisecs)
            .earliest();
        let (Some(time_range_begin), Some(time_range_end)) = (time_range_begin, time_range_end)
        else {
            return Err(ClientError::InvalidInput(
                "timestamp out of representable range".to_owned(),
            ));
        };
        let job_statuses = match &value.job_status {
            Some(s) => {
                let parsed: Vec<CompressionJobStatus> = s
                    .split(',')
                    .map(str::trim)
                    .filter(|t| !t.is_empty())
                    .map(|token| {
                        token.parse().map_err(|_| {
                            ClientError::InvalidInput(format!("Unknown job_status: {token}"))
                        })
                    })
                    .collect::<Result<Vec<_>, _>>()?;
                let mut seen = Vec::new();
                for &status in &parsed {
                    if seen.contains(&status) {
                        return Err(ClientError::InvalidInput(format!(
                            "Duplicate job_status: {status:?}"
                        )));
                    }
                    seen.push(status);
                }
                parsed
            }
            None => DEFAULT_JOB_STATUSES.to_vec(),
        };
        if job_statuses.is_empty() {
            return Err(ClientError::InvalidInput(
                "job_status must contain at least one valid status".to_owned(),
            ));
        }
        Ok(Self {
            time_range_begin,
            time_range_end,
            job_statuses,
            limit: value.limit,
        })
    }
}

/// Resource usage statistics for the compression job with the specified ID.
#[derive(Serialize, sqlx::FromRow, ToSchema)]
pub struct CompressionUsage {
    /// Compression job ID.
    pub id: i32,
    /// Current status of the job.
    #[sqlx(rename = "status", try_from = "i32")]
    pub job_status: CompressionJobStatus,
    /// Time the job was created (epoch milliseconds).
    #[serde(with = "chrono::serde::ts_milliseconds")]
    #[schema(value_type = i64)]
    #[sqlx(rename = "creation_time")]
    pub time_creation_millisecs: DateTime<Utc>,
    /// Time the job started executing (epoch milliseconds).
    #[serde(with = "chrono::serde::ts_milliseconds")]
    #[schema(value_type = i64)]
    #[sqlx(rename = "start_time")]
    pub time_begin_millisecs: DateTime<Utc>,
    /// Wall-clock duration the job ran, in seconds. Absent if the job did not complete.
    #[sqlx(rename = "duration")]
    pub duration_secs: Option<f64>,
    /// Total uncompressed size of input files, in bytes.
    #[sqlx(rename = "uncompressed_size")]
    pub uncompressed_size_bytes: i64,
    /// Total compressed archive size, in bytes.
    #[sqlx(rename = "compressed_size")]
    pub compressed_size_bytes: i64,
    /// Number of tasks the job was split into.
    pub num_tasks: i32,
    /// Sum of all task durations, in seconds. Absent if no tasks reported a duration.
    #[sqlx(rename = "tasks_duration")]
    pub tasks_duration_secs: Option<f64>,
}

impl Client {
    /// Returns compression usage for each job within a time range, optionally
    /// filtered by job outcome.
    ///
    /// # Returns
    ///
    /// A vector of [`CompressionUsage`] (one per job) on success, ordered by
    /// `start_time` descending.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`sqlx::query::Query::fetch_all`]'s return values on failure.
    /// * Forwards [`sqlx::Row::try_get`]'s return values on failure.
    pub async fn get_compression_usage(
        &self,
        params: &ValidatedCompressionUsageParams,
    ) -> Result<Vec<CompressionUsage>, ClientError> {
        // Build the job-status IN clause dynamically so the DB sees a
        // static `IN (?, ?, ...)` predicate and can use an index on `j.status`.
        let placeholders = params
            .job_statuses
            .iter()
            .map(|_| "?")
            .collect::<Vec<_>>()
            .join(", ");
        let job_status_clause = format!(" AND j.status IN ({placeholders})");

        #[rustfmt::skip]
        let sql = format!(
            // MAX() is used on non-aggregated columns because they are all
            // functionally dependent on j.id (the primary key). Since each group
            // contains exactly one row, MAX() simply returns the column's value.
            // This is used instead of MySQL-only ANY_VALUE() for MariaDB compat.
            //
            // sqlx binds DateTime<Utc> as a UTC timestamp, so we compare
            // directly against the DATETIME columns in the database.
            //
            // Task durations are summed regardless of individual task status so
            // that failed jobs still report the compute resources they consumed.
            //
            // INNER JOIN filters out jobs with zero tasks (e.g. failed before
            // task creation) since they consumed no compute resources.
            "SELECT \
              j.id, \
              MAX(j.status) AS status, \
              MAX(j.creation_time) AS creation_time, \
              MAX(j.start_time) AS start_time, \
              MAX(j.duration) AS duration, \
              MAX(j.uncompressed_size) AS uncompressed_size, \
              MAX(j.compressed_size) AS compressed_size, \
              MAX(j.num_tasks) AS num_tasks, \
              SUM(t.duration) AS tasks_duration \
            FROM compression_jobs j \
            JOIN compression_tasks t ON t.job_id = j.id \
            WHERE j.start_time >= ? \
              AND j.start_time < ? \
              {job_status_clause} \
            GROUP BY j.id \
            ORDER BY MAX(j.start_time) DESC \
            LIMIT ?"
        );

        let mut query = sqlx::query_as::<_, CompressionUsage>(&sql)
            .bind(params.time_range_begin)
            .bind(params.time_range_end);
        for &status in &params.job_statuses {
            query = query.bind(i32::from(status));
        }
        query = query.bind(params.limit);
        query.fetch_all(&self.sql_pool).await.map_err(Into::into)
    }
}

const fn default_limit() -> i64 {
    1000
}
