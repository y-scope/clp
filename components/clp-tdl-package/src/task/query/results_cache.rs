//! Parsing and validation of query results-cache connection options.

use mongodb::options::Acknowledgment;
use mongodb::options::ClientOptions;

/// Parses a results-cache URI and requires a database and acknowledged writes.
///
/// # Returns
///
/// Validated client options on success.
///
/// # Errors
///
/// Returns an error if:
///
/// * [`anyhow::Error`] if the URI has no database or requests unacknowledged writes.
/// * Forwards [`ClientOptions::parse`]'s return values on failure.
pub(super) async fn parse_results_cache_uri(uri: &str) -> anyhow::Result<ClientOptions> {
    let options = ClientOptions::parse(uri).await?;
    anyhow::ensure!(
        options.default_database.is_some(),
        "results-cache URI must name a database"
    );
    anyhow::ensure!(
        !options
            .write_concern
            .as_ref()
            .is_some_and(|concern| concern.w == Some(Acknowledgment::Nodes(0))),
        "results-cache output requires acknowledged writes"
    );
    Ok(options)
}
