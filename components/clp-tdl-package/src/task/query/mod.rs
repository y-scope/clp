//! The query tasks: the `#[task]` wrappers Spider invokes and their implementations.

use clp_rust_utils::dataset::DatasetName;
use clp_rust_utils::job_config::QueryJobId;
use clp_rust_utils::job_config::QueryOutput;
use clp_rust_utils::job_config::SearchQuery;
use clp_rust_utils::types::ArchiveId;
use spider_tdl::TaskContext;
use spider_tdl::TdlError;
use spider_tdl::task;

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
