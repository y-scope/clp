//! The compression tasks: the `#[task]` wrappers Spider invokes and their implementations.

use clp_rust_utils::task_io::compression::{
    ClpSCompressionOption,
    CompressionTaskOutput,
    S3InputSource,
};
use spider_tdl::{TaskContext, TdlError, task};

mod commit;
mod compress;

#[task(name = "compression::clp_s_s3_compress")]
pub(crate) fn s3_compress_task(
    ctx: TaskContext,
    clp_s_option: ClpSCompressionOption,
    dataset: Option<String>,
    input_source: S3InputSource,
) -> Result<CompressionTaskOutput, TdlError> {
    compress::compress(
        &ctx,
        crate::common::spider_task_executor_config(),
        &clp_s_option,
        dataset,
        input_source,
    )
    .map_err(|e| TdlError::ExecutionError(format!("{e:#}")))
}

#[task(name = "compression::commit")]
pub(crate) fn commit_task(ctx: TaskContext) -> Result<(), TdlError> {
    tracing::info!(job_id = % ctx.job_id, "CLP commit task started.");
    unimplemented!("the clp-s commit task is not implemented yet")
}
