//! The compression tasks: the `#[task]` wrappers Spider invokes and their implementations.

use clp_rust_utils::task_io::compression::ClpSCompressionOption;
use clp_rust_utils::task_io::compression::CompressionTaskOutput;
use clp_rust_utils::task_io::compression::S3InputSource;
use spider_tdl::TaskContext;
use spider_tdl::TdlError;
use spider_tdl::task;

mod commit;
mod compress;

#[task(name = "compression::clp_s_s3_compress")]
pub(crate) fn s3_compress_task(
    ctx: TaskContext,
    _clp_s_option: ClpSCompressionOption,
    dataset: Option<String>,
    _input_source: S3InputSource,
) -> Result<CompressionTaskOutput, TdlError> {
    tracing::info!(
        job_id = % ctx.job_id,
        task_id = % ctx.task_id,
        task_instance_id = ctx.task_instance_id,
        dataset = dataset.as_deref().unwrap_or("<default>"),
        "CLP compression task started."
    );
    unimplemented!("the clp-s compression task is not implemented yet")
}

#[task(name = "compression::commit")]
pub(crate) fn commit_task(ctx: TaskContext) -> Result<(), TdlError> {
    tracing::info!(job_id = % ctx.job_id, "CLP commit task started.");
    unimplemented!("the clp-s commit task is not implemented yet")
}
