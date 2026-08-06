//! Spider TDL task package `clp`: the CLP compression tasks the Spider task executor loads.

pub mod common;
mod task;

use spider_tdl::TdlError;

/// Initializes this package's process-global state.
///
/// # Errors
///
/// Returns an error if:
///
/// * Forwards [`common::init_runtime`]'s return values on failure.
/// * Forwards [`common::init_config`]'s return values on failure.
/// * Forwards [`common::init_clp_home`]'s return values on failure.
/// * Forwards [`common::init_stderr_tracing_subscriber`]'s return values on failure.
fn package_init() -> Result<(), TdlError> {
    common::init_runtime().map_err(|e| TdlError::ExecutionError(format!("{e:#}")))?;
    common::init_config().map_err(|e| TdlError::ExecutionError(format!("{e:#}")))?;
    common::init_clp_home().map_err(|e| TdlError::ExecutionError(format!("{e:#}")))?;
    common::init_stderr_tracing_subscriber().map_err(|e| {
        TdlError::ExecutionError(format!("failed to install the tracing subscriber: {e}"))
    })?;
    Ok(())
}

spider_tdl::register_tdl_package! {
    package_name: "clp",
    init: package_init,
    tasks: [task::compression::s3_compress_task, task::compression::commit_task],
}
