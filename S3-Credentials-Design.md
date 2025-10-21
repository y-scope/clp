# Overview

This document outlines a design to refactor how AWS S3 credentials are stored and managed in the CLP package. Currently, S3 credentials are stored directly in the `clp-config.yml` file and are also read into job configurations. This approach poses security risks, as these credentials can be exposed to the web UI or other components. This document presents options for creating a separate database table to store S3 credentials with enhanced security and providing a web UI for credential management.

**NOTE:** This document presents several options for implementing the refactor, with a recommendation for a clean break from the previous approach to ensure maximum security and system simplicity. The final decision will be made after considering all options and their trade-offs.

# Use case

The current approach to S3 credential management has several limitations:

1. **Security Risk**: S3 credentials are stored with job configurations in the DB and can be exposed through the web UI when querying compression job configurations.
2. **Inconvenience**: Service maintainers must update `clp-config.yml` and restart the package for every new log input source.
3. **Limited Scalability**: Difficult to onboard future use cases like an S3 log scanner that requires regular S3 authentication for different inputs sources, which usually requires different sets of credentials. e.g., the S3 log scanner to be added.W

The proposed solution addresses these issues by:

1. Creating a secure credential storage system in the database
2. Providing a web UI for credential management
3. Enabling credential reference by identifier in job configurations
4. Supporting future use cases requiring regular S3 authentication, such as the S3 log scanner

# Requirements

## Functional requirements

1. Store AWS S3 credentials (access key ID and secret access key) in a dedicated database table
2. Protect the credential table with a separate database user with higher privileges
3. Provide a web UI for managing (creating, listing, updating, deleting) credential pairs
4. Allow compression/query jobs to reference credentials by identifier
5. Handle all AWS authentication types, including special handling for `AwsAuthType.ec2`

Note: This proposal recommends that all S3 operations use the new credential management system. There would be no support for backward compatibility from previous credential handling methods in this approach.

## Non-Functional requirements

1. Clear migration path for users transitioning from previous credential configuration methods
2. Robust error handling and logging

# High-level Design

## Database schema changes

1. Create a new `aws_credentials` table in the clp-db with the following structure
    
    ```sql
    CREATE TABLE `aws_credentials` (
        `id` INT NOT NULL AUTO_INCREMENT,
        `name` VARCHAR(255) NOT NULL UNIQUE,
        `access_key_id` VARCHAR(255) NOT NULL,
        `secret_access_key` VARCHAR(255) NOT NULL,
        `session_token` VARCHAR(255),
        `created_at` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
        `updated_at` DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
        PRIMARY KEY (`id`) USING BTREE
    ) ROW_FORMAT=DYNAMIC;
    ```
    
    Alternatively, we can create the table with a flexible schema to store credentials of types other than just S3. See [**Future-proofing for other credential types**](https://www.notion.so/Future-proofing-for-other-credential-types-27104e4d9e6b80b19235dd673290b41e?pvs=21) for more details.
    
2. The compression job table column `clp_config` will not be changed, but it should no long carry direct credential information (i.e., 
`AwsAuthentication`.`S3Credentials` objects). Instead, references such as IDs of rows in the `aws_credentials` should be stored.

## Credential manager

Users will be able to manage their credentials using:

1. A new `admin-tools/credential-manager.sh` script that can read credentials from:
    1. Cmdline args
    2. Profiles in the `~/.aws/credentials` store
    3. Environment variables
2. A new “Credential Manager” component in the Web UI

## Credentials management flow

1. CLP deployment maintainer / WebUI users create named credential pairs through the `credential-manager.sh` script / WebUI. A default credential pair can be specified for convenience.
2. When submitting compression/query jobs, users select from available credential pairs by name. When not provide, the default credential pair is used.
3. During job execution, the scheduler retrieves the actual credentials using the privileged database user
4. Credentials are passed to the `clp-s` binary through environment variables as before

### Special Handling for Temporary Credentials

The user may want to use a set of low-privilege credentials that can authenticate to generate a set of temporary credentials for use in jobs. This is done through role assumption, where the resulting set of credentials have the set of permissions defined by the IAM role. In addition to the underlying set of credentials, a role and reference to these credentials must be provided. 

These could be stored in the clp-db as such:

1. Add optional `role_arn` and `source_name` columns to the `aws_credentials` table.
2. The `credential-manager` script would be used to re-authenticate whenever necessary using the credentials stored in the role with `source_name` .

This approach may cause confusion with which credential set should be used, as well as adds potentially unused columns to the table.

### Special Handling for AwsAuthType.ec2

The `AwsAuthType.ec2` authentication method doesn't require stored credentials as it uses IAM roles attached to EC2 instances. For this auth type:

1. No credential pair needs to be selected in job submission
2. The clp-s binary will use the EC2 instance's IAM role as before
3. This will be handled as a special case in the job configuration

## Database user privileges

1. Create a new database user with elevated privileges for accessing the `aws_credentials` table
2. Restrict access to the `aws_credentials` table for the web UI dashboard query user.
3. Ensure the scheduler / worker / webui credential manager components can use the privileged user when accessing credentials.

# **Subsystems**

## Configuration layer

### Modify job configurations

Update job configuration models to include optional S3 credential references:

```python
# clp-py-utils/clp_py_utils/clp_config.py
class S3Config(BaseModel):
# REMOVE:
#    aws_authentication: AwsAuthentication
# ADD:
    aws_authentication_id: Optional[int] = None

# In job_orchestration/scheduler/job_config.py
class S3InputConfig(S3Config):
    pass
```

## Database Access Layer

### New Credential DB Manager

Create a new module for managing S3 credentials in the DB:

```python

# clp-py-utils/clp_py_utils/clp_config.py
class AwsCredential:
    id: int
    name: str
    created_at: datetime = datetime.now()
    updated_at: datetime = datetime.now()
    
    access_key_id: SecretStr
    secret_access_key: SecretStr
    session_token: Optional[SecretStr] = None

# clp_py_utils/s3_credential_manager.py
class S3CredentialManager:
    def __init__(self, privileged_db_config):
        self.sql_adapter = SQL_Adapter(privileged_db_config)
    
    def create_credential(self, name: str, access_key_id: str, secret_access_key: str) -> int:
        pass
    
    def list_credentials(self) -> List[Tuple[int, str]]:
        # Lists crentials in (id, name) tuples
        pass

    def get_credential_by_id(self, credential_id: int) -> Optional[AwsCredential]:
        pass
    
    def update_credential(self, credential_id: int, name: str = None,
                         access_key_id: Optional[str] = None,
                         secret_access_key: Optional[str] = None,
                         session_token: Optional[str] = None):
        pass
    
    def delete_credential(self, credential_id: int):
        pass
```

### Credential manager script

The `admin-tools/credential-manager.sh` script provides a command-line interface for managing AWS S3 credentials in the database. It follows the same pattern as other admin tools in the CLP package, acting as a wrapper around a Python script that performs the actual operations.

**Command line arguments**

The credential manager supports the following commands and options:

```
	credential-manager.sh [options] <command> [command-args]

Options:
  -c, --config PATH       Path to CLP configuration file
  -v, --verbose           Enable verbose logging

Commands:
  create                  Create a new credential entry
  list                    List all credential entries (by name and ID only)
  get ID                  Get a credential entry by ID
  update ID               Update a credential entry by ID
  delete ID               Delete a credential entry by ID

Create command options:
  -n, --name NAME         Name for the credential entry
  --access-key-id KEY     AWS access key ID
  --secret-access-key KEY AWS secret access key
  --section-token KEY     AWS session token
  --from-profile PROFILE  Read credentials from AWS profile
  --from-env              Read credentials from environment variables

Update command options:
  -n, --name NAME         New name for the credential entry
  --access-key-id KEY     New AWS access key ID
  --secret-access-key KEY New AWS secret access key
  --section-token KEY     AWS session token
```

**Credential sources**

The credential manager can read credentials from three different sources:

1. **Command line arguments** (`AwsAuthType.credentials`): Directly provided access key ID and secret access key
2. **AWS profiles** (`AwsAuthType.profile`): Credentials from `~/.aws/credentials` file using a specified profile name
3. **Environment variables** (`AwsAuthType.env_vars`): Reading `AWS_ACCESS_KEY_ID` , `AWS_SECRET_ACCESS_KEY` , and `AWS_SESSION_TOKEN`(if exists) from the environment

For above 3  `AwsAuthType`s, the credential manager handles the reading of credentials in the same way, storing only the access key ID and secret access key in the database. The `AwsAuthType.ec2` type doesn't require stored credentials and is handled as a special case in job configurations.

## **Scheduler / worker modifications**

If compression job configurations store only credential references, each execution must first retrieve the credentials. Though both the compression scheduler and worker(s) have access to the clp-db, letting the scheduler to access the credentials and including them in the task payload provides more benefits:

1. The individual workers do not need to access the credentials separately, duplicating the work.
2. Credential access can be granted to only the scheduler, which reduces risks of a potentially broader leak in the worker.

### Compression scheduler

Modify `job_orchestration/scheduler/compress/compression_scheduler.py` to:

1. Accept the privileged database configuration
2. Retrieve S3 credentials when a job references them
3. Include credentials in the task payload when needed

**Compression worker**

Modify `job_orchestration/executor/compress/compression_task.py` to:

1. Accept S3 credentials in the task parameters
2. Use provided credentials instead of reading from configuration when available

## WebUI components

### Backend API

Create new API endpoints in the web UI server:

1. `GET /api/credentials` - List all credential references
2. `POST /api/credentials` - Create a new credential pair
3. `PUT /api/credentials/{id}` - Update a credential pair
4. `DELETE /api/credentials/{id}` - Delete a credential pair

### Frontend UI

Create a new section in the web UI for managing S3 credentials:

1. Credential listing page with name and creation date
2. Create/edit form with name, access key ID, secret access key, and optionally session token fields
3. Integration with job submission forms to select credential pairs

# Verification

## Unit tests

1. Test S3CredentialManager CRUD operations
2. Test credential retrieval with various authentication types
3. Test error handling for invalid credential references

## Integration tests

1. Test end-to-end job submission with referenced credentials
2. Test web UI credential management workflows
3. Test EC2 authentication type special handling
4. Test privilege separation between web UI and scheduler users

## Security tests

1. Verify that the web UI database user cannot access the s3_credentials table
2. Verify that only the privileged user can access credential secrets
3. Test credential encryption at rest (if implemented)
4. Verify proper input validation and sanitization

## Performance tests

1. Measure impact of credential lookup on job submission latency
2. Test credential management UI with large numbers of credential pairs
3. Verify no significant performance degradation in job execution

## Future considerations

1. **Query flow refactoring**
    - Credentials reading in the query flow can also be refactored if in the future we support multiple s3 destinations.
    - For now, this is not planned.
2. **Future-proofing for other credential types**
    
    While this refactor focuses on S3 credentials, we should consider how the design might accommodate other types of credentials for future use cases such as SFTP or WebDAV authentication. There are a few approaches we could take:
    
    1. **Generic credential model with metadata field (incompatible with current design proposal)**
        - Extend the database schema to include a `credential_type` field to distinguish between different types of credentials (e.g., 's3', 'sftp', 'webdav')
            - Add a `metadata` field (JSON format) to store type-specific configuration parameters
            - Modify the credential reference model to include the credential type
            - This would allow the same table structure to accommodate different types of credentials in the future, and requires credential handlers that can parse the metadata based on the credential type
                - For example, S3 credentials could be stored as:
                    
                    ```json
                    {
                      "access_key_id": "AKIA...",
                      "secret_access_key": "secret"
                    }
                    
                    ```
                    
                - While SFTP credentials might be stored as:
                    
                    ```json
                    {
                      "username": "user",
                      "password": "password",
                      "private_key": "-----BEGIN RSA PRIVATE KEY-----..."
                    }
                    
                    ```
                    
    2. **Plugin-based approach (compatible with current design proposal)**
        - Other types of inputs might be implemented as plugins with their own credential storage mechanisms
        - In this case, we might not need to make the current credential store table generic, as each plugin could manage its own credentials
        - This approach would provide better isolation between different credential types but might require more development effort for each new credential type
3. **Credential encryption**
    - Implement encryption at rest for S3 credentials in the database using AES-256 encryption
    - Store encryption keys separately using a key management service (KMS) or hardware security module (HSM)
    - Modify the Credential Manager to automatically encrypt credentials on storage and decrypt on retrieval
    - Add configuration options for encryption algorithms and key rotation policies
    - Ensure encryption keys are never stored in the same database as the encrypted credentials
4. **Role-based access**
    - Implement role-based access control (RBAC) for credential management in the web UI
    - Create predefined roles such as "admin" (full access), "operator" (read-only access), and "user" (limited access)
    - Add a user management system with role assignments stored in the database
    - Implement permission checks in all credential management API endpoints
    - Add UI elements to control access based on user roles, such as hiding certain actions for non-admin users
5. **Audit logging**
    - Add comprehensive audit logging for all credential-related operations
    - Log credential creation, modification, deletion, and access attempts with timestamps and user information
    - Store audit logs in a separate table with immutable entries to prevent tampering
    - Implement log retention policies and archiving mechanisms for compliance purposes
    - Add API endpoints and UI components to view and search audit logs
    - Include detailed information in logs such as IP addresses, user agents, and operation outcomes
6. **Retention Support**:
    - Implement time-to-live (TTL) policies for credentials to automatically expire old or unused credentials
    - Add a `expires_at` field to the credentials table to track when credentials should be considered expired
    - Implement a cleanup process that can automatically remove expired credentials
    - Provide configuration options for retention periods based on security policies
    - Add UI components to view and manage credential expiration dates

# Schedule

1. Create the new SQL table: 3 days.
Src: https://github.com/y-scope/clp/blob/main/components/clp-py-utils/clp_py_utils/initialize-clp-metadata-db.py
2. Credential DB Manager class: 5 days.
3. Credential manager script:
    1. keys: 2 days
    2. profile: 4 days
    3. env-vars: 2 days
4. Update job execution flow:
    1. Job scheduler: 3 days
    2. job worker: 3 days
5. WebUI:
    1. Add a DB manger in the backend server: 4 days
    2. Add RESTful APIs: 4 days
    3. Add credentials manager UI: 5 days
    4. Add credential selector drop-down to job submission UI: 3 days