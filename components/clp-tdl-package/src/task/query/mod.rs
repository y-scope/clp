//! The query tasks: the `#[task]` wrappers Spider invokes and their implementations.

use clp_rust_utils::job_config::QueryJobId;
use clp_rust_utils::task_io::query::ClpSQueryOption;
use clp_rust_utils::task_io::query::OutputHandle;
use clp_rust_utils::task_io::query::QueryTaskOutput;
use clp_rust_utils::types::ArchiveId;
use non_empty_string::NonEmptyString;
use spider_tdl::TaskContext;
use spider_tdl::TdlError;
use spider_tdl::task;

mod commit;
mod results_cache;
mod search;

/// Runs an archive's normal search and any configured timeline aggregation.
///
/// # Returns
///
/// Metadata identifying the normal and optional timeline result destinations on success.
///
/// # Errors
///
/// [`TdlError::ExecutionError`] if [`search::search`] fails.
///
/// # Panics
///
/// Panics if the TDL package has not initialized its runtime, configuration, or CLP home, or if
/// the generated timeline collection name is empty (an internal format invariant).
#[task(name = "query::clp_s_search")]
pub(crate) fn clp_s_search_task(
    ctx: TaskContext,
    query_job_id: QueryJobId,
    clp_s_query_option: ClpSQueryOption,
    dataset: Option<NonEmptyString>,
    archive_id: ArchiveId,
    output_handle: OutputHandle,
) -> Result<QueryTaskOutput, TdlError> {
    search::search(
        &ctx,
        crate::common::spider_task_executor_config(),
        query_job_id,
        &clp_s_query_option,
        archive_id,
        dataset.as_ref().map(NonEmptyString::as_str),
        &output_handle,
    )
    .map_err(|e| TdlError::ExecutionError(format!("{e:#}")))?;
    Ok(QueryTaskOutput {
        query_job_id,
        output_handle,
        timeline_collection: clp_s_query_option
            .count_by_time_bucket_size_millisecs
            .map(|_| {
                NonEmptyString::try_from(format!("{query_job_id}-timeline"))
                    .expect("timeline collection contains a job ID")
            }),
    })
}

/// Publishes the timeline histogram after all archive tasks complete successfully.
///
/// # Errors
///
/// * [`TdlError::ExecutionError`] if graph outputs are absent or
///   [`commit::commit_timeline_results`] fails.
/// * [`TdlError::DeserializationError`] if an archive output cannot be decoded as
///   [`QueryTaskOutput`].
/// * Forwards [`TaskContext::get_task_graph_outputs`]'s return values on failure.
///
/// # Panics
///
/// Panics if the TDL package has not initialized its runtime.
#[task(name = "query::commit_timeline")]
pub(crate) fn commit_timeline_task(ctx: TaskContext) -> Result<(), TdlError> {
    let outputs = ctx.get_task_graph_outputs()?.ok_or_else(|| {
        TdlError::ExecutionError("timeline commit requires task graph outputs".to_owned())
    })?;
    let outputs = outputs
        .iter()
        .map(|output| rmp_serde::from_slice(output))
        .collect::<Result<Vec<QueryTaskOutput>, _>>()
        .map_err(|error| TdlError::DeserializationError(error.to_string()))?;
    crate::common::runtime()
        .block_on(commit::commit_timeline_results(&outputs))
        .map_err(|error| TdlError::ExecutionError(format!("{error:#}")))
}
