//! Filesystem path validation.

use std::io;
use std::path::Path;
use std::path::PathBuf;

/// Resolves an absolute input path within a canonical input root.
///
/// # Returns
///
/// The canonical path on success.
///
/// # Errors
///
/// Returns an error if:
///
/// * [`io::Error`] with kind [`io::ErrorKind::InvalidInput`] if `path` is relative, the resolved
///   path is outside `root`, is not UTF-8, or contains a carriage return or line feed.
/// * Forwards [`Path::canonicalize`]'s return values on failure.
pub fn resolve_input_path(root: &Path, path: &Path) -> io::Result<PathBuf> {
    if !path.is_absolute() {
        return Err(io::Error::new(
            io::ErrorKind::InvalidInput,
            "input path must be absolute",
        ));
    }
    let resolved = path.canonicalize().map_err(|error| {
        io::Error::new(
            error.kind(),
            format!("failed to resolve input {}: {error}", path.display()),
        )
    })?;
    if !resolved.starts_with(root) {
        return Err(io::Error::new(
            io::ErrorKind::InvalidInput,
            format!(
                "input path {} is outside {}",
                path.display(),
                root.display()
            ),
        ));
    }
    if resolved
        .to_str()
        .is_none_or(|value| value.contains(['\n', '\r']))
    {
        return Err(io::Error::new(
            io::ErrorKind::InvalidInput,
            "input path must be UTF-8 without line breaks",
        ));
    }
    Ok(resolved)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn input_paths_resolve_within_root_and_reject_invalid_paths() {
        let directory =
            std::env::temp_dir().join(format!("clp-input-paths-{}", uuid::Uuid::new_v4()));
        let root = directory.join("root");
        std::fs::create_dir_all(&root).expect("test input root should be created");
        let root = root.canonicalize().expect("test input root should resolve");
        let file = root.join("input.log");
        std::fs::write(&file, []).expect("test file should be created");
        assert_eq!(
            resolve_input_path(&root, &file).expect("file should resolve"),
            file
        );
        assert_eq!(
            resolve_input_path(&root, &root).expect("directory should resolve"),
            root
        );
        assert_eq!(
            resolve_input_path(&root, Path::new("input.log"))
                .expect_err("relative input path should be rejected")
                .kind(),
            io::ErrorKind::InvalidInput,
        );
        for invalid in [directory.clone(), root.join("missing")] {
            assert!(
                resolve_input_path(&root, &invalid).is_err(),
                "{}",
                invalid.display()
            );
        }
        for name in ["line\nbreak", "line\rbreak"] {
            let path = root.join(name);
            std::fs::write(&path, []).expect("line-break test file should be created");
            assert!(resolve_input_path(&root, &path).is_err());
        }
        #[cfg(unix)]
        {
            use std::os::unix::ffi::OsStringExt;

            let internal = root.join("internal");
            std::os::unix::fs::symlink(&file, &internal)
                .expect("internal symlink should be created");
            assert_eq!(
                resolve_input_path(&root, &internal).expect("internal symlink should resolve"),
                file
            );
            let escaping = root.join("escaping");
            std::os::unix::fs::symlink(&directory, &escaping)
                .expect("escaping symlink should be created");
            assert!(resolve_input_path(&root, &escaping).is_err());
            let invalid_utf8 = root.join(std::ffi::OsString::from_vec(vec![0xff]));
            std::fs::write(&invalid_utf8, []).expect("non-UTF-8 test file should be created");
            assert!(resolve_input_path(&root, &invalid_utf8).is_err());
        }
        std::fs::remove_dir_all(directory).expect("test directory should be removed");
    }
}
