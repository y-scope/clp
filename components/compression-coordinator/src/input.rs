//! Discovery of regular files selected for compression.

use std::collections::HashSet;
use std::path::Path;

use clp_rust_utils::logs_input::LogsInputMountPoint;
use non_empty_string::NonEmptyString;
use walkdir::WalkDir;

use crate::Error;
use crate::partition::FsEntry;

/// Recursively discovers regular files selected by container paths under the logs-input mount.
///
/// Traversal follows symlinks within the mount. Canonical paths deduplicate overlapping selections
/// and prevent revisiting directories through symlink cycles.
/// Directories are omitted, while zero-byte regular files are retained.
///
/// # Returns
///
/// The files' canonical mount-relative paths and sizes on success, in no particular order. The
/// list is empty if the valid selection contains no files.
///
/// # Errors
///
/// Returns an error if:
///
/// * [`Error::NoFsInputPaths`] if `paths` is empty.
/// * [`Error::UnsupportedFsInput`] if a selected or discovered entry is neither a regular file nor
///   a directory.
/// * [`Error::FsTraversal`] if traversing directory entries fails.
/// * Forwards [`LogsInputMountPoint::canonical_input_path`]'s return values on failure.
/// * Forwards [`walkdir::DirEntry::metadata`]'s return values on failure.
pub fn discover_fs_entries(
    paths: &[NonEmptyString],
    mount_point: &LogsInputMountPoint,
) -> Result<Vec<FsEntry>, Error> {
    if paths.is_empty() {
        return Err(Error::NoFsInputPaths);
    }
    let mut visited = HashSet::new();
    let mut files = Vec::new();
    for path in paths {
        let root_input_path = mount_point.canonical_input_path(Path::new(path.as_str()))?;
        let mut entries = WalkDir::new(path.as_str()).follow_links(true).into_iter();
        while let Some(entry) = entries.next() {
            let entry = match entry {
                Ok(entry) => entry,
                Err(error) => {
                    if let Some(path) = error.path().filter(|_| error.loop_ancestor().is_some()) {
                        mount_point.canonical_input_path(path)?;
                        continue;
                    }
                    return Err(error.into());
                }
            };
            let input_path = if entry.depth() == 0 {
                root_input_path.clone()
            } else {
                mount_point.canonical_input_path(entry.path())?
            };
            if !visited.insert(input_path.as_ref().to_owned()) {
                if entry.file_type().is_dir() {
                    entries.skip_current_dir();
                }
                continue;
            }
            if entry.file_type().is_file() {
                files.push(FsEntry {
                    path: input_path,
                    size: entry.metadata()?.len(),
                });
            } else if !entry.file_type().is_dir() {
                return Err(Error::UnsupportedFsInput(entry.into_path()));
            }
        }
    }
    Ok(files)
}

#[cfg(test)]
mod tests {
    use std::collections::BTreeMap;
    use std::os::unix::fs::symlink;
    use std::os::unix::net::UnixListener;
    use std::path::Path;

    use clp_rust_utils::logs_input::LogsInputMountPoint;
    use non_empty_string::NonEmptyString;

    use super::discover_fs_entries;
    use crate::Error;

    fn path_string(path: &Path) -> NonEmptyString {
        NonEmptyString::new(
            path.to_str()
                .expect("temporary path should be UTF-8")
                .to_owned(),
        )
        .expect("temporary path should not be empty")
    }

    #[test]
    fn recursive_overlapping_selections_include_zero_byte_files_once() -> anyhow::Result<()> {
        let root = tempfile::tempdir()?;
        let mount_point = LogsInputMountPoint::resolve(path_string(root.path()))?;
        std::fs::create_dir(root.path().join("dir"))?;
        std::fs::write(root.path().join("dir/data.log"), b"hello")?;
        std::fs::write(root.path().join("empty.log"), b"")?;
        symlink("dir", root.path().join("alias"))?;
        symlink("..", root.path().join("dir/cycle"))?;
        let files = discover_fs_entries(
            &[
                path_string(root.path()),
                path_string(&root.path().join("alias")),
                path_string(&root.path().join("dir/data.log")),
            ],
            &mount_point,
        )?;
        let num_files = files.len();
        let files: BTreeMap<_, _> = files
            .into_iter()
            .map(|entry| (String::from(entry.path), entry.size))
            .collect();
        assert_eq!(num_files, 2);
        assert_eq!(
            files,
            BTreeMap::from([
                ("/dir/data.log".to_owned(), 5),
                ("/empty.log".to_owned(), 0)
            ])
        );
        Ok(())
    }

    #[test]
    fn missing_escaping_and_special_paths_are_rejected() -> anyhow::Result<()> {
        let root = tempfile::tempdir()?;
        let mount_point = LogsInputMountPoint::resolve(path_string(root.path()))?;
        let outside = tempfile::tempdir()?;
        symlink(outside.path(), root.path().join("escape"))?;
        std::fs::create_dir(root.path().join("nested"))?;
        symlink("missing", root.path().join("nested/broken"))?;
        std::fs::create_dir(root.path().join("nested-escape"))?;
        symlink(outside.path(), root.path().join("nested-escape/link"))?;
        std::fs::create_dir(root.path().join("invalid-cycle"))?;
        symlink(".", root.path().join("invalid-cycle/loop\n"))?;
        let _socket = UnixListener::bind(root.path().join("socket"))?;
        for (selection, invalid_path) in [
            ("missing", "missing"),
            ("escape", "escape"),
            ("socket", "socket"),
            ("nested", "nested/broken"),
            ("nested-escape", "nested-escape/link"),
            ("invalid-cycle", "invalid-cycle/loop\n"),
        ] {
            let path = root.path().join(selection);
            let error = discover_fs_entries(&[path_string(&path)], &mount_point)
                .err()
                .expect("invalid selection should fail");
            let invalid_path = root.path().join(invalid_path);
            assert!(
                error
                    .to_string()
                    .contains(invalid_path.to_str().expect("path should be UTF-8"))
            );
        }
        Ok(())
    }

    #[test]
    fn empty_directories_and_directory_cycles_produce_no_files() -> anyhow::Result<()> {
        let root = tempfile::tempdir()?;
        let mount_point = LogsInputMountPoint::resolve(path_string(root.path()))?;
        std::fs::create_dir(root.path().join("empty"))?;
        symlink(".", root.path().join("cycle"))?;
        let entries = discover_fs_entries(&[path_string(root.path())], &mount_point)?;
        assert_eq!(
            entries
                .into_iter()
                .map(|entry| entry.path)
                .collect::<Vec<_>>(),
            vec![]
        );
        Ok(())
    }

    #[test]
    fn empty_selection_is_rejected() -> anyhow::Result<()> {
        let root = tempfile::tempdir()?;
        let mount_point = LogsInputMountPoint::resolve(path_string(root.path()))?;
        let result = discover_fs_entries(&[], &mount_point);
        assert!(matches!(result, Err(Error::NoFsInputPaths)));
        Ok(())
    }
}
