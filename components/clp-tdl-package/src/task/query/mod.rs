//! The query tasks: the `#[task]` wrappers Spider invokes and their implementations.

use clp_rust_utils::dataset::DatasetName;
use clp_rust_utils::job_config::QueryJobId;
use clp_rust_utils::job_config::QueryOutput;
use clp_rust_utils::job_config::SearchQuery;
use clp_rust_utils::task_io::extraction::ExtractTaskInput;
use clp_rust_utils::types::ArchiveId;
use spider_tdl::TaskContext;
use spider_tdl::TdlError;
use spider_tdl::task;

mod archive_input;
mod commit;
mod extract;
mod results_cache;
mod search;

#[task(name = "query::clp_s_search")]
pub(crate) fn clp_s_search_task(
    ctx: TaskContext,
    query_job_id: QueryJobId,
    query: SearchQuery,
    dataset: DatasetName,
    archive_id: ArchiveId,
    output: QueryOutput,
) -> Result<(), TdlError> {
    search::search(
        &ctx,
        crate::common::spider_task_executor_config(),
        query_job_id,
        &query,
        archive_id,
        &dataset,
        &output,
    )
    .map_err(|e| TdlError::ExecutionError(format!("{e:#}")))
}

/// Extracts and publishes the archive's ordered JSON stream.
///
/// # Errors
///
/// Returns [`TdlError::ExecutionError`] if extraction or publication fails.
#[task(name = "query::clp_s_extract_json")]
pub(crate) fn clp_s_extract_json_task(
    ctx: TaskContext,
    input: ExtractTaskInput,
) -> Result<(), TdlError> {
    extract::extract(
        &crate::common::runtime(),
        crate::common::clp_home(),
        crate::common::spider_task_executor_config(),
        ctx.task_instance_id,
        &input,
    )
    .map_err(|error| TdlError::ExecutionError(format!("{error:#}")))
}

#[task(name = "query::commit_timeline")]
pub(crate) fn commit_timeline_task(ctx: TaskContext) -> Result<(), TdlError> {
    crate::common::runtime()
        .block_on(commit::commit_timeline_results(
            ctx.job_id,
            crate::common::spider_task_executor_config(),
        ))
        .map_err(|error| TdlError::ExecutionError(format!("{error:#}")))
}
