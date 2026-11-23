//! Utilities that the CLP package's command-line tools share.

use std::path::Path;
use std::path::PathBuf;

use anyhow::Context;
use clp_rust_utils::clp_config::package::DEFAULT_CONFIG_FILE_PATH;

/// The directory at which the container mounts the host's root directory.
pub const CONTAINER_HOST_ROOT: &str = "/mnt/host";

/// The exit code of a command that a signal interrupted.
pub const INTERRUPTED_EXIT_CODE: u8 = 130;

/// The most symlinks that [`resolve_host_path`] follows in a path, the same as Linux's limit.
const MAX_NUM_SYMLINKS_FOLLOWED: usize = 40;

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
/// The runtime is shut down without waiting for its blocking tasks, e.g., a write to stdout that an
/// idle pager blocks.
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
    let output = runtime.block_on(async {
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
    });
    runtime.shutdown_background();
    output
}

/// Resolves the path at which the container reads the package config.
///
/// The package config's path on the host is `host_config_path`, or by default,
/// [`DEFAULT_CONFIG_FILE_PATH`] joined onto `CLP_HOME`. A leading `~` component of it is replaced
/// with `HOME`, and a relative path is joined onto `CLP_PWD_HOST`. The resulting absolute path is
/// then resolved by [`resolve_host_path`].
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

    let host_config_path = if let Some(host_config_path) = host_config_path {
        host_config_path.to_path_buf()
    } else {
        let clp_home = var_os("CLP_HOME").context("`CLP_HOME` isn't set")?;
        Path::new(&clp_home).join(DEFAULT_CONFIG_FILE_PATH)
    };
    let host_config_path = match host_config_path.strip_prefix("~") {
        Ok(path_in_home) => {
            let home = var_os("HOME").context("`HOME` isn't set")?;
            Path::new(&home).join(path_in_home)
        }
        Err(_) => host_config_path,
    };
    let host_config_path = if host_config_path.is_relative() {
        let pwd_host = var_os("CLP_PWD_HOST").context("`CLP_PWD_HOST` isn't set")?;
        Path::new(&pwd_host).join(host_config_path)
    } else {
        host_config_path
    };
    anyhow::ensure!(
        host_config_path.is_absolute(),
        "the host path {} is relative",
        host_config_path.display()
    );
    Ok(resolve_host_path(&host_config_path))
}

/// Resolves `host_path`, an absolute path on the host, into the path at which the container
/// reaches it under [`CONTAINER_HOST_ROOT`].
///
/// The path is resolved one component at a time, the way the host resolves it: a `..` component
/// moves up a directory, but never above the host's root directory, and a symlink is replaced with
/// its target, which starts again from the host's root directory if it's absolute. Once
/// [`MAX_NUM_SYMLINKS_FOLLOWED`] symlinks are followed (e.g., in a symlink loop), the remaining
/// symlinks are left as they are.
///
/// # Returns
///
/// The path in the container.
fn resolve_host_path(host_path: &Path) -> PathBuf {
    let container_host_root = Path::new(CONTAINER_HOST_ROOT);
    let mut path = container_host_root.to_path_buf();
    // The components that are left to resolve, with the next one last.
    let mut components = reversed_components(host_path);
    let mut num_symlinks_followed = 0;
    while let Some(component) = components.pop() {
        if component == ".." {
            if path != container_host_root {
                path.pop();
            }
            continue;
        }
        path.push(component);
        if num_symlinks_followed == MAX_NUM_SYMLINKS_FOLLOWED {
            continue;
        }
        let Ok(target) = std::fs::read_link(&path) else {
            continue;
        };
        num_symlinks_followed += 1;
        if target.is_absolute() {
            container_host_root.clone_into(&mut path);
        } else {
            path.pop();
        }
        components.extend(reversed_components(&target));
    }
    path
}

/// # Returns
///
/// The normal and `..` components of `path`, in reverse order.
fn reversed_components(path: &Path) -> Vec<std::ffi::OsString> {
    use std::path::Component;

    path.components()
        .rev()
        .filter_map(|component| match component {
            Component::Normal(name) => Some(name.to_owned()),
            Component::ParentDir => Some("..".into()),
            Component::Prefix(_) | Component::RootDir | Component::CurDir => None,
        })
        .collect()
}
