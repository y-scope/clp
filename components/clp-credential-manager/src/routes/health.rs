/// Responds with message affirming the service is running.
///
/// # Returns
///
/// A static string stating that the credential manager is running.
pub async fn health() -> &'static str {
    "clp-credential-manager is running"
}
