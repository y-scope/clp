//! The query-task signatures registered with Spider.

use clp_rust_utils::job_config::QueryJobId;
use clp_rust_utils::task_io::query::ClpSQueryOption;
use clp_rust_utils::task_io::query::OutputHandle;
use non_empty_string::NonEmptyString;
use spider_tdl::TaskContext;
use spider_tdl::task;

#[task(name = "query::clp_s_search")]
pub(crate) fn clp_s_search_task(
    _ctx: TaskContext,
    _query_job_id: QueryJobId,
    _clp_s_query_option: ClpSQueryOption,
    _dataset: Option<NonEmptyString>,
    _archive_id: NonEmptyString,
    _output_handle: OutputHandle,
) -> Result<(), spider_tdl::TdlError> {
    todo!("clp-s search task is not implemented")
}
