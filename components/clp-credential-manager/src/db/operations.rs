use secrecy::ExposeSecret;
use sqlx::{MySqlPool, mysql::MySqlQueryResult};

use crate::{
    db::schema::{AWS_CREDENTIAL_METADATA_COLUMNS, AWS_CREDENTIALS_TABLE},
    error::{ServiceError, ServiceResult},
    models::{CreateCredentialRequest, CredentialMetadata, CredentialMetadataRow},
};

/// Returns all credential metadata sorted alphabetically by name.
pub async fn list_credentials(pool: &MySqlPool) -> ServiceResult<Vec<CredentialMetadata>> {
    let query = format!(
        "SELECT {columns} FROM {table} ORDER BY name",
        columns = AWS_CREDENTIAL_METADATA_COLUMNS,
        table = AWS_CREDENTIALS_TABLE
    );

    let rows = sqlx::query_as::<_, CredentialMetadataRow>(&query)
        .fetch_all(pool)
        .await?;

    rows.into_iter().map(CredentialMetadata::try_from).collect()
}

/// Fetches a single credential row by ID.
pub async fn get_credential(pool: &MySqlPool, id: i64) -> ServiceResult<CredentialMetadata> {
    let query = format!(
        "SELECT {columns} FROM {table} WHERE id = ?",
        columns = AWS_CREDENTIAL_METADATA_COLUMNS,
        table = AWS_CREDENTIALS_TABLE
    );

    let row = sqlx::query_as::<_, CredentialMetadataRow>(&query)
        .bind(id)
        .fetch_optional(pool)
        .await?;

    match row {
        Some(row) => CredentialMetadata::try_from(row),
        None => Err(ServiceError::NotFound(format!(
            "credential with id `{id}` was not found"
        ))),
    }
}

/// Deletes the credential row and returns the affected row count.
pub async fn delete_credential(pool: &MySqlPool, id: i64) -> ServiceResult<u64> {
    let query = format!(
        "DELETE FROM {table} WHERE id = ?",
        table = AWS_CREDENTIALS_TABLE
    );

    let result = sqlx::query(&query).bind(id).execute(pool).await?;
    Ok(result.rows_affected())
}

/// Persists a new credential then fetches the stored metadata.
///
/// # Errors
///
/// * Propagates validation/persistence failures from [`insert_credential`].
/// * Propagates errors from [`get_credential`] when reloading the inserted row.
pub async fn create_credential(
    pool: &MySqlPool,
    request: &CreateCredentialRequest,
) -> ServiceResult<CredentialMetadata> {
    let result = insert_credential(pool, request).await?;
    let inserted_id = result.last_insert_id() as i64;
    get_credential(pool, inserted_id).await
}

/// Raw insert helper that maps low-level driver errors into [`ServiceError`] variants.
async fn insert_credential(
    pool: &MySqlPool,
    request: &CreateCredentialRequest,
) -> ServiceResult<MySqlQueryResult> {
    let query = format!(
        "INSERT INTO {table} (name, credential_type, access_key_id, secret_access_key, role_arn, \
         external_id, description, default_session_duration_seconds, transient, created_by) \
         VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
        table = AWS_CREDENTIALS_TABLE
    );

    let name = request.name.as_str();
    let credential_type = request.credential_type.as_str();
    let access_key_id = request.access_key_id.as_deref();
    let secret_access_key = request
        .secret_access_key
        .as_ref()
        .map(|secret| secret.expose_secret().to_owned());
    let role_arn = request.role_arn.as_deref();
    let external_id = request.external_id.as_deref();
    let description = request.description.as_deref();
    let default_duration = request.default_session_duration_seconds.unwrap_or(3600);
    let transient = request.transient.unwrap_or(false);
    let created_by = request.created_by.as_deref();

    sqlx::query(&query)
        .bind(name)
        .bind(credential_type)
        .bind(access_key_id)
        .bind(secret_access_key.as_deref())
        .bind(role_arn)
        .bind(external_id)
        .bind(description)
        .bind(default_duration)
        .bind(transient)
        .bind(created_by)
        .execute(pool)
        .await
        .map_err(|err| map_insert_error(err, name))
}

/// Converts insert failures into domain-specific conflicts when possible.
fn map_insert_error(err: sqlx::Error, name: &str) -> ServiceError {
    let is_duplicate = err
        .as_database_error()
        .and_then(|db_err| db_err.try_downcast_ref::<sqlx::mysql::MySqlDatabaseError>())
        .is_some_and(|mysql_err| mysql_err.number() == 1062);

    if is_duplicate {
        return ServiceError::Conflict(format!("credential with name `{}` already exists", name));
    }

    ServiceError::from(err)
}
