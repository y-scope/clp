use std::{sync::Arc, time::Duration};

use jsonwebtoken::{DecodingKey, EncodingKey};
use secrecy::ExposeSecret;
use serde_json::json;
use sqlx::{MySqlPool, mysql::MySqlPoolOptions};

use crate::{
    audit::{self, AuditEvent},
    config::{AppConfig, JwtConfig},
    db,
    error::{ServiceError, ServiceResult},
    models::{CreateCredentialRequest, CredentialMetadata},
};

/// Reference-counted handle that Axum stores as application state.
pub type SharedService = Arc<CredentialManagerService>;

/// High-level façade that wires together persistence, auditing, and JWT handling.
pub struct CredentialManagerService {
    db_pool: MySqlPool,
    jwt: JwtManager,
}

impl CredentialManagerService {
    /// Establishes the database connection pool and prepares JWT helpers.
    ///
    /// # Returns
    ///
    /// A fully initialized [`CredentialManagerService`] ready to be shared with Axum routes.
    ///
    /// # Errors
    ///
    /// * Propagates errors from [`sqlx::mysql::MySqlPoolOptions::connect_with`].
    /// * Propagates errors from [`JwtManager::new`].
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

    /// Wraps `self` in an [`Arc`] so it can be cloned into Axum handlers.
    pub fn clone_shared(self) -> SharedService {
        Arc::new(self)
    }

    /// Exposes the underlying pool for callers that need direct SQLx access.
    pub fn db_pool(&self) -> &MySqlPool {
        &self.db_pool
    }

    /// Returns the JWT helper for issuing or validating service tokens.
    pub fn jwt(&self) -> &JwtManager {
        &self.jwt
    }

    /// Fetches every credential metadata row and records the attempt in audit logs.
    pub async fn list_credentials(&self) -> ServiceResult<Vec<CredentialMetadata>> {
        match db::list_credentials(self.db_pool()).await {
            Ok(credentials) => {
                let mut event = AuditEvent::success("credential_list");
                event.metadata = Some(json!({ "count": credentials.len() }));
                audit::log_event(event);
                Ok(credentials)
            }
            Err(err) => {
                let error_message = err.to_string();
                let event = AuditEvent::failure("credential_list", &error_message);
                audit::log_event(event);
                Err(err)
            }
        }
    }

    /// Retrieves a single credential and enriches audit records with identifiers.
    pub async fn get_credential(&self, id: i64) -> ServiceResult<CredentialMetadata> {
        match db::get_credential(self.db_pool(), id).await {
            Ok(credential) => {
                let mut event = AuditEvent::success("credential_get");
                event.credential_id = Some(credential.id);
                event.credential_name = Some(&credential.name);
                audit::log_event(event);
                Ok(credential)
            }
            Err(err) => {
                let error_message = err.to_string();
                let mut event = AuditEvent::failure("credential_get", &error_message);
                event.credential_id = Some(id);
                audit::log_event(event);
                Err(err)
            }
        }
    }

    /// Attempts to delete a credential and distinguishes between success and missing rows.
    pub async fn delete_credential(&self, id: i64) -> ServiceResult<()> {
        let rows = db::delete_credential(self.db_pool(), id).await;
        match rows {
            Ok(rows_affected) if rows_affected > 0 => {
                let mut event = AuditEvent::success("credential_delete");
                event.credential_id = Some(id);
                audit::log_event(event);
                Ok(())
            }
            Ok(_) => {
                let error_message = format!("credential with id `{id}` was not found");
                let mut event = AuditEvent::failure("credential_delete", &error_message);
                event.credential_id = Some(id);
                audit::log_event(event);
                Err(ServiceError::NotFound(error_message))
            }
            Err(err) => {
                let error_message = err.to_string();
                let mut event = AuditEvent::failure("credential_delete", &error_message);
                event.credential_id = Some(id);
                audit::log_event(event);
                Err(err)
            }
        }
    }

    /// Validates, persists, and audits a new credential request.
    ///
    /// # Errors
    ///
    /// * Returns [`ServiceError::Validation`] when the input fails
    ///   [`CreateCredentialRequest::validate`].
    /// * Propagates persistence failures from [`db::create_credential`].
    pub async fn create_credential(
        &self,
        request: CreateCredentialRequest,
    ) -> ServiceResult<CredentialMetadata> {
        if let Err(err) = request.validate() {
            let error_message = err.to_string();
            let event = AuditEvent::failure("credential_create", &error_message);
            audit::log_event(event);
            return Err(err);
        }

        match db::create_credential(self.db_pool(), &request).await {
            Ok(created) => {
                let mut event = AuditEvent::success("credential_create");
                event.credential_id = Some(created.id);
                event.credential_name = Some(&created.name);
                event.metadata = Some(json!({
                    "credential_type": created.credential_type.as_str(),
                    "transient": created.transient,
                }));
                audit::log_event(event);
                Ok(created)
            }
            Err(err) => {
                let error_message = err.to_string();
                let event = AuditEvent::failure("credential_create", &error_message);
                audit::log_event(event);
                Err(err)
            }
        }
    }
}

/// Lightweight wrapper around jsonwebtoken keys plus default TTL metadata.
pub struct JwtManager {
    encoding_key: EncodingKey,
    decoding_key: DecodingKey,
    default_ttl: Duration,
}

impl JwtManager {
    /// Builds signing and verification keys from configuration while enforcing required fields.
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

    /// Returns the reusable encoding key when constructing JWT headers.
    pub fn encoding_key(&self) -> &EncodingKey {
        &self.encoding_key
    }

    /// Returns the reusable decoding key for JWT validation.
    pub fn decoding_key(&self) -> &DecodingKey {
        &self.decoding_key
    }

    /// Provides the configured token lifetime so callers can align expirations.
    pub fn default_ttl(&self) -> Duration {
        self.default_ttl
    }
}
