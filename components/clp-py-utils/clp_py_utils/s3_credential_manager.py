import re
from contextlib import closing
from datetime import datetime

from pydantic import SecretStr
from sql_adapter import SQL_Adapter

from clp_py_utils.clp_config import (
    AwsCredential,
    CLP_METADATA_TABLE_PREFIX,
    Database,
    TemporaryCredential,
)
from clp_py_utils.clp_logging import get_logger
from clp_py_utils.clp_metadata_db_utils import (
    get_aws_credentials_table_name,
    get_aws_temporary_credentials_table_name,
)

logger = get_logger(__name__)


class S3CredentialManager:
    """Manages AWS S3 credentials in the database."""

    def __init__(self, database_config: Database):
        """
        Initializes the credential manager.

        :param database_config: Database configuration for connecting to CLP metadata database.
        """
        self.sql_adapter = SQL_Adapter(database_config)
        database_config.ensure_credentials_loaded()
        conn_params = database_config.get_clp_connection_params_and_type()
        self.table_prefix = conn_params.get("table_prefix", CLP_METADATA_TABLE_PREFIX)

    def create_credential(
        self,
        name: str,
        access_key_id: str,
        secret_access_key: str,
        role_arn: str | None = None,
    ) -> int:
        """
        Creates a new AWS long-term credential entry.

        :param name: Unique name for the credential.
        :param access_key_id:
        :param secret_access_key:
        :param role_arn: Optional ARN of IAM role to assume.
        :return: The ID of the created credential.
        :raises ValueError: If name already exists or validation fails.
        """
        # Validate inputs
        self._validate_credential_name(name)
        self._validate_access_key_id(access_key_id)
        self._validate_secret_access_key(secret_access_key)

        table_name = get_aws_credentials_table_name(self.table_prefix)

        with (
            closing(self.sql_adapter.create_connection(True)) as db_conn,
            closing(db_conn.cursor(dictionary=True)) as cursor,
        ):
            # Check for duplicate name
            cursor.execute(f"SELECT id FROM `{table_name}` WHERE name = %s", (name,))
            if cursor.fetchone():
                raise ValueError(f"Credential with name '{name}' already exists")

            cursor.execute(
                f"""
                INSERT INTO `{table_name}`
                (name, access_key_id, secret_access_key, role_arn)
                VALUES (%s, %s, %s, %s)
                """,
                (name, access_key_id, secret_access_key, role_arn),
            )
            db_conn.commit()

            credential_id = cursor.lastrowid
            logger.info("Created credential '%s' with ID %s", name, credential_id)
            return credential_id

    def list_credentials(self) -> list[tuple[int, str, datetime]]:
        """
        Lists all credential entries (metadata only, no secrets).

        :return: List of tuples containing:
            - The credential id.
            - The credential name.
            - The time the credential was created at.
        """
        table_name = get_aws_credentials_table_name(self.table_prefix)

        with (
            closing(self.sql_adapter.create_connection(True)) as db_conn,
            closing(db_conn.cursor(dictionary=True)) as cursor,
        ):
            cursor.execute(
                f"""
                SELECT id, name, created_at
                FROM `{table_name}`
                ORDER BY name
                """
            )
            rows = cursor.fetchall()
            return [(row["id"], row["name"], row["created_at"]) for row in rows]

    def get_credential_by_id(self, credential_id: int) -> AwsCredential | None:
        """
        Retrieves a long-term credential by ID (includes secrets).

        :param credential_id:
        :return: The AwsCredential object representing the database entry or None if not found.
        """
        table_name = get_aws_credentials_table_name(self.table_prefix)

        with (
            closing(self.sql_adapter.create_connection(True)) as db_conn,
            closing(db_conn.cursor(dictionary=True)) as cursor,
        ):
            cursor.execute(
                f"""
                SELECT id, name, access_key_id, secret_access_key,
                       role_arn, created_at, updated_at
                FROM `{table_name}`
                WHERE id = %s
                """,
                (credential_id,),
            )
            row = cursor.fetchone()

            if not row:
                return None

            return AwsCredential(
                id=row["id"],
                name=row["name"],
                access_key_id=SecretStr(row["access_key_id"]),
                secret_access_key=SecretStr(row["secret_access_key"]),
                role_arn=row["role_arn"],
                created_at=row["created_at"],
                updated_at=row["updated_at"],
            )

    def get_credential_by_name(self, name: str) -> AwsCredential | None:
        """
        Retrieves a long-term credential by name (includes secrets).

        :param name:
        :return: The AwsCredential object representing the database entry or None if not found.
        """
        table_name = get_aws_credentials_table_name(self.table_prefix)

        with (
            closing(self.sql_adapter.create_connection(True)) as db_conn,
            closing(db_conn.cursor(dictionary=True)) as cursor,
        ):
            cursor.execute(
                f"""
                SELECT id, name, access_key_id, secret_access_key,
                       role_arn, created_at, updated_at
                FROM `{table_name}`
                WHERE name = %s
                """,
                (name,),
            )
            row = cursor.fetchone()

            if not row:
                return None

            return AwsCredential(
                id=row["id"],
                name=row["name"],
                access_key_id=SecretStr(row["access_key_id"]),
                secret_access_key=SecretStr(row["secret_access_key"]),
                role_arn=row["role_arn"],
                created_at=row["created_at"],
                updated_at=row["updated_at"],
            )

    def update_credential(
        self,
        credential_id: int,
        name: str | None = None,
        access_key_id: str | None = None,
        secret_access_key: str | None = None,
        role_arn: str | None = None,
    ) -> bool:
        """
        Updates an existing long-term credential. Only provided fields are updated.

        :param credential_id: The credential ID to update.
        :param name: New name (must be unique).
        :param access_key_id: New access key ID.
        :param secret_access_key: New secret access key.
        :param role_arn: New role ARN (use '' to clear).
        :return: True if updated, False if credential not found.
        :raises ValueError: If name conflicts or validation fails.
        """
        if all(v is None for v in [name, access_key_id, secret_access_key, role_arn]):
            raise ValueError("At least one field must be specified for update")

        table_name = get_aws_credentials_table_name(self.table_prefix)

        with (
            closing(self.sql_adapter.create_connection(True)) as db_conn,
            closing(db_conn.cursor(dictionary=True)) as cursor,
        ):
            # Check if credential exists
            cursor.execute(f"SELECT id FROM `{table_name}` WHERE id = %s", (credential_id,))
            if not cursor.fetchone():
                return False

            # Validate updates
            if name is not None:
                self._validate_credential_name(name)
                # Check name uniqueness
                cursor.execute(
                    f"SELECT id FROM `{table_name}` WHERE name = %s AND id != %s",
                    (name, credential_id),
                )
                if cursor.fetchone():
                    raise ValueError(f"Credential with name '{name}' already exists")

            if access_key_id is not None:
                self._validate_access_key_id(access_key_id)
            if secret_access_key is not None:
                self._validate_secret_access_key(secret_access_key)

            # Build UPDATE query dynamically
            update_fields = []
            params = []

            if name is not None:
                update_fields.append("name = %s")
                params.append(name)
            if access_key_id is not None:
                update_fields.append("access_key_id = %s")
                params.append(access_key_id)
            if secret_access_key is not None:
                update_fields.append("secret_access_key = %s")
                params.append(secret_access_key)
            if role_arn is not None:
                update_fields.append("role_arn = %s")
                params.append(role_arn if role_arn != "" else None)

            params.append(credential_id)

            cursor.execute(
                f"""
                UPDATE `{table_name}`
                SET {', '.join(update_fields)}
                WHERE id = %s
                """,
                params,
            )
            db_conn.commit()

            logger.info("Updated credential ID %s", credential_id)
            return True

    def delete_credential(self, credential_id: int) -> bool:
        """
        Deletes a credential by ID.

        The foreign key constraint with ON DELETE CASCADE will automatically delete any cached
        temporary credentials associated with this credential.

        :param credential_id:
        :return: True if deleted, False if not found.
        """
        table_name = get_aws_credentials_table_name(self.table_prefix)

        with (
            closing(self.sql_adapter.create_connection(True)) as db_conn,
            closing(db_conn.cursor(dictionary=True)) as cursor,
        ):
            cursor.execute(f"DELETE FROM `{table_name}` WHERE id = %s", (credential_id,))
            deleted = cursor.rowcount > 0
            db_conn.commit()

            if deleted:
                logger.info("Deleted credential ID %s", credential_id)
            else:
                logger.warning(f"Credential ID {credential_id} not found for deletion")

            return deleted

    # Temporary credential cache methods

    def cache_session_token(
        self,
        long_term_key_id: int,
        access_key_id: str,
        secret_access_key: str,
        session_token: str,
        source: str,
        expires_at: datetime,
    ) -> int:
        """
        Caches a session token in the temporary credentials table.

        :param long_term_key_id: ID of the associated long-term credential.
        :param access_key_id:
        :param secret_access_key:
        :param session_token:
        :param source: Origin identifier (role ARN or S3 resource ARN).
        :param expires_at: When the session token expires.
        :return: The ID of the cached credential.
        :raises ValueError: If validation fails.
        """
        if not access_key_id or not secret_access_key or not session_token:
            raise ValueError("Temporary credentials cannot have empty fields")

        if not source:
            raise ValueError("Source cannot be empty")

        if len(source) > 2048:
            raise ValueError("Source cannot exceed 2048 characters")

        table_name = get_aws_temporary_credentials_table_name(self.table_prefix)

        with (
            closing(self.sql_adapter.create_connection(True)) as db_conn,
            closing(db_conn.cursor(dictionary=True)) as cursor,
        ):
            cursor.execute(
                f"""
                INSERT INTO `{table_name}`
                (long_term_key_id, access_key_id, secret_access_key, session_token, source, expires_at)
                VALUES (%s, %s, %s, %s, %s, %s)
                """,
                (
                    long_term_key_id,
                    access_key_id,
                    secret_access_key,
                    session_token,
                    source,
                    expires_at,
                ),
            )
            db_conn.commit()

            credential_id = cursor.lastrowid
            logger.info(
                "Cached session token for source '%s' with ID '%s', expires at %s",
                source,
                credential_id,
                expires_at,
            )
            return credential_id

    def get_cached_session_token_by_source(
        self, source: str, long_term_key_id: int | None = None
    ) -> TemporaryCredential | None:
        """
        Retrieves a valid (non-expired) cached session token by source.

        Looks up session tokens by source ARN (role or S3 resource). Optionally filters
        by long_term_key_id for more specific lookups.

        :param source: Source identifier (role ARN or S3 resource ARN).
        :param long_term_key_id: Optional filter by associated long-term credential ID.
        :return: TemporaryCredential object or None if not found or expired.
        """
        table_name = get_aws_temporary_credentials_table_name(self.table_prefix)

        with (
            closing(self.sql_adapter.create_connection(True)) as db_conn,
            closing(db_conn.cursor(dictionary=True)) as cursor,
        ):
            # Build query with optional long_term_key_id filter
            query = f"""
                SELECT id, long_term_key_id, access_key_id, secret_access_key,
                       session_token, source, expires_at, created_at
                FROM `{table_name}`
                WHERE source = %s AND expires_at > NOW()
            """
            params = [source]

            if long_term_key_id is not None:
                query += " AND long_term_key_id = %s"
                params.append(long_term_key_id)

            query += " ORDER BY expires_at DESC LIMIT 1"

            cursor.execute(query, params)
            row = cursor.fetchone()

            if not row:
                return None

            return TemporaryCredential(
                id=row["id"],
                long_term_key_id=row["long_term_key_id"],
                access_key_id=SecretStr(row["access_key_id"]),
                secret_access_key=SecretStr(row["secret_access_key"]),
                session_token=SecretStr(row["session_token"]),
                source=row["source"],
                expires_at=row["expires_at"],
                created_at=row["created_at"],
            )

    def cleanup_expired_session_tokens(self) -> int:
        """
        Deletes expired session tokens from the cache.

        :return: Count of deleted credentials.
        """
        table_name = get_aws_temporary_credentials_table_name(self.table_prefix)

        with (
            closing(self.sql_adapter.create_connection(True)) as db_conn,
            closing(db_conn.cursor(dictionary=True)) as cursor,
        ):
            cursor.execute(
                f"""
                DELETE FROM `{table_name}`
                WHERE expires_at < NOW()
                """
            )
            deleted_count = cursor.rowcount
            db_conn.commit()

            if deleted_count > 0:
                logger.info("Cleaned up %s expired session token(s)", deleted_count)
            else:
                logger.debug("No expired session tokens to clean up")

            return deleted_count

    def _validate_credential_name(self, name: str) -> None:
        """
        Validates credential name.

        :param name: Name to validate.
        :raises ValueError: If name is invalid.
        """
        if not name or not name.strip():
            raise ValueError("Credential name cannot be empty")

        if len(name) > 255:
            raise ValueError("Credential name cannot exceed 255 characters")

        if " " in name:
            raise ValueError("Credential name cannot contain spaces")

        # Check for special characters (allow alphanumeric, hyphens, underscores)
        if not re.match(r"^[a-zA-Z0-9_-]+$", name):
            raise ValueError(
                "Credential name can only contain alphanumeric characters, hyphens, and underscores"
            )

    def _validate_access_key_id(self, access_key_id: str) -> None:
        """
        Validates AWS access key ID format.

        :param access_key_id: Access key to validate.
        :raises ValueError: If invalid.
        """
        if not access_key_id or not access_key_id.strip():
            raise ValueError("Access key ID cannot be empty")

    def _validate_secret_access_key(self, secret_access_key: str) -> None:
        """
        Validates AWS secret access key.

        :param secret_access_key: Secret key to validate.
        :raises ValueError: If invalid.
        """
        if not secret_access_key or not secret_access_key.strip():
            raise ValueError("Secret access key cannot be empty")
