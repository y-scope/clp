//! The query-task signatures registered with Spider.

use clp_rust_utils::task_io::query::ClpSQueryOption;
use clp_rust_utils::task_io::query::QueryTaskOutput;
use spider_tdl::TaskContext;
use spider_tdl::TdlError;
use spider_tdl::task;

/// Queries one CLP-S archive and writes its matches directly to the results cache.
#[task(name = "query::clp_s_query_to_results_cache")]
pub(crate) fn clp_s_query_to_results_cache_task(
    _ctx: TaskContext,
    _query_job_id: i32,
    _clp_s_query_option: ClpSQueryOption,
    _dataset: String,
    _archive_id: String,
) -> Result<(), TdlError> {
    todo!("Implement the CLP-S results-cache query task")
}
