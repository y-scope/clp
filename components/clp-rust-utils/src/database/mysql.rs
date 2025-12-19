use secrecy::ExposeSecret;

use crate::clp_config::package::{
    config::{Database as DatabaseConfig, ClpDbNameType},
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
        .database(
            config
                .names
                .get(&ClpDbNameType::Clp)
                .ok_or_else(|| {
                    crate::Error::Config("Missing 'clp' database name in config".to_owned())
                })?,
        )
        .username(&credentials.user)
        .password(credentials.password.expose_secret());

    Ok(sqlx::mysql::MySqlPoolOptions::new()
        .max_connections(max_connections)
        .connect_with(mysql_options)
        .await?)
}
