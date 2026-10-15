use aws_config::profile::ProfileFileCredentialsProvider;
use aws_config::provider_config::ProviderConfig;
use aws_sdk_s3::config::Region;

pub const AWS_DEFAULT_REGION: &str = "us-east-1";

/// Uses a named shared-config profile even when ambient AWS access-key variables are set.
///
/// # Returns
///
/// A credential provider for `profile`, configured for `region`.
#[must_use]
pub fn named_profile_credentials_provider(
    profile: &str,
    region: &str,
) -> ProfileFileCredentialsProvider {
    ProfileFileCredentialsProvider::builder()
        .configure(&ProviderConfig::default().with_region(Some(Region::new(region.to_owned()))))
        .profile_name(profile)
        .build()
}

#[cfg(test)]
mod tests {
    use std::process::Command;
    use std::time::SystemTime;
    use std::time::UNIX_EPOCH;

    use aws_sdk_s3::config::ProvideCredentials;

    use super::named_profile_credentials_provider;

    #[test]
    fn named_profile_wins_over_environment_credentials() {
        let unique = SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .expect("system time precedes the Unix epoch")
            .as_nanos();
        let directory =
            std::env::temp_dir().join(format!("clp-profile-test-{}-{unique}", std::process::id()));
        std::fs::create_dir(&directory).expect("failed to create profile test directory");
        let credentials_file = directory.join("credentials");
        let config_file = directory.join("config");
        std::fs::write(
            &credentials_file,
            "[explicit]\naws_access_key_id = profile-access\naws_secret_access_key = \
             profile-secret\n",
        )
        .expect("failed to write profile test credentials");
        std::fs::write(&config_file, "").expect("failed to write profile test config");

        let output =
            Command::new(std::env::current_exe().expect("failed to locate test executable"))
                .args(["--exact", "aws::tests::named_profile_child", "--nocapture"])
                .env("CLP_PROFILE_TEST_CHILD", "1")
                .env("AWS_SHARED_CREDENTIALS_FILE", &credentials_file)
                .env("AWS_CONFIG_FILE", &config_file)
                .env("AWS_ACCESS_KEY_ID", "environment-access")
                .env("AWS_SECRET_ACCESS_KEY", "environment-secret")
                .env("AWS_EC2_METADATA_DISABLED", "true")
                .output()
                .expect("failed to run profile test child");
        std::fs::remove_dir_all(directory).expect("failed to remove profile test directory");
        assert!(
            output.status.success(),
            "profile test child failed: {}",
            String::from_utf8_lossy(&output.stdout)
        );
    }

    #[test]
    fn named_profile_child() {
        if std::env::var_os("CLP_PROFILE_TEST_CHILD").is_none() {
            return;
        }
        let runtime = tokio::runtime::Runtime::new().expect("failed to create Tokio runtime");
        let credentials = runtime
            .block_on(
                named_profile_credentials_provider("explicit", "us-east-1").provide_credentials(),
            )
            .expect("failed to read named profile credentials");
        assert_eq!(credentials.access_key_id(), "profile-access");
        assert_eq!(credentials.secret_access_key(), "profile-secret");
    }
}
