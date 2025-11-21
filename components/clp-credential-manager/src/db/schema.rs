/// Canonical table name for the unified AWS credentials store.
pub const AWS_CREDENTIALS_TABLE: &str = "clp_aws_credentials";

/// Columns selected by metadata queries to avoid leaking secrets.
pub const AWS_CREDENTIAL_METADATA_COLUMNS: &str = "id, name, credential_type, description, \n    \
                                                   default_session_duration_seconds, transient, \
                                                   created_at, updated_at, last_used_at";
