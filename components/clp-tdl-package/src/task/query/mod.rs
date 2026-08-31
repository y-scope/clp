//! The query tasks: the `#[task]` wrappers Spider invokes and their implementations.

use clp_rust_utils::task_io::query::ClpSQueryOption;
use clp_rust_utils::task_io::query::OutputHandle;
use non_empty_string::NonEmptyString;
use spider_tdl::TaskContext;
use spider_tdl::TdlError;
use spider_tdl::task;

mod search;

/// Queries one CLP-S archive and writes its matches directly to the results cache.
///
/// # Errors
///
/// Returns an error if:
///
/// * [`TdlError::ExecutionError`] if [`search::search`] fails.
#[task(name = "query::clp_s_query_to_results_cache")]
pub(crate) fn clp_s_query_to_results_cache_task(
    ctx: TaskContext,
    query_job_id: i32,
    clp_s_query_option: ClpSQueryOption,
    output_handle: OutputHandle,
    dataset: Option<NonEmptyString>,
    archive_id: String,
) -> Result<(), TdlError> {
    search::search(
        &ctx,
        crate::common::spider_task_executor_config(),
        query_job_id,
        &clp_s_query_option,
        &output_handle,
        dataset.as_ref().map(NonEmptyString::as_str),
        archive_id,
    )
    .map_err(|e| TdlError::ExecutionError(format!("{e:#}")))
}
