use std::{sync::Arc, time::Duration};

use jsonwebtoken::{DecodingKey, EncodingKey};
use secrecy::ExposeSecret;
use sqlx::{MySqlPool, mysql::MySqlPoolOptions};

use crate::{
    config::{AppConfig, JwtConfig},
    error::{ServiceError, ServiceResult},
    models::{CreateCredentialRequest, CredentialMetadata, CredentialMetadataRow},
};

pub type SharedService = Arc<CredentialManagerService>;

pub struct CredentialManagerService {
    db_pool: MySqlPool,
    jwt: JwtManager,
}

impl CredentialManagerService {
    pub async fn new(config: &AppConfig) -> ServiceResult<Self> {
        let db_config = &config.database;
        let options = sqlx::mysql::MySqlConnectOptions::new()
            .host(&db_config.host)
            .port(db_config.port)
            .database(&db_config.name)
            .username(&db_config.user)
            .password(db_config.password.expose_secret());

        let pool = MySqlPoolOptions::new()
            .max_connections(db_config.max_connections)
            .connect_with(options)
            .await?;

        let jwt = JwtManager::new(&config.jwt)?;

        Ok(Self { db_pool: pool, jwt })
    }

    pub fn clone_shared(self) -> SharedService {
        Arc::new(self)
    }

    pub fn db_pool(&self) -> &MySqlPool {
        &self.db_pool
    }

    pub fn jwt(&self) -> &JwtManager {
        &self.jwt
    }

    pub async fn list_credentials(&self) -> ServiceResult<Vec<CredentialMetadata>> {
        let rows = sqlx::query_as::<_, CredentialMetadataRow>(
            "SELECT id, name, credential_type, description, default_session_duration_seconds, \
             transient, created_at, updated_at, last_used_at FROM clp_aws_credentials ORDER BY name",
        )
        .fetch_all(self.db_pool())
        .await?;

        rows.into_iter().map(CredentialMetadata::try_from).collect()
    }

    pub async fn get_credential(&self, id: i64) -> ServiceResult<CredentialMetadata> {
        let row = sqlx::query_as::<_, CredentialMetadataRow>(
            "SELECT id, name, credential_type, description, default_session_duration_seconds, \
             transient, created_at, updated_at, last_used_at FROM clp_aws_credentials WHERE id = ?",
        )
        .bind(id)
        .fetch_optional(self.db_pool())
        .await?;

        match row {
            Some(row) => CredentialMetadata::try_from(row),
            None => Err(ServiceError::NotFound(format!(
                "credential with id `{id}` was not found"
            ))),
        }
    }

    pub async fn delete_credential(&self, id: i64) -> ServiceResult<()> {
        let result = sqlx::query("DELETE FROM clp_aws_credentials WHERE id = ?")
            .bind(id)
            .execute(self.db_pool())
            .await?;

        if result.rows_affected() == 0 {
            return Err(ServiceError::NotFound(format!(
                "credential with id `{id}` was not found"
            )));
        }

        Ok(())
    }

    pub async fn create_credential(
        &self,
        request: CreateCredentialRequest,
    ) -> ServiceResult<CredentialMetadata> {
        request.validate()?;

        let name = request.name.clone();
        let credential_type = request.credential_type;
        let access_key_id = request.access_key_id.clone();
        let secret_access_key = request
            .secret_access_key
            .as_ref()
            .map(|secret| secret.expose_secret().to_owned());
        let role_arn = request.role_arn.clone();
        let external_id = request.external_id.clone();
        let description = request.description.clone();
        let default_duration = request.default_session_duration_seconds.unwrap_or(3600);
        let transient = request.transient.unwrap_or(false);
        let created_by = request.created_by.clone();

        let result = sqlx::query(
            "INSERT INTO clp_aws_credentials (name, credential_type, access_key_id, \
             secret_access_key, role_arn, external_id, description, \
             default_session_duration_seconds, transient, created_by) VALUES (?, ?, ?, ?, ?, ?, \
             ?, ?, ?, ?)",
        )
        .bind(&name)
        .bind(credential_type.as_str())
        .bind(access_key_id.as_deref())
        .bind(secret_access_key.as_deref())
        .bind(role_arn.as_deref())
        .bind(external_id.as_deref())
        .bind(description.as_deref())
        .bind(default_duration)
        .bind(transient)
        .bind(created_by.as_deref())
        .execute(self.db_pool())
        .await
        .map_err(|err| {
            let is_duplicate = err
                .as_database_error()
                .and_then(|db_err| db_err.try_downcast_ref::<sqlx::mysql::MySqlDatabaseError>())
                .is_some_and(|mysql_err| mysql_err.number() == 1062);

            if is_duplicate {
                return ServiceError::Conflict(format!(
                    "credential with name `{}` already exists",
                    name
                ));
            }

            ServiceError::from(err)
        })?;

        let inserted_id = result.last_insert_id() as i64;
        self.get_credential(inserted_id).await
    }
}

pub struct JwtManager {
    encoding_key: EncodingKey,
    decoding_key: DecodingKey,
    default_ttl: Duration,
}

impl JwtManager {
    fn new(config: &JwtConfig) -> ServiceResult<Self> {
        let secret = config.secret.expose_secret();
        if secret.is_empty() {
            return Err(ServiceError::Config(
                "jwt secret must not be empty".to_owned(),
            ));
        }

        let default_ttl = Duration::from_secs(config.token_ttl_seconds);

        Ok(Self {
            encoding_key: EncodingKey::from_secret(secret.as_bytes()),
            decoding_key: DecodingKey::from_secret(secret.as_bytes()),
            default_ttl,
        })
    }

    pub fn encoding_key(&self) -> &EncodingKey {
        &self.encoding_key
    }

    pub fn decoding_key(&self) -> &DecodingKey {
        &self.decoding_key
    }

    pub fn default_ttl(&self) -> Duration {
        self.default_ttl
    }
}
