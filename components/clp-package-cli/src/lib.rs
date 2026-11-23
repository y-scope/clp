//! Utilities that the CLP package's command-line tools share.

use std::path::Path;
use std::path::PathBuf;

use anyhow::Context;
use clp_rust_utils::clp_config::package::DEFAULT_CONFIG_FILE_PATH;

/// The directory at which the container mounts the host's root directory.
pub const CONTAINER_HOST_ROOT: &str = "/mnt/host";

/// The exit code of a command that a signal interrupted.
pub const INTERRUPTED_EXIT_CODE: u8 = 130;

/// Sets up logging to stderr, at the debug level for `target`'s events if `is_verbose`, and at the
/// info level otherwise.
pub fn set_up_logging(target: &str, is_verbose: bool) {
    use std::io::IsTerminal;

    let filter = if is_verbose {
        format!("info,{target}=debug")
    } else {
        "info".to_owned()
    };
    tracing_subscriber::fmt()
        .with_env_filter(tracing_subscriber::EnvFilter::new(filter))
        .with_writer(std::io::stderr)
        .with_ansi(std::io::stderr().is_terminal())
        .with_target(false)
        .init();
}

/// Runs `future` to completion on a current-thread runtime, unless SIGINT or SIGTERM interrupts it.
///
/// # Type Parameters
///
/// * `FutureType` - The future to run.
///
/// # Returns
///
/// `future`'s output, or `None` if a signal interrupts it, on success.
///
/// # Errors
///
/// Returns an error if:
///
/// * Forwards [`tokio::runtime::Builder::build`]'s return values on failure.
/// * Forwards [`tokio::signal::unix::signal`]'s return values on failure.
pub fn run_until_signal<FutureType: Future>(
    future: FutureType,
) -> anyhow::Result<Option<FutureType::Output>> {
    use tokio::signal::unix::SignalKind;
    use tokio::signal::unix::signal;

    let runtime = tokio::runtime::Builder::new_current_thread()
        .enable_all()
        .build()
        .context("failed to create the runtime")?;
    runtime.block_on(async {
        // As the container's PID 1, the process has no default action for these signals.
        let mut interrupt =
            signal(SignalKind::interrupt()).context("failed to listen for SIGINT")?;
        let mut terminate =
            signal(SignalKind::terminate()).context("failed to listen for SIGTERM")?;
        tokio::select! {
            output = future => Ok(Some(output)),
            _ = interrupt.recv() => Ok(None),
            _ = terminate.recv() => Ok(None),
        }
    })
}

/// Resolves the path at which the container reads the package config.
///
/// Without `host_config_path`, the path is [`DEFAULT_CONFIG_FILE_PATH`] joined onto `CLP_HOME`.
/// Otherwise, `host_config_path` is a path on the host: a leading `~` component is replaced with
/// `HOME`, a relative path is joined onto `CLP_PWD_HOST`, and the resulting absolute path is joined
/// under [`CONTAINER_HOST_ROOT`], where the container mounts the host's root directory. Symlinks
/// and `..` components are left as they are.
///
/// # Returns
///
/// The path of the package config in the container on success.
///
/// # Errors
///
/// Returns an error if:
///
/// * [`anyhow::Error`] if:
///   * An environment variable that the resolution needs isn't set.
///   * The path on the host is still relative once it's joined onto `CLP_PWD_HOST`.
pub fn resolve_config_path(host_config_path: Option<&Path>) -> anyhow::Result<PathBuf> {
    use std::env::var_os;

    let Some(host_config_path) = host_config_path else {
        let clp_home = var_os("CLP_HOME").context("`CLP_HOME` isn't set")?;
        return Ok(Path::new(&clp_home).join(DEFAULT_CONFIG_FILE_PATH));
    };
    let host_config_path = match host_config_path.strip_prefix("~") {
        Ok(path_in_home) => {
            let home = var_os("HOME").context("`HOME` isn't set")?;
            Path::new(&home).join(path_in_home)
        }
        Err(_) => host_config_path.to_path_buf(),
    };
    let host_config_path = if host_config_path.is_relative() {
        let pwd_host = var_os("CLP_PWD_HOST").context("`CLP_PWD_HOST` isn't set")?;
        Path::new(&pwd_host).join(host_config_path)
    } else {
        host_config_path
    };
    let host_config_path_from_root = host_config_path
        .strip_prefix("/")
        .with_context(|| format!("the host path {} is relative", host_config_path.display()))?;
    Ok(Path::new(CONTAINER_HOST_ROOT).join(host_config_path_from_root))
}
