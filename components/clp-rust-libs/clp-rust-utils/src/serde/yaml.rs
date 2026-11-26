use serde::de::DeserializeOwned;

use crate::Error;

/// Reads a YAML file from a specified path and deserializes its contents into the requested type
/// `T`.
///
/// # Type Parameters
///
/// * `T` - The type to deserialize the file's contents into.
/// * `P` - The type of the path to the YAML file.
///
/// # Returns
///
/// The deserialized value of type `T` on success.
///
/// # Errors
///
/// Returns an [`Error`] if:
///
/// * Forwards [`std::fs::File::open`]'s return values on failure.
/// * Forwards [`yaml_serde::from_reader`]'s return values on failure.
pub fn from_path<T: DeserializeOwned, P: AsRef<std::path::Path>>(path: P) -> Result<T, Error> {
    let file = std::fs::File::open(path)?;
    Ok(yaml_serde::from_reader(file)?)
}
