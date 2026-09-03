//! Helpers shared by the tasks that invoke CLP's core binaries.

use std::path::Path;
use std::path::PathBuf;

use anyhow::Context;
use aws_config::BehaviorVersion;
use aws_sdk_s3::config::ProvideCredentials;
use clp_rust_utils::clp_config::AwsAuthentication;

/// Resolves the path of a CLP binary under `clp_home`, joining `bin/{binary}`.
///
/// # Returns
///
/// The path to the named binary under the CLP installation.
pub(super) fn clp_binary_path(clp_home: &Path, binary: &str) -> PathBuf {
    clp_home.join("bin").join(binary)
}

/// Resolves the AWS credential env vars clp-s needs to access the S3 objects.
///
/// # Returns
///
/// The env-var name-value pairs with the following environment variables set:
///
/// * `AWS_ACCESS_KEY_ID`
/// * `AWS_SECRET_ACCESS_KEY`
/// * `AWS_SESSION_TOKEN` (if any)
///
/// # Errors
///
/// Returns an error if:
///
/// * The default AWS SDK credential provider chain has no provider.
/// * Forwards [`ProvideCredentials::provide_credentials`]'s return values on failure.
pub(super) fn s3_credential_env(
    runtime: &tokio::runtime::Handle,
    region: &str,
    auth: &AwsAuthentication,
) -> anyhow::Result<Vec<(&'static str, String)>> {
    /// The env var holding the AWS access key ID.
    const AWS_ACCESS_KEY_ID_ENV_VAR: &str = "AWS_ACCESS_KEY_ID";

    /// The env var holding the AWS secret access key.
    const AWS_SECRET_ACCESS_KEY_ENV_VAR: &str = "AWS_SECRET_ACCESS_KEY";

    /// The env var holding the AWS session token.
    const AWS_SESSION_TOKEN_ENV_VAR: &str = "AWS_SESSION_TOKEN";

    let (access_key_id, secret_access_key, session_token) = match auth {
        AwsAuthentication::Credentials { credentials } => (
            credentials.access_key_id.clone(),
            credentials.secret_access_key.clone(),
            credentials.session_token.clone(),
        ),
        AwsAuthentication::Default => {
            let sdk_config = runtime.block_on(
                aws_config::defaults(BehaviorVersion::latest())
                    .region(aws_sdk_s3::config::Region::new(region.to_string()))
                    .load(),
            );
            let provider = sdk_config
                .credentials_provider()
                .context("default AWS SDK credential provider is unavailable")?;
            let credentials = runtime
                .block_on(provider.provide_credentials())
                .context("failed to resolve credentials from the default AWS SDK provider chain")?;
            (
                credentials.access_key_id().to_string(),
                credentials.secret_access_key().to_string(),
                credentials
                    .session_token()
                    .map(std::string::ToString::to_string),
            )
        }
    };

    let mut env = vec![
        (AWS_ACCESS_KEY_ID_ENV_VAR, access_key_id),
        (AWS_SECRET_ACCESS_KEY_ENV_VAR, secret_access_key),
    ];
    if let Some(session_token) = session_token {
        env.push((AWS_SESSION_TOKEN_ENV_VAR, session_token));
    }
    Ok(env)
}

#[cfg(test)]
mod tests {
    use clp_rust_utils::clp_config::AwsAuthentication;
    use clp_rust_utils::clp_config::AwsCredentials;

    use super::s3_credential_env;

    #[test]
    fn s3_credential_env_credentials() {
        let runtime = tokio::runtime::Runtime::new().expect("failed to create Tokio runtime");
        let auth = AwsAuthentication::Credentials {
            credentials: AwsCredentials {
                access_key_id: "the-access-key".to_string(),
                secret_access_key: "the-secret-key".to_string(),
                session_token: Some("the-session-token".to_string()),
            },
        };

        assert_eq!(
            s3_credential_env(runtime.handle(), "us-east-1", &auth)
                .expect("failed to resolve credentials"),
            vec![
                ("AWS_ACCESS_KEY_ID", "the-access-key".to_string()),
                ("AWS_SECRET_ACCESS_KEY", "the-secret-key".to_string()),
                ("AWS_SESSION_TOKEN", "the-session-token".to_string()),
            ]
        );
    }
}
