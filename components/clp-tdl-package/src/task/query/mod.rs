//! The query tasks: the `#[task]` wrappers Spider invokes and their implementations.

use clp_rust_utils::job_config::QueryJobId;
use clp_rust_utils::task_io::query::ClpSQueryOption;
use clp_rust_utils::task_io::query::OutputHandle;
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
/// # Errors
///
/// Returns an error if:
///
/// * [`TdlError::ExecutionError`] if [`search::search`] fails.
///
/// # Panics
///
/// Panics if the TDL package has not initialized its runtime, configuration, or CLP home.
#[task(name = "query::clp_s_search")]
pub(crate) fn clp_s_search_task(
    ctx: TaskContext,
    query_job_id: QueryJobId,
    clp_s_query_option: ClpSQueryOption,
    dataset: Option<NonEmptyString>,
    archive_id: ArchiveId,
    output_handle: OutputHandle,
) -> Result<(), TdlError> {
    search::search(
        &ctx,
        crate::common::spider_task_executor_config(),
        query_job_id,
        &clp_s_query_option,
        archive_id,
        dataset.as_ref().map(NonEmptyString::as_str),
        &output_handle,
    )
    .map_err(|e| TdlError::ExecutionError(format!("{e:#}")))
}

/// Publishes the timeline histogram after all archive tasks complete successfully.
///
/// # Errors
///
/// Returns an error if:
///
/// * [`TdlError::ExecutionError`] if [`commit::commit_timeline_results`] fails.
///
/// # Panics
///
/// Panics if the TDL package has not initialized its runtime or configuration.
#[task(name = "query::commit_timeline")]
pub(crate) fn commit_timeline_task(ctx: TaskContext) -> Result<(), TdlError> {
    crate::common::runtime()
        .block_on(commit::commit_timeline_results(
            ctx.job_id,
            crate::common::spider_task_executor_config(),
        ))
        .map_err(|error| TdlError::ExecutionError(format!("{error:#}")))
}
