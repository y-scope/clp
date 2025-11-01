use secrecy::ExposeSecret;

use crate::package_config::{
    config::Database as DatabaseConfig,
    credentials::Database as DatabaseCredentials,
};

/// Creates a new `MySQL` connection pool using the provided configuration and credentials.
///
/// # Return
///
/// A newly created `MySQL` connection pool configured with the specified maximum number of
/// connections.
///
/// # Errors
///
/// Returns an error if:
///
/// * Forwards [`sqlx::mysql::MySqlPoolOptions::connect_with`]'s errors on failure.
pub async fn create_mysql_pool(
    config: &DatabaseConfig,
    credentials: &DatabaseCredentials,
    max_connections: u32,
) -> Result<sqlx::MySqlPool, crate::Error> {
    let mysql_options = sqlx::mysql::MySqlConnectOptions::new()
        .host(&config.host)
        .port(config.port)
        .database(&config.name)
        .username(&credentials.user)
        .password(credentials.password.expose_secret());

    Ok(sqlx::mysql::MySqlPoolOptions::new()
        .max_connections(max_connections)
        .connect_with(mysql_options)
        .await?)
}
