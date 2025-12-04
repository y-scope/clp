use std::sync::Arc;

use clp_rust_utils::{
    clp_config::package::{config as package_config, credentials as package_credentials},
    database::mysql::create_mysql_pool,
};
use sqlx::MySqlPool;

use crate::{config::CredentialManagerConfig, error::ServiceError};

/// High-level facade that wires together persistence, auditing, and JWT handling.
#[allow(dead_code)]
pub struct CredentialManagerService {
    db_pool: MySqlPool,
}

/// State handle that Axum stores and clones for each request.
#[derive(Clone)]
pub struct CredentialManagerServiceState {
    inner: Arc<CredentialManagerService>,
}

impl CredentialManagerService {
    /// Establishes the database connection pool used by all route handlers.
    ///
    /// # Returns
    ///
    /// A fully initialized [`CredentialManagerService`] ready to be shared with Axum routes.
    ///
    /// # Errors
    ///
    /// Returns an error if:
    ///
    /// * [`create_mysql_pool`] fails to create the pool.
    pub async fn new(config: &CredentialManagerConfig) -> Result<Self, ServiceError> {
        let db_config = &config.database;
        let pool_config = package_config::Database {
            host: db_config.host.clone(),
            port: db_config.port,
            name: db_config.name.clone(),
        };
        let pool_credentials = package_credentials::Database {
            user: db_config.user.clone(),
            password: db_config.password.clone(),
        };

        let pool = create_mysql_pool(&pool_config, &pool_credentials, db_config.max_connections)
            .await?;

        Ok(Self { db_pool: pool })
    }

    /// Consumes the service and wraps it in the sharable state handle.
    pub fn into_state(self) -> CredentialManagerServiceState {
        CredentialManagerServiceState::new(self)
    }
}

impl CredentialManagerServiceState {
    /// Wraps the provided service inside an [`Arc`] so Axum can clone it per request.
    pub fn new(service: CredentialManagerService) -> Self {
        Self {
            inner: Arc::new(service),
        }
    }
}
