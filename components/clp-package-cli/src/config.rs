//! The API server's URL, read from the CLP package's config.

use std::num::NonZeroU16;
use std::path::Path;
use std::path::PathBuf;

use non_empty_string::NonEmptyString;
use serde::Deserialize;
use url::Url;

/// The environment variables that `sbin/search.sh` forwards to its container, which locate the
/// package config.
pub struct HostEnv {
    /// `CLP_HOME`, the package's directory, which the container mounts at the same path.
    clp_home: Option<PathBuf>,

    /// `HOME`, the user's home directory on the host.
    home: Option<PathBuf>,

    /// `CLP_PWD_HOST`, the working directory on the host.
    pwd_host: Option<PathBuf>,
}

impl HostEnv {
    /// # Returns
    ///
    /// The environment variables of this process.
    #[must_use]
    pub fn from_env() -> Self {
        Self {
            clp_home: std::env::var_os(CLP_HOME_ENV_VAR).map(PathBuf::from),
            home: std::env::var_os(HOME_ENV_VAR).map(PathBuf::from),
            pwd_host: std::env::var_os(PWD_HOST_ENV_VAR).map(PathBuf::from),
        }
    }

    /// Locates the package config in the container.
    ///
    /// Without `host_config_path`, the config is the default one, `$CLP_HOME/etc/clp-config.yaml`.
    /// Otherwise, `host_config_path` is a path on the host, resolved as
    /// `clp_py_utils.core.resolve_host_path_in_container` resolves it: a leading `~` is expanded
    /// with `HOME`, a relative path is resolved against `CLP_PWD_HOST`, and the result is located
    /// through the host's root directory, which the container mounts at `/mnt/host`.
    ///
    /// # Returns
    ///
    /// The path to read the package config from on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`Error::MissingEnvVar`] if a variable that locating the config needs isn't set.
    /// * [`Error::RelativeHostPath`] if the config's path on the host is still relative once it's
    ///   resolved.
    pub fn config_path(&self, host_config_path: Option<&Path>) -> Result<PathBuf, Error> {
        use std::path::Component;

        let Some(host_config_path) = host_config_path else {
            let clp_home = self
                .clp_home
                .as_deref()
                .ok_or(Error::MissingEnvVar(CLP_HOME_ENV_VAR))?;
            return Ok(clp_home.join(DEFAULT_CONFIG_RELATIVE_PATH));
        };

        let mut components = host_config_path.components();
        let host_config_path = match components.next() {
            Some(Component::Normal(first_component)) if first_component == "~" => self
                .home
                .as_deref()
                .ok_or(Error::MissingEnvVar(HOME_ENV_VAR))?
                .join(components.as_path()),
            _ => host_config_path.to_path_buf(),
        };
        let host_config_path = if host_config_path.is_relative() {
            self.pwd_host
                .as_deref()
                .ok_or(Error::MissingEnvVar(PWD_HOST_ENV_VAR))?
                .join(host_config_path)
        } else {
            host_config_path
        };
        let Ok(host_config_path_from_root) = host_config_path.strip_prefix("/") else {
            return Err(Error::RelativeHostPath(host_config_path));
        };
        Ok(Path::new(CONTAINER_HOST_ROOT).join(host_config_path_from_root))
    }
}

/// An error returned when the package config can't be located, or the API server's URL can't be
/// read from it.
#[derive(Debug, thiserror::Error)]
pub enum Error {
    #[error("the environment variable `{0}` isn't set")]
    MissingEnvVar(&'static str),

    #[error(
        "the package config's path on the host, {0:?}, is relative once `~` is expanded and it's \
         resolved against `CLP_PWD_HOST`"
    )]
    RelativeHostPath(PathBuf),

    #[error("failed to read the package config: {0}")]
    Read(#[source] clp_rust_utils::Error),

    #[error("invalid `api_server.host` {host:?} in the package config: {source}")]
    InvalidApiServerHost {
        host: String,
        source: url::ParseError,
    },
}

/// Reads the API server's URL from the package config at `config_path`.
///
/// A config without `api_server`, or an empty config, configures the API server with its default
/// host and port, as the Python package utilities do.
///
/// # Returns
///
/// On success:
///
/// * The API server's URL.
/// * `None` if the config disables the API server with `api_server: null`.
///
/// # Errors
///
/// Returns an error if:
///
/// * [`Error::Read`] if the config can't be read, or doesn't deserialize.
/// * Forwards [`ApiServer::into_url`]'s return values on failure.
pub fn read_api_server_url(config_path: &Path) -> Result<Option<Url>, Error> {
    let config: Option<PackageConfig> =
        clp_rust_utils::serde::yaml::from_path(config_path).map_err(Error::Read)?;
    // Like `clp_package_utils.general.load_config_file`, an empty config means the default one.
    config
        .unwrap_or_default()
        .api_server
        .map(ApiServer::into_url)
        .transpose()
}

const CLP_HOME_ENV_VAR: &str = "CLP_HOME";

const HOME_ENV_VAR: &str = "HOME";

const PWD_HOST_ENV_VAR: &str = "CLP_PWD_HOST";

/// The path of the default package config, relative to `CLP_HOME`.
const DEFAULT_CONFIG_RELATIVE_PATH: &str = "etc/clp-config.yaml";

/// The directory at which the container mounts the host's root directory.
const CONTAINER_HOST_ROOT: &str = "/mnt/host";

/// The part of the package config, `clp_py_utils.clp_config.ClpConfig`, that configures the API
/// server.
///
/// # NOTE
///
/// * This type is partially defined: unused fields are omitted and discarded through
///   deserialization.
/// * The default values must be kept in sync with the Python definition.
#[derive(Deserialize)]
#[serde(default)]
struct PackageConfig {
    /// The API server's config, or `None` if the API server is disabled.
    api_server: Option<ApiServer>,
}

impl Default for PackageConfig {
    fn default() -> Self {
        Self {
            api_server: Some(ApiServer::default()),
        }
    }
}

/// Mirror of `clp_py_utils.clp_config.ApiServer`. Must be kept in sync.
///
/// # NOTE
///
/// * This type is partially defined: unused fields are omitted and discarded through
///   deserialization.
/// * The default values must be kept in sync with the Python definition.
#[derive(Deserialize)]
#[serde(default)]
struct ApiServer {
    host: NonEmptyString,
    port: NonZeroU16,
}

impl ApiServer {
    /// # Returns
    ///
    /// The API server's URL on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`Error::InvalidApiServerHost`] if `host` isn't a valid host of a URL.
    ///
    /// # Panics
    ///
    /// Panics if `http://localhost` doesn't parse, or if an HTTP URL rejects a port, neither of
    /// which can happen.
    fn into_url(self) -> Result<Url, Error> {
        let mut url = Url::parse("http://localhost").expect("the URL should be valid");
        url.set_host(Some(self.host.as_str()))
            .map_err(|source| Error::InvalidApiServerHost {
                host: self.host.into_inner(),
                source,
            })?;
        url.set_port(Some(self.port.get()))
            .expect("an HTTP URL should be able to have a port");
        Ok(url)
    }
}

impl Default for ApiServer {
    fn default() -> Self {
        const DEFAULT_PORT: NonZeroU16 = NonZeroU16::new(3001).expect("3001 is non-zero");

        Self {
            host: NonEmptyString::new("localhost".to_owned()).expect("`localhost` is non-empty"),
            port: DEFAULT_PORT,
        }
    }
}

#[cfg(test)]
mod tests {
    use std::io::Write;

    use anyhow::Result;
    use tempfile::NamedTempFile;

    use super::*;

    const BASE_HOST_ENV: HostEnv = HostEnv {
        clp_home: None,
        home: None,
        pwd_host: None,
    };

    /// Reads the API server's URL from a package config whose content is `config`.
    ///
    /// # Returns
    ///
    /// The result of [`read_api_server_url`] on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`NamedTempFile::new`]'s return values on failure.
    /// * Forwards [`Write::write_all`]'s return values on failure.
    fn read_api_server_url_from(config: &str) -> Result<Result<Option<Url>, Error>> {
        let mut config_file = NamedTempFile::new()?;
        config_file.write_all(config.as_bytes())?;
        Ok(read_api_server_url(config_file.path()))
    }

    #[test]
    fn config_without_api_server_has_the_default_url() -> Result<()> {
        for config in [
            "telemetry:\n  disable: true\n",
            "",
            "# All commented out.\n",
        ] {
            assert_eq!(
                read_api_server_url_from(config)??,
                Some(Url::parse("http://localhost:3001")?),
                "config: {config:?}"
            );
        }
        Ok(())
    }

    #[test]
    fn config_with_a_null_api_server_has_no_url() -> Result<()> {
        assert_eq!(read_api_server_url_from("api_server: null\n")??, None);
        Ok(())
    }

    #[test]
    fn config_with_an_api_server_host_and_port_has_their_url() -> Result<()> {
        for (config, expected_url) in [
            (
                "api_server:\n  host: \"10.0.0.1\"\n  port: 30301\n  query_job_polling: {}\n",
                "http://10.0.0.1:30301",
            ),
            (
                "api_server:\n  host: \"clp-host\"\n",
                "http://clp-host:3001",
            ),
            ("api_server:\n  port: 30301\n", "http://localhost:30301"),
        ] {
            assert_eq!(
                read_api_server_url_from(config)??,
                Some(Url::parse(expected_url)?),
                "config: {config:?}"
            );
        }
        Ok(())
    }

    #[test]
    fn config_with_an_invalid_api_server_host_is_rejected() -> Result<()> {
        let error = read_api_server_url_from("api_server:\n  host: \"localhost/path\"\n")?
            .expect_err("a host with a path should be rejected");

        assert!(
            matches!(&error, Error::InvalidApiServerHost { host, .. } if host == "localhost/path"),
            "unexpected error: {error:?}"
        );
        Ok(())
    }

    #[test]
    fn config_path_defaults_to_the_config_in_clp_home() -> Result<()> {
        let host_env = HostEnv {
            clp_home: Some(PathBuf::from("/opt/clp")),
            ..BASE_HOST_ENV
        };

        assert_eq!(
            host_env.config_path(None)?,
            Path::new("/opt/clp/etc/clp-config.yaml")
        );
        Ok(())
    }

    #[test]
    fn config_path_locates_a_host_path_through_the_host_root() -> Result<()> {
        let host_env = HostEnv {
            home: Some(PathBuf::from("/home/user")),
            pwd_host: Some(PathBuf::from("/home/user/clp")),
            ..BASE_HOST_ENV
        };

        for (host_config_path, expected_config_path) in [
            ("/etc/clp-config.yaml", "/mnt/host/etc/clp-config.yaml"),
            (
                "etc/clp-config.yaml",
                "/mnt/host/home/user/clp/etc/clp-config.yaml",
            ),
            (
                "../clp-config.yaml",
                "/mnt/host/home/user/clp/../clp-config.yaml",
            ),
            ("~/clp-config.yaml", "/mnt/host/home/user/clp-config.yaml"),
        ] {
            assert_eq!(
                host_env.config_path(Some(Path::new(host_config_path)))?,
                Path::new(expected_config_path),
                "host config path: {host_config_path:?}"
            );
        }
        Ok(())
    }

    #[test]
    fn config_path_fails_without_the_env_vars_it_needs() {
        for (host_config_path, missing_env_var) in [
            (None, CLP_HOME_ENV_VAR),
            (Some("~/clp-config.yaml"), HOME_ENV_VAR),
            (Some("clp-config.yaml"), PWD_HOST_ENV_VAR),
        ] {
            let result = BASE_HOST_ENV.config_path(host_config_path.map(Path::new));

            assert!(
                matches!(result, Err(Error::MissingEnvVar(env_var)) if env_var == missing_env_var),
                "host config path: {host_config_path:?}, result: {result:?}"
            );
        }
    }

    #[test]
    fn config_path_fails_if_the_host_path_stays_relative() {
        let host_env = HostEnv {
            pwd_host: Some(PathBuf::new()),
            ..BASE_HOST_ENV
        };

        let result = host_env.config_path(Some(Path::new("clp-config.yaml")));

        assert!(
            matches!(
                &result,
                Err(Error::RelativeHostPath(path)) if path == Path::new("clp-config.yaml")
            ),
            "result: {result:?}"
        );
    }
}
