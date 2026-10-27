//! Paths in the logs input, and listing of the logs input's directories.

use std::fmt::Display;
use std::fmt::Formatter;
use std::path::PathBuf;

use clp_rust_utils::clp_config::package::config::LOGS_INPUT_MOUNT_POINT;
use clp_rust_utils::clp_config::package::config::LogsInput;
use clp_rust_utils::clp_config::package::config::StorageEngine;
use clp_rust_utils::types::non_empty_string::ExpectedNonEmpty;
use non_empty_string::NonEmptyString;
use serde::Deserialize;
use serde::Serialize;
use utoipa::ToSchema;

use crate::error::ClientError;
use crate::webui_client::WebuiClient;

/// A path in the logs input, relative to the logs-input mount point ([`LOGS_INPUT_MOUNT_POINT`]).
///
/// A valid logs-input path:
///
/// * Starts with `/`, which denotes the mount point itself.
/// * Has no `..` components.
/// * Has no NUL characters.
///
/// A logs-input path is lexically normalized:
///
/// * `.` components are removed.
/// * Repeated `/`s are collapsed into one.
/// * A trailing `/` is removed, except from the mount point itself, which is `/`.
///
/// A logs-input path serializes as its normalized string. It deserializes from a string, validated
/// and normalized by the same rules as its [`TryFrom<String>`] implementation.
#[derive(Clone, Debug, Deserialize, Eq, PartialEq, Serialize)]
#[serde(try_from = "String", into = "String")]
pub struct LogsInputPath {
    value: String,
}

impl LogsInputPath {
    /// # Returns
    ///
    /// Whether the path denotes the mount point itself.
    fn is_mount_point(&self) -> bool {
        self.value == "/"
    }

    /// # Returns
    ///
    /// A [`ClientError::LogsInputPathUnresolvable`] for the path, caused by `source`.
    fn unresolvable(&self, source: std::io::Error) -> ClientError {
        ClientError::LogsInputPathUnresolvable {
            path: self.clone(),
            source,
        }
    }
}

impl TryFrom<String> for LogsInputPath {
    type Error = ParseLogsInputPathError;

    fn try_from(value: String) -> Result<Self, Self::Error> {
        if !value.starts_with('/') {
            return Err(ParseLogsInputPathError::NotAbsolute(value));
        }
        if value.contains('\0') {
            return Err(ParseLogsInputPathError::NulCharacter(value));
        }
        let mut normalized = String::with_capacity(value.len());
        for component in value.split('/') {
            match component {
                "" | "." => {}
                ".." => return Err(ParseLogsInputPathError::ParentDirectoryComponent(value)),
                name => {
                    normalized.push('/');
                    normalized.push_str(name);
                }
            }
        }
        if normalized.is_empty() {
            normalized.push('/');
        }
        Ok(Self { value: normalized })
    }
}

impl Display for LogsInputPath {
    fn fmt(&self, formatter: &mut Formatter<'_>) -> std::fmt::Result {
        self.value.fmt(formatter)
    }
}

impl From<LogsInputPath> for String {
    fn from(path: LogsInputPath) -> Self {
        path.value
    }
}

/// An error returned when parsing an invalid logs-input path.
#[derive(Debug, Eq, PartialEq, thiserror::Error)]
pub enum ParseLogsInputPathError {
    #[error("invalid logs-input path {0:?}: paths must start with `/`")]
    NotAbsolute(String),

    #[error("invalid logs-input path {0:?}: paths must not have `..` components")]
    ParentDirectoryComponent(String),

    #[error("invalid logs-input path {0:?}: paths must not contain NUL characters")]
    NulCharacter(String),
}

/// An entry of a directory in the logs input.
#[derive(Debug, Eq, PartialEq, Serialize, ToSchema)]
pub struct DirEntry {
    /// Whether the entry is a directory, or a symlink to a directory under the logs-input mount
    /// point, which the `WebUI`'s path picker can expand.
    pub is_expandable: bool,
    /// The entry's file name.
    pub name: String,
    /// The logs-input path of the directory that contains the entry.
    #[schema(value_type = String)]
    pub parent_path: LogsInputPath,
}

/// The logs-input mount point, resolved to its canonical form.
#[derive(Debug)]
pub(super) struct LogsInputMountPoint {
    path: NonEmptyString,
    canonical_path: PathBuf,
}

impl LogsInputMountPoint {
    /// Factory function.
    ///
    /// Resolves the mount point at `path` to its canonical form.
    ///
    /// # Returns
    ///
    /// The resolved mount point on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`ClientError::LogsInputMountPointUnavailable`] if `path` can't be resolved.
    pub(super) async fn resolve(path: NonEmptyString) -> Result<Self, ClientError> {
        let canonical_path = tokio::fs::canonicalize(path.as_str())
            .await
            .map_err(ClientError::LogsInputMountPointUnavailable)?;
        Ok(Self {
            path,
            canonical_path,
        })
    }

    /// Locates a logs-input path under the mount point, and checks that it doesn't resolve to
    /// outside the mount point through a symlink.
    ///
    /// Symlinks are only resolved for the check: the returned location is the path's lexical
    /// location under the mount point.
    ///
    /// # Returns
    ///
    /// The path's lexical location under the mount point on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`ClientError::LogsInputPathNotFound`] if `path` doesn't exist or has a component that is
    ///   too long.
    /// * [`ClientError::LogsInputPathOutsideMountPoint`] if `path` resolves to outside the mount
    ///   point.
    /// * [`ClientError::LogsInputPathUnresolvable`] if `path` can't be resolved for any other
    ///   reason (e.g., a symlink loop or a permission error).
    pub(super) async fn locate(&self, path: &LogsInputPath) -> Result<NonEmptyString, ClientError> {
        let mut location = self.path.clone();
        if !path.is_mount_point() {
            location.push_str(&path.value);
        }

        let canonical_location =
            tokio::fs::canonicalize(location.as_str())
                .await
                .map_err(|error| match error.kind() {
                    std::io::ErrorKind::NotFound
                    | std::io::ErrorKind::NotADirectory
                    | std::io::ErrorKind::InvalidFilename => {
                        ClientError::LogsInputPathNotFound(path.clone())
                    }
                    _ => path.unresolvable(error),
                })?;
        if !canonical_location.starts_with(&self.canonical_path) {
            return Err(ClientError::LogsInputPathOutsideMountPoint(path.clone()));
        }
        Ok(location)
    }

    /// Lists the entries of the directory at a logs-input path under the mount point.
    ///
    /// # Returns
    ///
    /// The directory's entries on success, in no particular order, or an empty list if `path`
    /// isn't a directory.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`ClientError::LogsInputPathUnresolvable`] if the directory's entries can't be read.
    /// * Forwards [`Self::locate`]'s return values on failure.
    async fn list(&self, path: &LogsInputPath) -> Result<Vec<DirEntry>, ClientError> {
        let location = self.locate(path).await?;
        let unresolvable = |error| path.unresolvable(error);
        if !tokio::fs::metadata(location.as_str())
            .await
            .map_err(unresolvable)?
            .is_dir()
        {
            return Ok(Vec::new());
        }
        let mut read_dir = tokio::fs::read_dir(location.as_str())
            .await
            .map_err(unresolvable)?;
        let mut entries = Vec::new();
        while let Some(entry) = read_dir.next_entry().await.map_err(unresolvable)? {
            let file_type = entry.file_type().await.map_err(unresolvable)?;
            let is_expandable = if file_type.is_symlink() {
                match tokio::fs::canonicalize(entry.path()).await {
                    Ok(target) => {
                        target.starts_with(&self.canonical_path)
                            && tokio::fs::metadata(&target)
                                .await
                                .is_ok_and(|metadata| metadata.is_dir())
                    }
                    Err(_) => false,
                }
            } else {
                file_type.is_dir()
            };
            entries.push(DirEntry {
                is_expandable,
                name: entry.file_name().to_string_lossy().into_owned(),
                parent_path: path.clone(),
            });
        }
        Ok(entries)
    }
}

impl WebuiClient {
    /// Lists the entries of the directory at a path in the logs input.
    ///
    /// # Returns
    ///
    /// The directory's entries on success, in no particular order, or an empty list if `path` isn't
    /// a directory.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`Self::check_logs_input_access`]'s return values on failure.
    /// * Forwards [`LogsInputMountPoint::resolve`]'s return values on failure.
    /// * Forwards [`LogsInputMountPoint::list`]'s return values on failure.
    pub async fn list_logs_input_entries(
        &self,
        path: &LogsInputPath,
    ) -> Result<Vec<DirEntry>, ClientError> {
        self.check_logs_input_access("logs-input listing")?;
        LogsInputMountPoint::resolve(NonEmptyString::from_static_str(LOGS_INPUT_MOUNT_POINT))
            .await?
            .list(path)
            .await
    }

    /// Checks that the package's configuration lets the API server access the logs input.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`ClientError::InvalidInput`] if:
    ///   * the package's storage engine isn't clp-s.
    ///   * the package's logs input isn't on the filesystem.
    pub(super) fn check_logs_input_access(&self, operation: &str) -> Result<(), ClientError> {
        if self.config.package.storage_engine != StorageEngine::ClpS {
            return Err(ClientError::InvalidInput(format!(
                "{operation} requires the clp-s storage engine"
            )));
        }
        match self.config.logs_input {
            LogsInput::Fs { .. } => Ok(()),
            LogsInput::S3 { .. } => Err(ClientError::InvalidInput(format!(
                "{operation} requires filesystem logs input"
            ))),
        }
    }
}

#[cfg(test)]
mod tests {
    use std::os::unix::fs::symlink;

    use clp_rust_utils::clp_config::AwsAuthentication;
    use clp_rust_utils::clp_config::package::config::Config;
    use clp_rust_utils::clp_config::package::config::LogsInput;
    use clp_rust_utils::clp_config::package::config::Package;
    use clp_rust_utils::clp_config::package::config::S3Ingestion;
    use clp_rust_utils::clp_config::package::config::StorageEngine;
    use non_empty_string::NonEmptyString;
    use tempfile::TempDir;

    use super::DirEntry;
    use super::LogsInputMountPoint;
    use super::LogsInputPath;
    use super::ParseLogsInputPathError;
    use crate::error::ClientError;
    use crate::webui_client::WebuiClient;

    /// A logs-input mount point `logs` in a temporary directory, next to a directory `logs-evil`
    /// and a file `private.log` that are outside it, and a symlink `logs-link` to the mount point.
    struct Fixture {
        _base: TempDir,
        mount_point: NonEmptyString,
        linked_mount_point: NonEmptyString,
    }

    impl Fixture {
        /// Factory function.
        ///
        /// Creates the mount point with the following entries:
        ///
        /// * `app.log`
        /// * `dir/inner.log`
        /// * `inside-link`, a relative symlink to `dir`.
        /// * `inside-file-link`, a relative symlink to `app.log`.
        /// * `outside-link`, a symlink to `private.log`.
        /// * `sibling-link`, a symlink to `logs-evil`.
        /// * `dangling-link`, a symlink to the nonexistent `missing.log`.
        /// * `loop-link`, a symlink to itself.
        ///
        /// # Returns
        ///
        /// The newly created fixture.
        ///
        /// # Panics
        ///
        /// Panics if any file, directory, or symlink can't be created.
        fn new() -> Self {
            let base = tempfile::tempdir().expect("failed to create the base directory");
            let mount_point = base.path().join("logs");
            std::fs::create_dir_all(mount_point.join("dir")).expect("failed to create `dir`");
            std::fs::write(mount_point.join("app.log"), b"").expect("failed to create `app.log`");
            std::fs::write(mount_point.join("dir/inner.log"), b"")
                .expect("failed to create `inner.log`");
            std::fs::create_dir(base.path().join("logs-evil"))
                .expect("failed to create `logs-evil`");
            std::fs::write(base.path().join("logs-evil/x.log"), b"")
                .expect("failed to create `x.log`");
            std::fs::write(base.path().join("private.log"), b"")
                .expect("failed to create `private.log`");
            symlink("dir", mount_point.join("inside-link"))
                .expect("failed to create `inside-link`");
            symlink("app.log", mount_point.join("inside-file-link"))
                .expect("failed to create `inside-file-link`");
            symlink(
                base.path().join("private.log"),
                mount_point.join("outside-link"),
            )
            .expect("failed to create `outside-link`");
            symlink(
                base.path().join("logs-evil"),
                mount_point.join("sibling-link"),
            )
            .expect("failed to create `sibling-link`");
            symlink("missing.log", mount_point.join("dangling-link"))
                .expect("failed to create `dangling-link`");
            symlink("loop-link", mount_point.join("loop-link"))
                .expect("failed to create `loop-link`");
            symlink("logs", base.path().join("logs-link")).expect("failed to create `logs-link`");
            Self {
                mount_point: Self::non_empty_path(&mount_point),
                linked_mount_point: Self::non_empty_path(&base.path().join("logs-link")),
                _base: base,
            }
        }

        /// # Returns
        ///
        /// The location of `suffix` under the mount point.
        fn under_mount_point(&self, suffix: &str) -> NonEmptyString {
            let mut path = self.mount_point.clone();
            path.push_str(suffix);
            path
        }

        /// # Returns
        ///
        /// `path` as a [`NonEmptyString`].
        ///
        /// # Panics
        ///
        /// Panics if `path` isn't UTF-8 or is empty.
        fn non_empty_path(path: &std::path::Path) -> NonEmptyString {
            NonEmptyString::new(
                path.to_str()
                    .expect("the temporary directory's path should be UTF-8")
                    .to_owned(),
            )
            .expect("the path should be non-empty")
        }
    }

    /// # Returns
    ///
    /// The logs-input path parsed from `value`.
    ///
    /// # Panics
    ///
    /// Panics if `value` isn't a valid logs-input path.
    fn path(value: &str) -> LogsInputPath {
        LogsInputPath::try_from(value.to_owned()).expect("the test path should be valid")
    }

    #[test]
    fn logs_input_paths_are_lexically_normalized() -> anyhow::Result<()> {
        for (value, expected) in [
            ("/", "/"),
            ("//", "/"),
            ("/./", "/"),
            ("/app.log", "/app.log"),
            ("/dir/", "/dir"),
            ("//dir//inner.log//", "/dir/inner.log"),
            ("/./dir/./inner.log/.", "/dir/inner.log"),
            ("/a..b/..c/.d", "/a..b/..c/.d"),
        ] {
            assert_eq!(
                String::from(LogsInputPath::try_from(value.to_owned())?),
                expected,
                "value: {value}"
            );
        }
        Ok(())
    }

    #[test]
    fn logs_input_paths_must_start_with_a_slash() {
        for value in ["", "app.log", "./app.log", "dir/inner.log/"] {
            assert_eq!(
                LogsInputPath::try_from(value.to_owned()),
                Err(ParseLogsInputPathError::NotAbsolute(value.to_owned()))
            );
        }
    }

    #[test]
    fn logs_input_paths_must_not_have_parent_directory_components() {
        for value in [
            "/..",
            "/../logs-evil",
            "/dir/../app.log",
            "/dir/..",
            "//..//",
        ] {
            assert_eq!(
                LogsInputPath::try_from(value.to_owned()),
                Err(ParseLogsInputPathError::ParentDirectoryComponent(
                    value.to_owned()
                ))
            );
        }
    }

    #[test]
    fn logs_input_paths_must_not_contain_nul_characters() {
        assert_eq!(
            LogsInputPath::try_from("/app\0.log".to_owned()),
            Err(ParseLogsInputPathError::NulCharacter(
                "/app\0.log".to_owned()
            ))
        );
    }

    #[tokio::test]
    async fn resolving_a_missing_mount_point_is_unavailable() {
        let fixture = Fixture::new();
        let mut mount_point = fixture.mount_point.clone();
        mount_point.push_str("-missing");
        let result = LogsInputMountPoint::resolve(mount_point).await;
        assert!(
            matches!(
                &result,
                Err(ClientError::LogsInputMountPointUnavailable(error))
                    if error.kind() == std::io::ErrorKind::NotFound
            ),
            "unexpected result: {result:?}"
        );
    }

    #[tokio::test]
    async fn paths_are_located_at_their_lexical_location_under_the_mount_point()
    -> anyhow::Result<()> {
        let fixture = Fixture::new();
        let mount_point = LogsInputMountPoint::resolve(fixture.mount_point.clone()).await?;
        for (value, suffix) in [
            ("/", ""),
            ("/dir/inner.log", "/dir/inner.log"),
            ("/inside-link", "/inside-link"),
            ("/inside-link/inner.log", "/inside-link/inner.log"),
        ] {
            assert_eq!(
                mount_point.locate(&path(value)).await?,
                fixture.under_mount_point(suffix),
                "value: {value}"
            );
        }
        Ok(())
    }

    #[tokio::test]
    async fn paths_under_a_symlinked_mount_point_are_located_through_the_symlink()
    -> anyhow::Result<()> {
        let fixture = Fixture::new();
        let linked_mount_point = &fixture.linked_mount_point;
        let mount_point = LogsInputMountPoint::resolve(linked_mount_point.clone()).await?;
        assert_eq!(
            mount_point.locate(&path("/dir/inner.log")).await?,
            NonEmptyString::new(format!("{linked_mount_point}/dir/inner.log"))
                .expect("the path should be non-empty")
        );
        assert_eq!(mount_point.locate(&path("/")).await?, *linked_mount_point);
        let result = mount_point.locate(&path("/sibling-link")).await;
        assert!(
            matches!(
                &result,
                Err(ClientError::LogsInputPathOutsideMountPoint(rejected))
                    if *rejected == path("/sibling-link")
            ),
            "unexpected result: {result:?}"
        );
        Ok(())
    }

    #[tokio::test]
    async fn paths_resolving_outside_the_mount_point_are_rejected() -> anyhow::Result<()> {
        let fixture = Fixture::new();
        let mount_point = LogsInputMountPoint::resolve(fixture.mount_point.clone()).await?;
        for value in ["/outside-link", "/sibling-link", "/sibling-link/x.log"] {
            let result = mount_point.locate(&path(value)).await;
            assert!(
                matches!(
                    &result,
                    Err(ClientError::LogsInputPathOutsideMountPoint(rejected))
                        if *rejected == path(value)
                ),
                "unexpected result for {value}: {result:?}"
            );
        }
        Ok(())
    }

    #[tokio::test]
    async fn missing_paths_are_not_found() -> anyhow::Result<()> {
        let fixture = Fixture::new();
        let mount_point = LogsInputMountPoint::resolve(fixture.mount_point.clone()).await?;
        let too_long_name = format!("/{}", "a".repeat(300));
        for value in [
            "/missing.log",
            "/app.log/inner.log",
            "/dangling-link",
            too_long_name.as_str(),
        ] {
            let result = mount_point.locate(&path(value)).await;
            assert!(
                matches!(
                    &result,
                    Err(ClientError::LogsInputPathNotFound(missing)) if *missing == path(value)
                ),
                "unexpected result for {value}: {result:?}"
            );
        }
        Ok(())
    }

    #[tokio::test]
    async fn unresolvable_paths_are_unresolvable() -> anyhow::Result<()> {
        let fixture = Fixture::new();
        let mount_point = LogsInputMountPoint::resolve(fixture.mount_point.clone()).await?;
        let result = mount_point.locate(&path("/loop-link")).await;
        assert!(
            matches!(
                &result,
                Err(ClientError::LogsInputPathUnresolvable { path: unresolvable, source })
                    if *unresolvable == path("/loop-link")
                        && format!("{:?}", source.kind()) == "FilesystemLoop"
            ),
            "unexpected result: {result:?}"
        );
        Ok(())
    }

    #[tokio::test]
    async fn directories_list_their_entries_under_the_requested_path() -> anyhow::Result<()> {
        let fixture = Fixture::new();

        let expected: Vec<DirEntry> = [
            ("app.log", false),
            ("dangling-link", false),
            ("dir", true),
            ("inside-file-link", false),
            ("inside-link", true),
            ("loop-link", false),
            ("outside-link", false),
            ("sibling-link", false),
        ]
        .into_iter()
        .map(|(name, is_expandable)| DirEntry {
            is_expandable,
            name: name.to_owned(),
            parent_path: path("/"),
        })
        .collect();
        for mount_point_path in [&fixture.mount_point, &fixture.linked_mount_point] {
            let mount_point = LogsInputMountPoint::resolve(mount_point_path.clone()).await?;
            let mut entries = mount_point.list(&path("/")).await?;
            entries.sort_by(|lhs, rhs| lhs.name.cmp(&rhs.name));
            assert_eq!(entries, expected, "mount point: {mount_point_path}");
        }

        let mount_point = LogsInputMountPoint::resolve(fixture.mount_point.clone()).await?;
        for value in ["/dir", "/inside-link"] {
            assert_eq!(
                mount_point.list(&path(value)).await?,
                vec![DirEntry {
                    is_expandable: false,
                    name: "inner.log".to_owned(),
                    parent_path: path(value),
                }],
                "value: {value}"
            );
        }
        Ok(())
    }

    #[tokio::test]
    async fn files_list_no_entries() -> anyhow::Result<()> {
        let fixture = Fixture::new();
        let mount_point = LogsInputMountPoint::resolve(fixture.mount_point.clone()).await?;
        assert_eq!(mount_point.list(&path("/app.log")).await?, vec![]);
        Ok(())
    }

    #[tokio::test]
    async fn listing_a_path_outside_the_mount_point_is_rejected() -> anyhow::Result<()> {
        let fixture = Fixture::new();
        let mount_point = LogsInputMountPoint::resolve(fixture.mount_point.clone()).await?;
        let result = mount_point.list(&path("/sibling-link")).await;
        assert!(
            matches!(
                &result,
                Err(ClientError::LogsInputPathOutsideMountPoint(rejected))
                    if *rejected == path("/sibling-link")
            ),
            "unexpected result: {result:?}"
        );
        Ok(())
    }

    #[tokio::test]
    async fn logs_input_listing_requires_the_clp_s_storage_engine() {
        let client = WebuiClient::new_lazy(Config {
            package: Package {
                storage_engine: StorageEngine::Clp,
            },
            ..Config::default()
        })
        .await;
        let result = client.list_logs_input_entries(&path("/")).await;
        assert_eq!(
            result
                .expect_err("the listing should be rejected")
                .to_string(),
            "Invalid input: logs-input listing requires the clp-s storage engine"
        );
    }

    #[tokio::test]
    async fn logs_input_listing_requires_filesystem_logs_input() {
        let logs_input = LogsInput::S3 {
            config: S3Ingestion {
                aws_authentication: AwsAuthentication::Default,
            },
        };
        let client = WebuiClient::new_lazy(Config {
            package: Package {
                storage_engine: StorageEngine::ClpS,
            },
            logs_input,
            ..Config::default()
        })
        .await;
        let result = client.list_logs_input_entries(&path("/")).await;
        assert_eq!(
            result
                .expect_err("the listing should be rejected")
                .to_string(),
            "Invalid input: logs-input listing requires filesystem logs input"
        );
    }
}
