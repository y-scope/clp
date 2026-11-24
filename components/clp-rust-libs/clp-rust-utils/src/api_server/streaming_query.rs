//! The request and the events' data of the API server's streaming query route `POST /query/stream`.

use serde::Deserialize;
use serde::Serialize;
use utoipa::ToSchema;

use crate::job_config::QueryJobStatus;
use crate::job_config::SearchQuery;
use crate::types::ArchiveUuid;

/// Defines the request configuration for submitting a query job whose results are streamed instead
/// of buffered.
#[derive(Debug, Deserialize, Serialize, ToSchema)]
#[serde(deny_unknown_fields)]
pub struct StreamingQueryConfig {
    /// The query to run.
    pub query: SearchQuery,
}

/// The data of a streaming query's event.
#[derive(ToSchema)]
#[serde(untagged)]
pub enum StreamingQueryEventData {
    Job(StreamingQueryJobEvent),
    Result(StreamingQueryResultEvent),
    End(StreamingQueryEndEvent),
    Error(StreamingQueryErrorEvent),
}

/// The data of a streaming query's `job` event.
#[derive(Deserialize, Serialize, ToSchema)]
#[serde(deny_unknown_fields)]
pub struct StreamingQueryJobEvent {
    /// The ID of the query's job.
    pub query_job_id: u64,
}

/// The data of a streaming query's result event.
#[derive(Serialize, ToSchema)]
#[serde(deny_unknown_fields)]
pub struct StreamingQueryResultEvent {
    /// The UUID of the archive that contains the result.
    // Search results keep reporting the UUID under the `archive_id` key of the public API.
    #[serde(rename = "archive_id")]
    #[schema(value_type = String)]
    pub archive_uuid: ArchiveUuid,

    /// The result's timestamp, in milliseconds since the Unix epoch.
    pub timestamp_millisecs: i64,

    /// The result's message with its trailing newline removed.
    pub message: String,
}

/// The data of a streaming query's `end` event.
#[derive(Deserialize, Serialize, ToSchema)]
#[serde(deny_unknown_fields)]
pub struct StreamingQueryEndEvent {
    /// The terminal status of the query's job.
    pub status: QueryJobStatus,

    /// The number of results streamed.
    pub num_results_emitted: u64,
}

/// The data of a streaming query's `error` event.
#[derive(Deserialize, Serialize, ToSchema)]
#[serde(deny_unknown_fields)]
pub struct StreamingQueryErrorEvent {
    /// A generic description of the error.
    pub message: String,
}
