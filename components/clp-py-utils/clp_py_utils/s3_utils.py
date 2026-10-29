import os
import re
from collections.abc import Generator
from pathlib import Path
from typing import Final

import boto3
import botocore
from botocore.config import Config
from job_orchestration.scheduler.job_config import S3InputConfig

from clp_py_utils.clp_config import (
    ARCHIVE_MANAGER_ACTION_NAME,
    AwsAuthentication,
    AwsAuthType,
    ClpConfig,
    COMPRESSION_SCHEDULER_COMPONENT_NAME,
    COMPRESSION_WORKER_COMPONENT_NAME,
    FsStorage,
    GARBAGE_COLLECTOR_COMPONENT_NAME,
    QUERY_SCHEDULER_COMPONENT_NAME,
    QUERY_WORKER_COMPONENT_NAME,
    S3Config,
    S3Credentials,
    S3Storage,
    StorageType,
    WEBUI_COMPONENT_NAME,
)
from clp_py_utils.core import FileMetadata

# Constants
AWS_ENDPOINT: Final[str] = "amazonaws.com"
AWS_ENV_VAR_ACCESS_KEY_ID: Final[str] = "AWS_ACCESS_KEY_ID"
AWS_ENV_VAR_SECRET_ACCESS_KEY: Final[str] = "AWS_SECRET_ACCESS_KEY"
AWS_ENV_VAR_SESSION_TOKEN: Final[str] = "AWS_SESSION_TOKEN"

S3_OBJECT_DELETION_BATCH_SIZE_MAX: Final[int] = 1000

SCHEME_REGEXP = r"(?P<scheme>(http|https))"
S3_PREFIX_REGEXP = r"(?P<s3>s3)"
ENDPOINT_REGEXP = r"(?P<endpoint>[a-z0-9.-]+(\:[0-9]+)?)"
REGION_CODE_REGEXP = r"(?P<region_code>[a-z0-9\-]+)"
BUCKET_NAME_REGEXP = r"(?P<bucket_name>[a-z0-9.-]+)"
KEY_PREFIX_REGEXP = r"(?P<key_prefix>[^?]+)"


def _get_session_credentials(aws_profile: str | None = None) -> S3Credentials | None:
    """
    Generates AWS credentials created by boto3 when starting a session with given profile.
    :param aws_profile: Name of profile configured in ~/.aws directory
    :return: An S3Credentials object with access key pair and session token if applicable.
    """
    aws_session: boto3.Session | None
    if aws_profile is not None:
        aws_session = boto3.Session(profile_name=aws_profile)
    else:
        aws_session = boto3.Session()
    credentials = aws_session.get_credentials()
    if credentials is None:
        return None

    return S3Credentials(
        access_key_id=credentials.access_key,
        secret_access_key=credentials.secret_key,
        session_token=credentials.token,
    )


def get_credential_env_vars(auth: AwsAuthentication) -> dict[str, str]:
    """
    Generates AWS credential environment variables for tasks.
    :param auth: AwsAuthentication
    :return: A dictionary containing an access key-pair and optionally, a session token; or an empty
    dictionary if the AWS-credential environment-variables should've been set already.
    :raise: ValueError if `auth.type` is not a supported type or fails to authenticate with the
    given `auth`.
    """
    env_vars: dict[str, str] | None
    aws_credentials: S3Credentials | None

    if AwsAuthType.env_vars == auth.type:
        # Environmental variables are already set
        return {}

    if AwsAuthType.credentials == auth.type:
        aws_credentials = auth.credentials

    elif AwsAuthType.profile == auth.type:
        aws_credentials = _get_session_credentials(auth.profile)
        if aws_credentials is None:
            raise ValueError(f"Failed to authenticate with profile {auth.profile}")

    elif AwsAuthType.default == auth.type:
        aws_credentials = _get_session_credentials()
        if aws_credentials is None:
            raise ValueError("Failed to authenticate with the default credential provider chain.")
    else:
        raise ValueError(f"Unsupported authentication type: {auth.type}")

    env_vars = {
        AWS_ENV_VAR_ACCESS_KEY_ID: aws_credentials.access_key_id,
        AWS_ENV_VAR_SECRET_ACCESS_KEY: aws_credentials.secret_access_key,
    }
    aws_session_token = aws_credentials.session_token
    if aws_session_token is not None:
        env_vars[AWS_ENV_VAR_SESSION_TOKEN] = aws_session_token
    return env_vars


def generate_container_auth_options(
    clp_config: ClpConfig, container_type: str
) -> tuple[bool, list[str]]:
    """
    Generates Docker container authentication options for AWS S3 access based on the given type.
    Handles authentication methods that require extra configuration (profile, env_vars).

    :param clp_config: ClpConfig containing storage configurations.
    :param container_type: Type of the calling container.
    :return: Tuple of (whether aws config mount is needed, credential env_vars to set).
    :raises: ValueError if environment variables are not set correctly.
    """
    output_storages_by_component_type: list[S3Storage | FsStorage]
    input_storage_needed = False

    if container_type in (
        COMPRESSION_SCHEDULER_COMPONENT_NAME,
        COMPRESSION_WORKER_COMPONENT_NAME,
    ):
        output_storages_by_component_type = [clp_config.archive_output.storage]
        input_storage_needed = True
    elif container_type in (ARCHIVE_MANAGER_ACTION_NAME,):
        output_storages_by_component_type = [clp_config.archive_output.storage]
    elif container_type in (WEBUI_COMPONENT_NAME,):
        output_storages_by_component_type = [clp_config.stream_output.storage]
    elif container_type in (
        GARBAGE_COLLECTOR_COMPONENT_NAME,
        QUERY_SCHEDULER_COMPONENT_NAME,
        QUERY_WORKER_COMPONENT_NAME,
    ):
        output_storages_by_component_type = [
            clp_config.archive_output.storage,
            clp_config.stream_output.storage,
        ]
    else:
        raise ValueError(f"Container type {container_type} is not valid.")
    config_mount = False
    add_env_vars = False

    for storage in output_storages_by_component_type:
        if StorageType.S3 == storage.type:
            auth = storage.s3_config.aws_authentication
            if AwsAuthType.profile == auth.type:
                config_mount = True
            elif AwsAuthType.env_vars == auth.type:
                add_env_vars = True

    if input_storage_needed and StorageType.S3 == clp_config.logs_input.type:
        auth = clp_config.logs_input.aws_authentication
        if AwsAuthType.profile == auth.type:
            config_mount = True
        elif AwsAuthType.env_vars == auth.type:
            add_env_vars = True

    credentials_env_vars = []

    if add_env_vars:
        access_key_id = os.getenv(AWS_ENV_VAR_ACCESS_KEY_ID)
        secret_access_key = os.getenv(AWS_ENV_VAR_SECRET_ACCESS_KEY)
        if access_key_id and secret_access_key:
            credentials_env_vars.extend(
                (
                    f"{AWS_ENV_VAR_ACCESS_KEY_ID}={access_key_id}",
                    f"{AWS_ENV_VAR_SECRET_ACCESS_KEY}={secret_access_key}",
                )
            )
        else:
            raise ValueError(
                f"{AWS_ENV_VAR_ACCESS_KEY_ID} and {AWS_ENV_VAR_SECRET_ACCESS_KEY} "
                "environment variables not set"
            )
        if os.getenv(AWS_ENV_VAR_SESSION_TOKEN):
            raise ValueError(
                f"{AWS_ENV_VAR_SESSION_TOKEN} not supported for environmental variable credentials."
            )

    return config_mount, credentials_env_vars


def _create_s3_client(
    endpoint_url: str | None,
    region_code: str | None,
    s3_auth: AwsAuthentication,
    boto3_config: Config | None = None,
) -> boto3.client:
    aws_session: boto3.Session | None
    if AwsAuthType.profile == s3_auth.type:
        aws_session = boto3.Session(
            profile_name=s3_auth.profile,
            region_name=region_code,
        )
    elif AwsAuthType.credentials == s3_auth.type:
        credentials = s3_auth.credentials
        aws_session = boto3.Session(
            aws_access_key_id=credentials.access_key_id,
            aws_secret_access_key=credentials.secret_access_key,
            region_name=region_code,
            aws_session_token=credentials.session_token,
        )
    elif AwsAuthType.env_vars == s3_auth.type or AwsAuthType.default == s3_auth.type:
        # Use default session which will use environment variables or instance role
        aws_session = boto3.Session(region_name=region_code)
    else:
        raise ValueError(f"Unsupported authentication type: {s3_auth.type}")

    s3_client = aws_session.client("s3", endpoint_url=endpoint_url, config=boto3_config)
    return s3_client


def parse_s3_url(s3_url: str) -> tuple[str | None, str | None, str, str]:
    """
    Parses the endpoint_url, region_code, bucket, and key_prefix from the given S3 URL.
    :param s3_url: A host-style URL or path-style URL.
    :return: A tuple of (endpoint_url, region_code, bucket, key_prefix). A value of None is returned
    if the endpoint_url originates from AWS.
    :raise: ValueError if `s3_url` is not a valid host-style URL or path-style URL.
    """
    host_style_url_regex = re.compile(
        rf"{SCHEME_REGEXP}://{BUCKET_NAME_REGEXP}\."
        rf"{S3_PREFIX_REGEXP}\.({REGION_CODE_REGEXP}\.)?"
        rf"{ENDPOINT_REGEXP}/{KEY_PREFIX_REGEXP}.*"
    )
    match = host_style_url_regex.match(s3_url)

    if match is None:
        path_style_url_regex = re.compile(
            rf"{SCHEME_REGEXP}://({S3_PREFIX_REGEXP}\."
            rf"({REGION_CODE_REGEXP}\.)?)?{ENDPOINT_REGEXP}"
            rf"/{BUCKET_NAME_REGEXP}/{KEY_PREFIX_REGEXP}.*"
        )
        match = path_style_url_regex.match(s3_url)

    if match is None:
        raise ValueError(f"Unsupported URL format: {s3_url}")

    scheme = match.group("scheme")
    endpoint = match.group("endpoint")
    region_code = match.group("region_code")
    bucket_name = match.group("bucket_name")

    s3_prefix = "s3." if match.group("s3") is not None else ""
    endpoint_url = f"{scheme}://{s3_prefix}{endpoint}" if endpoint != AWS_ENDPOINT else None
    key_prefix = match.group("key_prefix")

    return endpoint_url, region_code, bucket_name, key_prefix


def generate_s3_url(
    endpoint_url: str | None, region_code: str | None, bucket_name: str, object_key: str
) -> str:
    if not bool(bucket_name):
        raise ValueError("Bucket name is not specified")
    if not bool(object_key):
        raise ValueError("Object key is not specified")

    if endpoint_url is None:
        if region_code is None:
            return f"https://{bucket_name}.s3.{AWS_ENDPOINT}/{object_key}"
        return f"https://{bucket_name}.s3.{region_code}.{AWS_ENDPOINT}/{object_key}"

    endpoint_url_regex = re.compile(
        rf"{SCHEME_REGEXP}://({S3_PREFIX_REGEXP}\.)?{ENDPOINT_REGEXP}/?$"
    )
    match = endpoint_url_regex.match(endpoint_url)
    if match is None:
        raise ValueError(f"Unsupported endpoint URL format: {endpoint_url}")

    s3_prefix = "s3." if match.group("s3") is not None else ""
    scheme = match.group("scheme")
    endpoint = match.group("endpoint")

    if region_code is None:
        return f"{scheme}://{s3_prefix}{endpoint}/{bucket_name}/{object_key}"

    return f"{scheme}://{s3_prefix}{region_code}.{endpoint}/{bucket_name}/{object_key}"


def s3_get_object_metadata(s3_input_config: S3InputConfig) -> list[FileMetadata]:
    """
    Gets the metadata of all objects specified by the given input config.

    NOTE: We reuse FileMetadata to store the metadata of S3 objects where the object's key is stored
    as `path` in FileMetadata.

    :param s3_input_config:
    :return: A list of `FileMetadata` containing the object's metadata on success.
    :raise: Propagates `_create_s3_client`'s exceptions.
    :raise: Propagates `_s3_get_object_metadata_from_single_prefix`'s exceptions.
    :raise: Propagates `_s3_get_object_metadata_from_keys`'s exceptions.
    """
    s3_client = _create_s3_client(
        s3_input_config.endpoint_url,
        s3_input_config.region_code,
        s3_input_config.aws_authentication,
    )

    if s3_input_config.keys is None:
        return _s3_get_object_metadata_from_single_prefix(
            s3_client, s3_input_config.bucket, s3_input_config.key_prefix
        )

    return _s3_get_object_metadata_from_keys(
        s3_client, s3_input_config.bucket, s3_input_config.key_prefix, s3_input_config.keys
    )


def s3_put(s3_config: S3Config, src_file: Path, dest_path: str) -> None:
    """
    Uploads a local file to an S3 bucket using AWS's PutObject operation.

    :param s3_config: S3 configuration specifying the upload destination and credentials.
    :param src_file: Local file to upload.
    :param dest_path: The destination path for the uploaded file in the S3 bucket, relative to
    `s3_config.key_prefix` (the file's S3 key will be `s3_config.key_prefix` + `dest_path`).
    :raises: ValueError if `src_file` doesn't exist, doesn't resolve to a file or is larger than the
    S3 PutObject limit.
    :raises: Propagates `boto3.client`'s exceptions.
    :raises: Propagates `boto3.client.put_object`'s exceptions.
    """
    if not src_file.exists():
        raise ValueError(f"{src_file} doesn't exist")
    if not src_file.is_file():
        raise ValueError(f"{src_file} is not a file")
    if src_file.stat().st_size > 5 * 1024 * 1024 * 1024:
        raise ValueError(
            f"{src_file} is larger than the limit (5GiB) for a single PutObject operation."
        )

    boto3_config = Config(retries=dict(total_max_attempts=3, mode="adaptive"))
    s3_client = _create_s3_client(
        s3_config.endpoint_url, s3_config.region_code, s3_config.aws_authentication, boto3_config
    )

    with open(src_file, "rb") as file_data:
        s3_client.put_object(
            Bucket=s3_config.bucket, Body=file_data, Key=s3_config.key_prefix + dest_path
        )


def s3_delete_by_key_prefix(
    endpoint_url: str | None,
    region_code: str | None,
    bucket_name: str,
    key_prefix: str,
    s3_auth: AwsAuthentication,
) -> None:
    """
    Deletes all objects under the S3 path `bucket_name`/`key_prefix`.

    :param endpoint_url:
    :param region_code:
    :param bucket_name:
    :param key_prefix:
    :param s3_auth:
    :raises: ValueError if any parameter is invalid.
    :raises: Propagates `boto3.client.delete_objects`'s exceptions.
    """
    if not bool(bucket_name):
        raise ValueError("Bucket name is not specified")
    if not bool(key_prefix):
        raise ValueError("Key prefix is not specified")

    boto3_config = Config(retries=dict(total_max_attempts=3, mode="adaptive"))
    s3_client = _create_s3_client(endpoint_url, region_code, s3_auth, boto3_config)

    paginator = s3_client.get_paginator("list_objects_v2")
    for page in paginator.paginate(
        Bucket=bucket_name,
        Prefix=key_prefix,
        PaginationConfig={"PageSize": S3_OBJECT_DELETION_BATCH_SIZE_MAX},
    ):
        contents = page.get("Contents", None)
        if contents is None:
            continue

        deletion_config = {"Objects": [{"Key": obj["Key"]} for obj in contents]}
        s3_client.delete_objects(Bucket=bucket_name, Delete=deletion_config)


def s3_delete_objects(s3_config: S3Config, object_keys: set[str]) -> None:
    """
    Deletes objects from an S3 bucket. The objects are identified by keys relative to
    `s3_config.key_prefix`.

    Note: The AWS S3 `DeleteObjects` API, used by `boto3.client.delete_objects`, supports a maximum
    of 1,000 objects per request. This method splits the provided keys into batches of up to 1,000
    and issues multiple delete requests until all objects are removed.

    :param s3_config: The S3 config specifying the credentials and the bucket to perform deletion.
    :param object_keys: The set of object keys to delete, relative to `s3_config.key_prefix`.
    :raises: Propagates `boto3.client`'s exceptions.
    :raises: Propagates `boto3.client.delete_object`'s exceptions.
    """
    boto3_config = Config(retries=dict(total_max_attempts=3, mode="adaptive"))
    s3_client = _create_s3_client(
        s3_config.endpoint_url, s3_config.region_code, s3_config.aws_authentication, boto3_config
    )

    def _gen_deletion_config(objects_list: list[str]):
        return {"Objects": [{"Key": object_to_delete} for object_to_delete in objects_list]}

    objects_to_delete: list[str] = []
    for relative_obj_key in object_keys:
        objects_to_delete.append(s3_config.key_prefix + relative_obj_key)
        if len(objects_to_delete) < S3_OBJECT_DELETION_BATCH_SIZE_MAX:
            continue

        s3_client.delete_objects(
            Bucket=s3_config.bucket,
            Delete=_gen_deletion_config(objects_to_delete),
        )
        objects_to_delete = []

    if len(objects_to_delete) != 0:
        s3_client.delete_objects(
            Bucket=s3_config.bucket,
            Delete=_gen_deletion_config(objects_to_delete),
        )


def _s3_get_object_metadata_from_single_prefix(
    s3_client: boto3.client, bucket: str, key_prefix: str
) -> list[FileMetadata]:
    """
    Gets the metadata of all objects under the <`bucket`>/<`key_prefix`>.

    :param s3_client:
    :param bucket:
    :param key_prefix:
    :return: A list of `FileMetadata` containing the object's metadata on success.
    :raise: Propagates `_iter_s3_objects`'s exceptions.
    """
    file_metadata_list: list[FileMetadata] = list()
    for object_key, object_size in _iter_s3_objects(s3_client, bucket, key_prefix):
        file_metadata_list.append(FileMetadata(Path(object_key), object_size))

    return file_metadata_list


def _s3_get_object_metadata_from_keys(
    s3_client: boto3.client, bucket: str, key_prefix: str, keys: list[str]
) -> list[FileMetadata]:
    """
    Gets the metadata of all objects specified in `keys` under the <`bucket`>.

    :param s3_client:
    :param bucket:
    :param key_prefix:
    :param keys:
    :return: A list of `FileMetadata` containing the object's metadata on success.
    :raise: ValueError if `keys` is an empty list.
    :raise: ValueError if any key in `keys` doesn't start with `key_prefix`.
    :raise: ValueError if duplicate keys are found in `keys`.
    :raise: ValueError if any key in `keys` ends with `/`.
    :raise: ValueError if any key in `keys` doesn't exist in the bucket.
    :raise: Propagates `_s3_get_object_metadata_from_key`'s exceptions.
    :raise: Propagates `_iter_s3_objects`'s exceptions.
    """
    # Key validation
    if len(keys) == 0:
        raise ValueError("The list of keys is empty.")

    keys = sorted(keys)
    for idx, key in enumerate(keys):
        if not key.startswith(key_prefix):
            raise ValueError(f"Key `{key}` doesn't start with the specified prefix `{key_prefix}`.")
        if idx > 0 and key == keys[idx - 1]:
            raise ValueError(f"Duplicate key found: `{key}`.")
        if key.endswith("/"):
            raise ValueError(f"Key `{key}` is invalid: S3 object keys must not end with `/`.")

    key_iterator = iter(keys)
    first_key = next(key_iterator)
    file_metadata_list: list[FileMetadata] = []
    file_metadata_list.append(_s3_get_object_metadata_from_key(s3_client, bucket, first_key))

    next_key = next(key_iterator, None)
    if next_key is None:
        return file_metadata_list

    for object_key, object_size in _iter_s3_objects(s3_client, bucket, key_prefix, first_key):
        # We need to do both < and > checks since they are handled differently. Ideally, we can do
        # it with a single comparison. However, Python doesn't support three-way comparison.
        if object_key < next_key:
            continue
        if object_key > next_key:
            raise ValueError(f"Key `{next_key}` doesn't exist in the bucket `{bucket}`.")

        file_metadata_list.append(FileMetadata(Path(object_key), object_size))
        next_key = next(key_iterator, None)
        if next_key is None:
            # Early exit since all keys have been found.
            return file_metadata_list

    # If control flow reaches here, it means there are still keys left to find.
    absent_keys = []
    while next_key is not None:
        absent_keys.append(next_key)
        next_key = next(key_iterator, None)
    serialized_absent_keys = "\n".join(absent_keys)
    raise ValueError(
        f"Cannot find following keys in the bucket `{bucket}`:\n{serialized_absent_keys}"
    )


def _s3_get_object_metadata_from_key(
    s3_client: boto3.client, bucket: str, key: str
) -> FileMetadata:
    """
    Gets the metadata of an object specified by the `key` under the <`bucket`>.

    :param s3_client:
    :param bucket:
    :param key:
    :return: A `FileMetadata` containing the object's metadata on success.
    :raise: ValueError if the object doesn't exist or fails to read the metadata.
    :raise: Propagates `boto3.client.head_object`'s exceptions.
    """
    try:
        return FileMetadata(
            Path(key), s3_client.head_object(Bucket=bucket, Key=key)["ContentLength"]
        )
    except botocore.exceptions.ClientError as e:
        raise ValueError(
            f"Failed to read metadata of the key `{key}` from the bucket `{bucket}`"
            f" with the error: {e}."
        ) from e


def _iter_s3_objects(
    s3_client: boto3.client, bucket: str, key_prefix: str, start_from: str | None = None
) -> Generator[tuple[str, int], None, None]:
    """
    Iterates over objects in an S3 bucket under the specified prefix, optionally starting after a
    given key.

    NOTE: Any object key that resolves to a directory-like path (i.e., ends with `/`) will be
    skipped.

    :param s3_client:
    :param bucket:
    :param key_prefix:
    :param start_from: Optional key to start listing after.
    :yield: The next object to iterator, presenting as a tuple that contains:
        - The key of the object.
        - The size of the object.
    :raise: Propagates `boto3.client.get_paginator`'s exceptions.
    :raise: Propagates `boto3.paginator`'s exceptions.
    """
    paginator = s3_client.get_paginator("list_objects_v2")
    paginator_args = {"Bucket": bucket, "Prefix": key_prefix}
    if start_from is not None:
        paginator_args["StartAfter"] = start_from
    pages = paginator.paginate(**paginator_args)
    for page in pages:
        contents = page.get("Contents", None)
        if contents is None:
            continue
        for obj in contents:
            object_key = obj["Key"]
            if object_key.endswith("/"):
                # Skip any object that resolves to a directory-like path
                continue
            object_size = obj["Size"]
            yield object_key, object_size
