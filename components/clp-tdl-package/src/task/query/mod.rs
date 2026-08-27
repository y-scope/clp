//! The query-task signatures registered with Spider.

use clp_rust_utils::task_io::query::ClpSQueryOption;
use clp_rust_utils::task_io::query::QueryTaskOutput;
use spider_tdl::TaskContext;
use spider_tdl::TdlError;
use spider_tdl::task;

/// Queries one CLP-S archive and writes its matches directly to the results cache.
#[task(name = "query::clp_s_query_to_results_cache")]
pub(crate) fn clp_s_query_to_results_cache_task(
    ctx: TaskContext,
    query_job_id: i32,
    clp_s_query_option: ClpSQueryOption,
    dataset: String,
    archive_id: String,
) -> Result<QueryTaskOutput, TdlError> {
    let _ = (ctx, query_job_id, clp_s_query_option, dataset, archive_id);
    todo!("Implement the CLP-S results-cache query task")
}
