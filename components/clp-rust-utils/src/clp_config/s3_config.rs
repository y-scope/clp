use secrecy::{ExposeSecret, SecretString};
use serde::{Deserialize, Serialize, Serializer, ser::SerializeStruct};

/// Represents the configuration for connecting to an S3 bucket.
#[derive(Debug, Clone, PartialEq, Eq, Serialize)]
pub struct S3Config {
    pub bucket: String,
    pub region_code: String,
    pub key_prefix: String,
    pub aws_authentication: AwsAuthentication,
}

/// An enum representing AWS authentication methods.
#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
#[serde(tag = "type")]
pub enum AwsAuthentication {
    #[serde(rename = "credentials")]
    Credentials { credentials: AwsCredentials },
}

/// Represents AWS credentials.
#[derive(Debug, Clone, Deserialize)]
pub struct AwsCredentials {
    pub access_key_id: String,
    pub secret_access_key: SecretString,
}

/// Implement [`PartialEq`] for [`AwsCredentials`] to bypass the constraints of [`SecretString`].
impl PartialEq for AwsCredentials {
    fn eq(&self, other: &Self) -> bool {
        self.access_key_id == other.access_key_id
            && self.secret_access_key.expose_secret() == other.secret_access_key.expose_secret()
    }
}

/// Implement [`Eq`] for [`AwsCredentials`] to bypass the constraints of [`SecretString`].
impl Eq for AwsCredentials {}

/// Implement [`Serialize`] for [`AwsCredentials`] to bypass the constraints of [`SecretString`].
impl Serialize for AwsCredentials {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: Serializer, {
        let mut state = serializer.serialize_struct("AwsCredentials", 2)?;
        state.serialize_field("access_key_id", &self.access_key_id)?;
        state.serialize_field("secret_access_key", self.secret_access_key.expose_secret())?;
        state.end()
    }
}

#[cfg(test)]
mod tests {
    use secrecy::SecretString;

    use super::AwsCredentials;

    #[test]
    fn serialize_aws_credentials_to_json() {
        const ACCESS_KEY_ID: &str = "YSCOPE";
        const SECRET_ACCESS_KEY: &str = "IamSecret";

        let creds = AwsCredentials {
            access_key_id: ACCESS_KEY_ID.to_string(),
            secret_access_key: SecretString::new(SECRET_ACCESS_KEY.to_string().into_boxed_str()),
        };

        let value =
            serde_json::to_value(&creds).expect("failed to serialize AwsCredentials to JSON");

        assert_eq!(
            value.get("access_key_id").and_then(|v| v.as_str()),
            Some(ACCESS_KEY_ID)
        );
        assert_eq!(
            value.get("secret_access_key").and_then(|v| v.as_str()),
            Some(SECRET_ACCESS_KEY)
        );
    }
}
