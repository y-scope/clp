use std::fmt;

use chrono::{DateTime, NaiveDateTime, Utc};
use secrecy::{ExposeSecret, SecretString};
use serde::{Deserialize, Serialize};
use sqlx::FromRow;

use crate::error::{ServiceError, ServiceResult};

#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
#[serde(rename_all = "snake_case")]
pub enum CredentialType {
    IamUser,
    IamRole,
    Temporary,
}

impl CredentialType {
    pub fn as_str(self) -> &'static str {
        match self {
            CredentialType::IamUser => "iam_user",
            CredentialType::IamRole => "iam_role",
            CredentialType::Temporary => "temporary",
        }
    }
}

impl TryFrom<&str> for CredentialType {
    type Error = ServiceError;

    fn try_from(value: &str) -> Result<Self, Self::Error> {
        match value {
            "iam_user" => Ok(CredentialType::IamUser),
            "iam_role" => Ok(CredentialType::IamRole),
            "temporary" => Ok(CredentialType::Temporary),
            other => Err(ServiceError::Config(format!(
                "unsupported credential_type `{other}` stored in database"
            ))),
        }
    }
}

impl fmt::Display for CredentialType {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}", self.as_str())
    }
}

#[derive(Debug, Clone, Deserialize)]
pub struct CreateCredentialRequest {
    pub name: String,
    pub credential_type: CredentialType,
    #[serde(default)]
    pub access_key_id: Option<String>,
    #[serde(default)]
    pub secret_access_key: Option<SecretString>,
    #[serde(default)]
    pub role_arn: Option<String>,
    #[serde(default)]
    pub external_id: Option<String>,
    #[serde(default)]
    pub description: Option<String>,
    #[serde(default)]
    pub default_session_duration_seconds: Option<i32>,
    #[serde(default)]
    pub transient: Option<bool>,
    #[serde(default)]
    pub created_by: Option<String>,
}

impl CreateCredentialRequest {
    pub fn validate(&self) -> ServiceResult<()> {
        if self.name.trim().is_empty() {
            return Err(ServiceError::Validation(
                "credential name must not be empty".to_owned(),
            ));
        }

        match self.credential_type {
            CredentialType::IamUser => {
                ensure_present(self.access_key_id.as_deref(), "access_key_id")?;
                ensure_secret_present(self.secret_access_key.as_ref(), "secret_access_key")?;
            }
            CredentialType::IamRole => {
                ensure_present(self.access_key_id.as_deref(), "access_key_id")?;
                ensure_secret_present(self.secret_access_key.as_ref(), "secret_access_key")?;
                ensure_present(self.role_arn.as_deref(), "role_arn")?;
            }
            CredentialType::Temporary => {
                return Err(ServiceError::Validation(
                    "temporary credentials cannot be created via this endpoint".to_owned(),
                ));
            }
        }

        if let Some(duration) = self.default_session_duration_seconds {
            if duration <= 0 {
                return Err(ServiceError::Validation(
                    "default_session_duration_seconds must be positive".to_owned(),
                ));
            }
        }

        Ok(())
    }
}

fn ensure_present(value: Option<&str>, field: &str) -> ServiceResult<()> {
    match value {
        Some(val) if !val.trim().is_empty() => Ok(()),
        _ => Err(ServiceError::Validation(format!(
            "`{field}` must be provided for credential_type that requires it"
        ))),
    }
}

fn ensure_secret_present(value: Option<&SecretString>, field: &str) -> ServiceResult<()> {
    match value {
        Some(secret) if !secret.expose_secret().trim().is_empty() => Ok(()),
        _ => Err(ServiceError::Validation(format!(
            "`{field}` must be provided for credential_type that requires it"
        ))),
    }
}

#[derive(Debug, Clone, FromRow)]
pub struct CredentialMetadataRow {
    pub id: i64,
    pub name: String,
    pub credential_type: String,
    pub description: Option<String>,
    pub default_session_duration_seconds: i32,
    pub transient: bool,
    pub created_at: NaiveDateTime,
    pub updated_at: NaiveDateTime,
    pub last_used_at: Option<NaiveDateTime>,
}

#[derive(Debug, Clone, Serialize)]
pub struct CredentialMetadata {
    pub id: i64,
    pub name: String,
    pub credential_type: CredentialType,
    pub description: Option<String>,
    pub default_session_duration_seconds: i32,
    pub transient: bool,
    pub created_at: DateTime<Utc>,
    pub updated_at: DateTime<Utc>,
    pub last_used_at: Option<DateTime<Utc>>,
}

impl TryFrom<CredentialMetadataRow> for CredentialMetadata {
    type Error = ServiceError;

    fn try_from(row: CredentialMetadataRow) -> Result<Self, Self::Error> {
        let credential_type = CredentialType::try_from(row.credential_type.as_str())?;
        Ok(Self {
            id: row.id,
            name: row.name,
            credential_type,
            description: row.description,
            default_session_duration_seconds: row.default_session_duration_seconds,
            transient: row.transient,
            created_at: to_utc(row.created_at),
            updated_at: to_utc(row.updated_at),
            last_used_at: row.last_used_at.map(to_utc),
        })
    }
}

fn to_utc(ts: NaiveDateTime) -> DateTime<Utc> {
    DateTime::<Utc>::from_naive_utc_and_offset(ts, Utc)
}
