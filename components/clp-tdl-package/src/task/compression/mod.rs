//! The compression tasks: the `#[task]` wrappers Spider invokes and their implementations.

use clp_rust_utils::dataset::DatasetName;
use clp_rust_utils::task_io::compression::ClpSCompressionOption;
use clp_rust_utils::task_io::compression::CompressionTaskOutput;
use clp_rust_utils::task_io::compression::FsInputSource;
use clp_rust_utils::task_io::compression::S3InputSource;
use spider_tdl::TaskContext;
use spider_tdl::TdlError;
use spider_tdl::task;

mod commit;
mod compress;

/// Compresses S3 inputs and publishes the resulting archives.
///
/// # Returns
///
/// The archive metadata on success.
///
/// # Errors
///
/// Returns [`TdlError::ExecutionError`] if [`compress::compress`] fails.
///
/// # Panics
///
/// Panics if required package globals have not been initialized or this function is called from
/// an async runtime context.
#[task(name = "compression::clp_s_s3_compress")]
pub(crate) fn s3_compress_task(
    ctx: TaskContext,
    clp_s_option: ClpSCompressionOption,
    dataset: DatasetName,
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

/// Compresses filesystem inputs and publishes the resulting archives.
///
/// # Returns
///
/// The archive metadata on success.
///
/// # Errors
///
/// Returns [`TdlError::ExecutionError`] if [`compress::compress`] fails.
///
/// # Panics
///
/// Panics if required package globals have not been initialized or this function is called from
/// an async runtime context.
#[task(name = "compression::clp_s_fs_compress")]
pub(crate) fn fs_compress_task(
    ctx: TaskContext,
    clp_s_option: ClpSCompressionOption,
    dataset: DatasetName,
    input_source: FsInputSource,
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

/// Publishes the compression task outputs and completes their job.
///
/// # Errors
///
/// Returns an error if:
///
/// * [`TdlError::ExecutionError`] if this is not a commit task, [`commit::collect_outputs`] fails,
///   or [`commit::commit`] fails.
/// * [`TdlError::DeserializationError`] if an output cannot be decoded.
/// * Forwards [`TaskContext::get_task_graph_outputs`]'s return values on failure.
///
/// # Panics
///
/// Panics if required package globals have not been initialized or this function is called from
/// an async runtime context.
#[task(name = "compression::commit")]
pub(crate) fn commit_task(ctx: TaskContext) -> Result<(), TdlError> {
    tracing::info!(job_id = % ctx.job_id, "CLP commit task started.");
    let outputs = ctx
        .get_task_graph_outputs()?
        .ok_or_else(|| TdlError::ExecutionError("commit must run as a commit task".to_owned()))?;
    let outputs: Vec<CompressionTaskOutput> = outputs
        .iter()
        .map(|output| rmp_serde::from_slice(output))
        .collect::<Result<_, _>>()
        .map_err(|e| TdlError::DeserializationError(e.to_string()))?;
    let output =
        commit::collect_outputs(outputs).map_err(|e| TdlError::ExecutionError(format!("{e:#}")))?;
    crate::common::runtime()
        .block_on(commit::commit(ctx.job_id, &output.dataset, output.archives))
        .inspect_err(|e| {
            tracing::error!(spider_job_id = % ctx.job_id, error = ? e, "CLP commit task failed.");
        })
        .map_err(|e| TdlError::ExecutionError(format!("{e:#}")))
}
