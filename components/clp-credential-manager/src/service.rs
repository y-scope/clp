use std::{sync::Arc};

use secrecy::ExposeSecret;
use sqlx::{MySqlPool, mysql::MySqlPoolOptions};

use crate::{
    config::{AppConfig},
    error::{ServiceResult},
};

/// Reference-counted handle that Axum stores as application state.
pub type SharedService = Arc<CredentialManagerService>;

/// High-level facade that wires together persistence, auditing, and JWT handling.
#[allow(dead_code)]
pub struct CredentialManagerService {
    db_pool: MySqlPool,
}

impl CredentialManagerService {
    /// Establishes the database connection pool used by all route handlers.
    ///
    /// # Parameters:
    ///
    /// * `config`: Fully parsed application configuration that contains database settings.
    ///
    /// # Returns:
    ///
    /// A fully initialized [`CredentialManagerService`] ready to be shared with Axum routes.
    ///
    /// # Errors:
    ///
    /// * Propagates errors from [`MySqlPoolOptions::connect_with`] when the pool cannot be created.
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
    ///
    /// # Returns:
    ///
    /// A [`SharedService`] reference-counted pointer that implements [`Clone`].
    pub fn clone_shared(self) -> SharedService {
        Arc::new(self)
    }
}
