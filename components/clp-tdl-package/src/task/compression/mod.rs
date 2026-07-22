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
    let outputs = ctx
        .get_task_graph_outputs()?
        .ok_or_else(|| TdlError::ExecutionError("commit must run as a commit task".to_owned()))?;
    let outputs: Vec<CompressionTaskOutput> = outputs
        .iter()
        .map(|output| rmp_serde::from_slice(output))
        .collect::<Result<_, _>>()
        .map_err(|e| TdlError::DeserializationError(e.to_string()))?;
    let dataset = outputs.first().and_then(|output| output.dataset.clone());
    if outputs.iter().any(|output| output.dataset != dataset) {
        return Err(TdlError::ExecutionError(
            "compression task outputs belong to more than one dataset".to_owned(),
        ));
    }
    let archives = outputs
        .into_iter()
        .flat_map(|output| output.archives)
        .collect();
    crate::common::runtime()
        .block_on(commit::commit(ctx.job_id, dataset, archives))
        .inspect_err(|e| {
            tracing::error!(spider_job_id = % ctx.job_id, error = ? e, "CLP commit task failed.");
        })
        .map_err(|e| TdlError::ExecutionError(format!("{e:#}")))
}
