//! Process-global state shared by this package's tasks: the Tokio runtime, the Spider task
//! executor config, and this cdylib's `tracing` subscriber.

use std::path::Path;
use std::path::PathBuf;
use std::sync::OnceLock;

use anyhow::Context;
use clp_rust_utils::clp_config::package::config::SpiderTaskExecutorConfig;

/// Initializes the process-wide Tokio runtime the compression tasks use to drive async S3 I/O.
///
/// Idempotent: a no-op once the runtime is initialized.
///
/// # Errors
///
/// Returns an error if:
///
/// * Forward [`tokio::runtime::Builder::build`]'s return values on failure.
pub fn init_runtime() -> anyhow::Result<()> {
    if TOKIO_RUNTIME.get().is_some() {
        return Ok(());
    }
    let rt = tokio::runtime::Builder::new_multi_thread()
        .enable_all()
        .build()
        .context("failed to build the clp-tdl-package Tokio runtime")?;
    let _ = TOKIO_RUNTIME.set(rt);
    Ok(())
}

/// Initializes the process-wide Spider task executor config from `CLP_CONFIG_PATH`.
///
/// Idempotent: a no-op once the config is initialized.
///
/// # Errors
///
/// Returns an error if:
///
/// * Forwards [`load_spider_task_executor_config_from_env`]'s return values on failure.
pub fn init_config() -> anyhow::Result<()> {
    if SPIDER_TASK_EXECUTOR_CONFIG.get().is_some() {
        return Ok(());
    }
    let cfg = load_spider_task_executor_config_from_env()?;
    let _ = SPIDER_TASK_EXECUTOR_CONFIG.set(cfg);
    Ok(())
}

/// Initializes the process-wide CLP home directory from `CLP_HOME`.
///
/// Idempotent: a no-op once the directory is initialized.
///
/// # Errors
///
/// Returns an error if:
///
/// * Forwards [`std::env::var`]'s return values on failure.
pub fn init_clp_home() -> anyhow::Result<()> {
    if CLP_HOME.get().is_some() {
        return Ok(());
    }
    let clp_home = std::env::var(CLP_HOME_ENV_VAR)
        .with_context(|| format!("failed to read the `{CLP_HOME_ENV_VAR}` environment variable"))?;
    let _ = CLP_HOME.set(PathBuf::from(clp_home));
    Ok(())
}

/// Installs `tracing` subscriber into the cdylib's own global dispatcher writing to `stderr`.
///
/// # Errors
///
/// Returns an error if:
///
/// * Forwards [`tracing_subscriber::fmt::SubscriberBuilder::try_init`]'s return values on failure.
pub fn init_stderr_tracing_subscriber() -> Result<(), Box<dyn std::error::Error + Send + Sync>> {
    tracing_subscriber::fmt()
        .event_format(
            tracing_subscriber::fmt::format()
                .with_level(true)
                .with_target(false)
                .with_file(true)
                .with_line_number(true)
                .json(),
        )
        .with_env_filter(tracing_subscriber::EnvFilter::from_default_env())
        .with_ansi(false)
        .with_writer(std::io::stderr)
        .try_init()
}

/// # Returns
///
/// A reference to the cached executor [`SpiderTaskExecutorConfig`].
///
/// # Panics
///
/// Panics if the config has not been initialized by the TDL package init hook.
#[must_use]
pub fn spider_task_executor_config() -> &'static SpiderTaskExecutorConfig {
    SPIDER_TASK_EXECUTOR_CONFIG.get().expect(
        "Spider task executor config not initialized; the TDL package init hook must run before \
         any task",
    )
}

/// # Returns
///
/// A reference to the cached CLP home directory.
///
/// # Panics
///
/// Panics if the directory has not been initialized by the TDL package init hook.
#[must_use]
pub fn clp_home() -> &'static Path {
    CLP_HOME
        .get()
        .expect("CLP home not initialized; the TDL package init hook must run before any task")
}

/// # Returns
///
/// A handle to the process-wide multithreaded Tokio runtime.
///
/// # Panics
///
/// Panics if the runtime has not been initialized by the TDL package init hook.
#[must_use]
pub fn runtime() -> tokio::runtime::Handle {
    TOKIO_RUNTIME
        .get()
        .expect("Tokio runtime not initialized; the TDL package init hook must run before any task")
        .handle()
        .clone()
}

/// The env var holding the path to the Spider task executor config YAML.
const CLP_CONFIG_PATH_ENV_VAR: &str = "CLP_CONFIG_PATH";

/// The env var holding the path to the CLP installation's home directory.
const CLP_HOME_ENV_VAR: &str = "CLP_HOME";

/// Process-wide CLP home directory, initialized by [`init_clp_home`].
static CLP_HOME: OnceLock<PathBuf> = OnceLock::new();

/// Process-wide multi-threaded Tokio runtime, initialized by [`init_runtime`].
static TOKIO_RUNTIME: OnceLock<tokio::runtime::Runtime> = OnceLock::new();

/// Process-wide cache of the Spider task executor config, initialized by [`init_config`].
static SPIDER_TASK_EXECUTOR_CONFIG: OnceLock<SpiderTaskExecutorConfig> = OnceLock::new();

/// Loads the [`SpiderTaskExecutorConfig`] from the YAML file at the path named by
/// [`CLP_CONFIG_PATH_ENV_VAR`].
///
/// # Returns
///
/// The deserialized [`SpiderTaskExecutorConfig`].
///
/// # Errors
///
/// Returns an error if:
///
/// * Forwards [`std::env::var`]'s return values on failure if failed to read the env var.
/// * Forwards [`clp_rust_utils::serde::yaml::from_path`]'s return values on failure if failed to
///   load the executor config.
fn load_spider_task_executor_config_from_env() -> anyhow::Result<SpiderTaskExecutorConfig> {
    let path = std::env::var(CLP_CONFIG_PATH_ENV_VAR).with_context(|| {
        format!("failed to read the `{CLP_CONFIG_PATH_ENV_VAR}` environment variable")
    })?;
    clp_rust_utils::serde::yaml::from_path(&path)
        .with_context(|| format!("failed to load the Spider task executor config from `{path}`"))
}
