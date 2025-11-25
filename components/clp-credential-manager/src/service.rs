#![allow(dead_code)]

use std::{sync::Arc, time::Duration};

use jsonwebtoken::{DecodingKey, EncodingKey};
use secrecy::ExposeSecret;
use sqlx::{MySqlPool, mysql::MySqlPoolOptions};

use crate::{
    config::{AppConfig, JwtConfig},
    error::{ServiceError, ServiceResult},
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
    /// # Returns:
    ///
    /// A fully initialized [`CredentialManagerService`] ready to be shared with Axum routes.
    ///
    /// # Errors:
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

    /// Exposes the underlying pool for callers that need direct `SQLx` access.
    pub const fn db_pool(&self) -> &MySqlPool {
        &self.db_pool
    }

    /// Returns the JWT helper for issuing or validating service tokens.
    pub const fn jwt(&self) -> &JwtManager {
        &self.jwt
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
    pub const fn encoding_key(&self) -> &EncodingKey {
        &self.encoding_key
    }

    /// Returns the reusable decoding key for JWT validation.
    pub const fn decoding_key(&self) -> &DecodingKey {
        &self.decoding_key
    }

    /// Provides the configured token lifetime so callers can align expirations.
    pub const fn default_ttl(&self) -> Duration {
        self.default_ttl
    }
}
