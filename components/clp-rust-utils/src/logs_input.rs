//! Paths in the logs input, and their resolution under the logs-input mount point.

use std::fmt::Display;
use std::fmt::Formatter;
use std::io;
use std::path::Path;
use std::path::PathBuf;

use non_empty_string::NonEmptyString;
use serde::Deserialize;
use serde::Serialize;

/// A path in the logs input, relative to the logs-input mount point
/// ([`LOGS_INPUT_MOUNT_POINT`](crate::clp_config::package::config::LOGS_INPUT_MOUNT_POINT)).
///
/// A valid logs-input path:
///
/// * Starts with `/`, which denotes the mount point itself.
/// * Has no `..` components.
/// * Has no NUL characters.
/// * Has no line breaks (`\n` or `\r`).
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
        if value.contains(['\n', '\r']) {
            return Err(ParseLogsInputPathError::LineBreak(value));
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

    #[error("invalid logs-input path {0:?}: paths must not contain line breaks")]
    LineBreak(String),
}

/// An entry of a directory in the logs input.
#[derive(Debug, Eq, PartialEq)]
pub struct LogsInputEntry {
    /// Whether the entry is a directory, or a symlink to a directory under the logs-input mount
    /// point.
    pub is_expandable: bool,

    /// The entry's file name.
    pub name: String,
}

/// The logs-input mount point, resolved to its canonical form.
#[derive(Debug)]
pub struct LogsInputMountPoint {
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
    /// * [`LogsInputError::MountPointUnavailable`] if `path` can't be resolved.
    pub fn resolve(path: NonEmptyString) -> Result<Self, LogsInputError> {
        let canonical_path =
            std::fs::canonicalize(path.as_str()).map_err(LogsInputError::MountPointUnavailable)?;
        Ok(Self {
            path,
            canonical_path,
        })
    }

    /// Converts a logs-input path to its container path under the mount point, and checks that it
    /// doesn't resolve to outside the mount point through a symlink.
    ///
    /// Symlinks are only resolved for the check: the returned container path is the path's lexical
    /// location under the mount point.
    ///
    /// # Returns
    ///
    /// The path's container path on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * Forwards [`Self::canonicalize`]'s return values on failure.
    pub fn to_container_path(
        &self,
        path: &LogsInputPath,
    ) -> Result<NonEmptyString, LogsInputError> {
        let mut container_path = self.path.clone();
        if !path.is_mount_point() {
            container_path.push_str(&path.value);
        }
        self.canonicalize(Path::new(container_path.as_str()), Path::new(&path.value))?;
        Ok(container_path)
    }

    /// Checks that a container path doesn't resolve to outside the mount point, and converts it to
    /// the container path to compress.
    ///
    /// # Returns
    ///
    /// On success:
    ///
    /// * `container_path` with its `.` components, repeated `/`s, and trailing `/` removed, if it's
    ///   lexically under the mount point.
    /// * `container_path`'s canonical form otherwise (e.g., if `container_path` is a symlink
    ///   outside the mount point that points to inside it).
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`LogsInputError::RelativeContainerPath`] if `container_path` is relative.
    /// * [`LogsInputError::UnsupportedName`] if the returned container path isn't UTF-8 or contains
    ///   a line break.
    /// * Forwards [`Self::canonicalize`]'s return values on failure.
    pub fn checked_container_path(
        &self,
        container_path: &Path,
    ) -> Result<NonEmptyString, LogsInputError> {
        if container_path.is_relative() {
            return Err(LogsInputError::RelativeContainerPath(
                container_path.to_path_buf(),
            ));
        }
        let container_path: PathBuf = container_path.components().collect();
        let canonical_path = self.canonicalize(&container_path, &container_path)?;
        let checked_path = if container_path.starts_with(self.path.as_str()) {
            container_path
        } else {
            canonical_path
        };
        checked_path
            .to_str()
            .filter(|path| !path.contains(['\n', '\r']))
            .and_then(|path| NonEmptyString::new(path.to_owned()).ok())
            .ok_or(LogsInputError::UnsupportedName(checked_path))
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
    /// * [`LogsInputError::Unresolvable`] if the directory's entries can't be read.
    /// * Forwards [`Self::to_container_path`]'s return values on failure.
    pub fn list(&self, path: &LogsInputPath) -> Result<Vec<LogsInputEntry>, LogsInputError> {
        let container_path = self.to_container_path(path)?;
        let unresolvable = |source| LogsInputError::Unresolvable {
            path: PathBuf::from(&path.value),
            source,
        };
        if !std::fs::metadata(container_path.as_str())
            .map_err(unresolvable)?
            .is_dir()
        {
            return Ok(Vec::new());
        }
        let mut entries = Vec::new();
        for entry in std::fs::read_dir(container_path.as_str()).map_err(unresolvable)? {
            let entry = entry.map_err(unresolvable)?;
            let file_type = entry.file_type().map_err(unresolvable)?;
            let is_expandable = if file_type.is_symlink() {
                let entry_path = entry.path();
                self.canonicalize(&entry_path, &entry_path)
                    .is_ok_and(|target| target.is_dir())
            } else {
                file_type.is_dir()
            };
            entries.push(LogsInputEntry {
                is_expandable,
                name: entry.file_name().to_string_lossy().into_owned(),
            });
        }
        Ok(entries)
    }

    /// Canonicalizes a container path, and checks that it's under the mount point.
    ///
    /// `path` is the path the caller gave for `container_path`, which the returned errors report.
    ///
    /// # Returns
    ///
    /// The container path's canonical form on success.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`LogsInputError::NotFound`] if `container_path` doesn't exist or has a component that is
    ///   too long.
    /// * [`LogsInputError::OutsideMountPoint`] if `container_path` resolves to outside the mount
    ///   point.
    /// * [`LogsInputError::Unresolvable`] if `container_path` can't be resolved for any other
    ///   reason (e.g., a symlink loop or a permission error).
    fn canonicalize(&self, container_path: &Path, path: &Path) -> Result<PathBuf, LogsInputError> {
        let canonical_path =
            std::fs::canonicalize(container_path).map_err(|source| match source.kind() {
                io::ErrorKind::NotFound
                | io::ErrorKind::NotADirectory
                | io::ErrorKind::InvalidFilename => LogsInputError::NotFound(path.to_path_buf()),
                _ => LogsInputError::Unresolvable {
                    path: path.to_path_buf(),
                    source,
                },
            })?;
        if !canonical_path.starts_with(&self.canonical_path) {
            return Err(LogsInputError::OutsideMountPoint(path.to_path_buf()));
        }
        Ok(canonical_path)
    }
}

/// An error returned when resolving a path under the logs-input mount point.
#[derive(Debug, thiserror::Error)]
pub enum LogsInputError {
    #[error("logs-input mount point can't be resolved: {0}")]
    MountPointUnavailable(io::Error),

    #[error("logs-input path not found: {}", .0.display())]
    NotFound(PathBuf),

    #[error(
        "logs-input path resolves to outside the logs-input mount point: {}",
        .0.display()
    )]
    OutsideMountPoint(PathBuf),

    #[error("logs-input container path isn't absolute: {}", .0.display())]
    RelativeContainerPath(PathBuf),

    #[error("logs-input path isn't UTF-8 or contains a line break: {}", .0.display())]
    UnsupportedName(PathBuf),

    #[error("logs-input path can't be resolved: {}: {source}", .path.display())]
    Unresolvable {
        path: PathBuf,
        #[source]
        source: io::Error,
    },
}

#[cfg(test)]
mod tests {
    use std::ffi::OsStr;
    use std::os::unix::ffi::OsStrExt;
    use std::os::unix::fs::symlink;
    use std::path::Path;
    use std::path::PathBuf;

    use non_empty_string::NonEmptyString;
    use tempfile::TempDir;

    use super::LogsInputEntry;
    use super::LogsInputError;
    use super::LogsInputMountPoint;
    use super::LogsInputPath;
    use super::ParseLogsInputPathError;

    /// A logs-input mount point `logs` in a temporary directory, next to a directory `logs-evil`
    /// and a file `private.log` that are outside it, a symlink `logs-link` to the mount point, and
    /// a symlink `outside-to-inside-link` to `logs/app.log`.
    struct Fixture {
        base: TempDir,
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
            symlink("logs/app.log", base.path().join("outside-to-inside-link"))
                .expect("failed to create `outside-to-inside-link`");
            Self {
                mount_point: Self::non_empty_path(&mount_point),
                linked_mount_point: Self::non_empty_path(&base.path().join("logs-link")),
                base,
            }
        }

        /// # Returns
        ///
        /// The container path of `suffix` under the mount point.
        fn under_mount_point(&self, suffix: &str) -> NonEmptyString {
            let mut path = self.mount_point.clone();
            path.push_str(suffix);
            path
        }

        /// # Returns
        ///
        /// The resolved mount point.
        ///
        /// # Panics
        ///
        /// Panics if the mount point can't be resolved.
        fn resolve(&self) -> LogsInputMountPoint {
            LogsInputMountPoint::resolve(self.mount_point.clone())
                .expect("the mount point should be resolvable")
        }

        /// # Returns
        ///
        /// `path` as a [`NonEmptyString`].
        ///
        /// # Panics
        ///
        /// Panics if `path` isn't UTF-8 or is empty.
        fn non_empty_path(path: &Path) -> NonEmptyString {
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

    #[test]
    fn logs_input_paths_must_not_contain_line_breaks() {
        for value in ["/app\n.log", "/app\r.log", "/dir/\r\n/inner.log"] {
            assert_eq!(
                LogsInputPath::try_from(value.to_owned()),
                Err(ParseLogsInputPathError::LineBreak(value.to_owned()))
            );
        }
    }

    #[test]
    fn resolving_a_missing_mount_point_is_unavailable() {
        let fixture = Fixture::new();
        let mut mount_point = fixture.mount_point;
        mount_point.push_str("-missing");
        let result = LogsInputMountPoint::resolve(mount_point);
        assert!(
            matches!(
                &result,
                Err(LogsInputError::MountPointUnavailable(error))
                    if error.kind() == std::io::ErrorKind::NotFound
            ),
            "unexpected result: {result:?}"
        );
    }

    #[test]
    fn paths_convert_to_their_lexical_container_paths() -> anyhow::Result<()> {
        let fixture = Fixture::new();
        let mount_point = fixture.resolve();
        for (value, suffix) in [
            ("/", ""),
            ("/dir/inner.log", "/dir/inner.log"),
            ("/inside-link", "/inside-link"),
            ("/inside-link/inner.log", "/inside-link/inner.log"),
        ] {
            assert_eq!(
                mount_point.to_container_path(&path(value))?,
                fixture.under_mount_point(suffix),
                "value: {value}"
            );
        }
        Ok(())
    }

    #[test]
    fn paths_under_a_symlinked_mount_point_convert_through_the_symlink() -> anyhow::Result<()> {
        let fixture = Fixture::new();
        let linked_mount_point = &fixture.linked_mount_point;
        let mount_point = LogsInputMountPoint::resolve(linked_mount_point.clone())?;
        assert_eq!(
            mount_point.to_container_path(&path("/dir/inner.log"))?,
            NonEmptyString::new(format!("{linked_mount_point}/dir/inner.log"))
                .expect("the path should be non-empty")
        );
        assert_eq!(
            mount_point.to_container_path(&path("/"))?,
            *linked_mount_point
        );
        let result = mount_point.to_container_path(&path("/sibling-link"));
        assert!(
            matches!(
                &result,
                Err(LogsInputError::OutsideMountPoint(rejected))
                    if rejected == Path::new("/sibling-link")
            ),
            "unexpected result: {result:?}"
        );
        Ok(())
    }

    #[test]
    fn paths_resolving_outside_the_mount_point_are_rejected() {
        let fixture = Fixture::new();
        let mount_point = fixture.resolve();
        for value in ["/outside-link", "/sibling-link", "/sibling-link/x.log"] {
            let result = mount_point.to_container_path(&path(value));
            assert!(
                matches!(
                    &result,
                    Err(LogsInputError::OutsideMountPoint(rejected))
                        if rejected == Path::new(value)
                ),
                "unexpected result for {value}: {result:?}"
            );
        }
    }

    #[test]
    fn missing_paths_are_not_found() {
        let fixture = Fixture::new();
        let mount_point = fixture.resolve();
        let too_long_name = format!("/{}", "a".repeat(300));
        for value in [
            "/missing.log",
            "/app.log/inner.log",
            "/dangling-link",
            too_long_name.as_str(),
        ] {
            let result = mount_point.to_container_path(&path(value));
            assert!(
                matches!(
                    &result,
                    Err(LogsInputError::NotFound(missing)) if missing == Path::new(value)
                ),
                "unexpected result for {value}: {result:?}"
            );
        }
    }

    #[test]
    fn unresolvable_paths_are_unresolvable() {
        let fixture = Fixture::new();
        let mount_point = fixture.resolve();
        let result = mount_point.to_container_path(&path("/loop-link"));
        assert!(
            matches!(
                &result,
                Err(LogsInputError::Unresolvable { path: unresolvable, source })
                    if unresolvable == Path::new("/loop-link")
                        && format!("{:?}", source.kind()) == "FilesystemLoop"
            ),
            "unexpected result: {result:?}"
        );
    }

    #[test]
    fn container_paths_under_the_mount_point_are_checked_as_is() -> anyhow::Result<()> {
        let fixture = Fixture::new();
        let mount_point = fixture.resolve();
        let location = fixture.under_mount_point("/dir/inner.log");
        assert_eq!(
            mount_point.checked_container_path(Path::new(location.as_str()))?,
            location
        );
        Ok(())
    }

    #[test]
    fn container_paths_under_the_mount_point_are_checked_normalized() -> anyhow::Result<()> {
        let fixture = Fixture::new();
        let mount_point = fixture.resolve();
        for suffix in ["//dir/./inner.log", "/dir//inner.log/", "/./dir/inner.log"] {
            let location = fixture.under_mount_point(suffix);
            assert_eq!(
                mount_point.checked_container_path(Path::new(location.as_str()))?,
                fixture.under_mount_point("/dir/inner.log"),
                "suffix: {suffix}"
            );
        }
        Ok(())
    }

    #[test]
    fn symlinks_under_the_mount_point_are_checked_as_is() -> anyhow::Result<()> {
        let fixture = Fixture::new();
        let mount_point = fixture.resolve();
        let location = fixture.under_mount_point("/inside-link");
        assert_eq!(
            mount_point.checked_container_path(Path::new(location.as_str()))?,
            location
        );
        Ok(())
    }

    #[test]
    fn symlinks_outside_the_mount_point_are_checked_as_their_targets() -> anyhow::Result<()> {
        let fixture = Fixture::new();
        let mount_point = fixture.resolve();
        let target = std::fs::canonicalize(fixture.under_mount_point("/app.log").as_str())?;
        assert_eq!(
            mount_point
                .checked_container_path(&fixture.base.path().join("outside-to-inside-link"))?,
            Fixture::non_empty_path(&target)
        );
        Ok(())
    }

    #[test]
    fn container_paths_resolving_outside_the_mount_point_are_rejected() {
        let fixture = Fixture::new();
        let mount_point = fixture.resolve();
        let location = PathBuf::from(fixture.under_mount_point("/outside-link").as_str());
        let result = mount_point.checked_container_path(&location);
        assert!(
            matches!(
                &result,
                Err(LogsInputError::OutsideMountPoint(rejected)) if *rejected == location
            ),
            "unexpected result: {result:?}"
        );
    }

    #[test]
    fn relative_container_paths_are_rejected() {
        let fixture = Fixture::new();
        let mount_point = fixture.resolve();
        let result = mount_point.checked_container_path(Path::new("logs/app.log"));
        assert!(
            matches!(
                &result,
                Err(LogsInputError::RelativeContainerPath(rejected))
                    if rejected == Path::new("logs/app.log")
            ),
            "unexpected result: {result:?}"
        );
    }

    #[test]
    fn missing_container_paths_are_not_found() {
        let fixture = Fixture::new();
        let mount_point = fixture.resolve();
        let location = PathBuf::from(fixture.under_mount_point("/missing.log").as_str());
        let result = mount_point.checked_container_path(&location);
        assert!(
            matches!(
                &result,
                Err(LogsInputError::NotFound(missing)) if *missing == location
            ),
            "unexpected result: {result:?}"
        );
    }

    #[test]
    fn container_paths_with_unsupported_names_are_rejected() -> anyhow::Result<()> {
        let fixture = Fixture::new();
        let mount_point = fixture.resolve();
        for name in [OsStr::from_bytes(b"app\xff.log"), OsStr::new("app\n.log")] {
            let location = Path::new(fixture.mount_point.as_str()).join(name);
            std::fs::write(&location, b"")?;
            let result = mount_point.checked_container_path(&location);
            assert!(
                matches!(
                    &result,
                    Err(LogsInputError::UnsupportedName(rejected)) if *rejected == location
                ),
                "unexpected result for {name:?}: {result:?}"
            );
        }
        Ok(())
    }

    #[test]
    fn directories_list_their_entries() -> anyhow::Result<()> {
        let fixture = Fixture::new();

        let expected: Vec<LogsInputEntry> = [
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
        .map(|(name, is_expandable)| LogsInputEntry {
            is_expandable,
            name: name.to_owned(),
        })
        .collect();
        for mount_point_path in [&fixture.mount_point, &fixture.linked_mount_point] {
            let mount_point = LogsInputMountPoint::resolve(mount_point_path.clone())?;
            let mut entries = mount_point.list(&path("/"))?;
            entries.sort_by(|lhs, rhs| lhs.name.cmp(&rhs.name));
            assert_eq!(entries, expected, "mount point: {mount_point_path}");
        }

        let mount_point = fixture.resolve();
        for value in ["/dir", "/inside-link"] {
            assert_eq!(
                mount_point.list(&path(value))?,
                vec![LogsInputEntry {
                    is_expandable: false,
                    name: "inner.log".to_owned(),
                }],
                "value: {value}"
            );
        }
        Ok(())
    }

    #[test]
    fn files_list_no_entries() -> anyhow::Result<()> {
        let fixture = Fixture::new();
        let mount_point = fixture.resolve();
        assert_eq!(mount_point.list(&path("/app.log"))?, vec![]);
        Ok(())
    }

    #[test]
    fn listing_a_path_outside_the_mount_point_is_rejected() {
        let fixture = Fixture::new();
        let mount_point = fixture.resolve();
        let result = mount_point.list(&path("/sibling-link"));
        assert!(
            matches!(
                &result,
                Err(LogsInputError::OutsideMountPoint(rejected))
                    if rejected == Path::new("/sibling-link")
            ),
            "unexpected result: {result:?}"
        );
    }
}
