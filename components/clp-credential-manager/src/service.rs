use std::{sync::Arc};

use secrecy::ExposeSecret;
use sqlx::{MySqlPool, mysql::MySqlPoolOptions};

use crate::{
    config::{AppConfig},
    error::{ServiceResult},
};

/// Reference-counted handle that Axum stores as application state.
pub type SharedService = Arc<CredentialManagerService>;

/// High-level façade that wires together persistence, auditing, and JWT handling.
#[allow(dead_code)]
pub struct CredentialManagerService {
    db_pool: MySqlPool,
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

        Ok(Self { db_pool: pool})
    }

    /// Wraps `self` in an [`Arc`] so it can be cloned into Axum handlers.
    pub fn clone_shared(self) -> SharedService {
        Arc::new(self)
    }
}
