import os
import re
from pathlib import Path
from typing import Dict, List, Optional, Tuple, Union

import boto3
from botocore.config import Config
from job_orchestration.scheduler.job_config import S3InputConfig

from clp_py_utils.clp_config import (
    AwsAuthentication,
    AwsAuthType,
    CLPConfig,
    COMPRESSION_SCHEDULER_COMPONENT_NAME,
    COMPRESSION_WORKER_COMPONENT_NAME,
    FsStorage,
    QUERY_SCHEDULER_COMPONENT_NAME,
    QUERY_WORKER_COMPONENT_NAME,
    S3Config,
    S3Credentials,
    S3Storage,
    StorageType,
    WEBUI_COMPONENT_NAME,
)
from clp_py_utils.compression import FileMetadata

# Constants
AWS_ENDPOINT = "amazonaws.com"
AWS_ENV_VAR_ACCESS_KEY_ID = "AWS_ACCESS_KEY_ID"
AWS_ENV_VAR_SECRET_ACCESS_KEY = "AWS_SECRET_ACCESS_KEY"
AWS_ENV_VAR_SESSION_TOKEN = "AWS_SESSION_TOKEN"


def _get_session_credentials(aws_profile: Optional[str] = None) -> Optional[S3Credentials]:
    """
    Generates AWS credentials created by boto3 when starting a session with given profile.
    :param aws_profile: Name of profile configured in ~/.aws directory
    :return: An S3Credentials object with access key pair and session token if applicable.
    """
    aws_session: Optional[boto3.Session] = None
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


def get_credential_env_vars(auth: AwsAuthentication) -> Dict[str, str]:
    """
    Generates AWS credential environment variables for tasks.
    :param auth: AwsAuthentication
    :return: A dictionary containing an access key-pair and optionally, a session token; or an empty
    dictionary if the AWS-credential environment-variables should've been set already.
    :raise: ValueError if `auth.type` is not a supported type or fails to authenticate with the
    given `auth`.
    """
    env_vars: Optional[Dict[str, str]] = None
    aws_credentials: Optional[S3Credentials] = None

    if AwsAuthType.env_vars == auth.type:
        # Environmental variables are already set
        return {}

    elif AwsAuthType.credentials == auth.type:
        aws_credentials = auth.credentials

    elif AwsAuthType.profile == auth.type:
        aws_credentials = _get_session_credentials(auth.profile)
        if aws_credentials is None:
            raise ValueError(f"Failed to authenticate with profile {auth.profile}")

    elif AwsAuthType.ec2 == auth.type:
        aws_credentials = _get_session_credentials()
        if aws_credentials is None:
            raise ValueError(f"Failed to authenticate with EC2 metadata.")
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
    clp_config: CLPConfig, component_type: str
) -> Tuple[bool, List[str]]:
    """
    Generates Docker container authentication options for AWS S3 access based on the given type.
    Handles authentication methods that require extra configuration (profile, env_vars).

    :param clp_config: CLPConfig containing storage configurations.
    :param component_type: Type of calling container (compression, log_viewer, or query).
    :return: Tuple of (whether aws config mount is needed, credential env_vars to set).
    :raises: ValueError if environment variables are not set correctly.
    """
    output_storages_by_component_type: List[Union[S3Storage, FsStorage]] = []
    input_storage_needed = False

    if component_type in (
        COMPRESSION_SCHEDULER_COMPONENT_NAME,
        COMPRESSION_WORKER_COMPONENT_NAME,
    ):
        output_storages_by_component_type = [clp_config.archive_output.storage]
        input_storage_needed = True
    elif component_type in (WEBUI_COMPONENT_NAME,):
        output_storages_by_component_type = [clp_config.stream_output.storage]
    elif component_type in (
        QUERY_SCHEDULER_COMPONENT_NAME,
        QUERY_WORKER_COMPONENT_NAME,
    ):
        output_storages_by_component_type = [
            clp_config.archive_output.storage,
            clp_config.stream_output.storage,
        ]
    else:
        raise ValueError(f"Component type {component_type} is not valid.")
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

    return (config_mount, credentials_env_vars)


def _create_s3_client(s3_config: S3Config, boto3_config: Optional[Config] = None) -> boto3.client:
    auth = s3_config.aws_authentication
    aws_session: Optional[boto3.Session] = None

    if AwsAuthType.profile == auth.type:
        aws_session = boto3.Session(
            profile_name=auth.profile,
            region_name=s3_config.region_code,
        )
    elif AwsAuthType.credentials == auth.type:
        credentials = auth.credentials
        aws_session = boto3.Session(
            aws_access_key_id=credentials.access_key_id,
            aws_secret_access_key=credentials.secret_access_key,
            region_name=s3_config.region_code,
            aws_session_token=credentials.session_token,
        )
    elif AwsAuthType.env_vars == auth.type or AwsAuthType.ec2 == auth.type:
        # Use default session which will use environment variables or instance role
        aws_session = boto3.Session(region_name=s3_config.region_code)
    else:
        raise ValueError(f"Unsupported authentication type: {auth.type}")

    s3_client = aws_session.client("s3", config=boto3_config)
    return s3_client


def parse_s3_url(s3_url: str) -> Tuple[str, str, str]:
    """
    Parses the region_code, bucket, and key_prefix from the given S3 URL.
    :param s3_url: A host-style URL or path-style URL.
    :return: A tuple of (region_code, bucket, key_prefix).
    :raise: ValueError if `s3_url` is not a valid host-style URL or path-style URL.
    """

    host_style_url_regex = re.compile(
        r"https://(?P<bucket_name>[a-z0-9.-]+)\.s3(\.(?P<region_code>[a-z]+-[a-z]+-[0-9]))"
        r"\.(?P<endpoint>[a-z0-9.-]+)/(?P<key_prefix>[^?]+).*"
    )
    match = host_style_url_regex.match(s3_url)

    if match is None:
        path_style_url_regex = re.compile(
            r"https://s3(\.(?P<region_code>[a-z]+-[a-z]+-[0-9]))\.(?P<endpoint>[a-z0-9.-]+)/"
            r"(?P<bucket_name>[a-z0-9.-]+)/(?P<key_prefix>[^?]+).*"
        )
        match = path_style_url_regex.match(s3_url)

    if match is None:
        raise ValueError(f"Unsupported URL format: {s3_url}")

    region_code = match.group("region_code")
    bucket_name = match.group("bucket_name")
    endpoint = match.group("endpoint")
    key_prefix = match.group("key_prefix")

    if AWS_ENDPOINT != endpoint:
        raise ValueError(f"Unsupported endpoint: {endpoint}")

    return region_code, bucket_name, key_prefix


def generate_s3_virtual_hosted_style_url(
    region_code: str, bucket_name: str, object_key: str
) -> str:
    if not bool(region_code):
        raise ValueError("Region code is not specified")
    if not bool(bucket_name):
        raise ValueError("Bucket name is not specified")
    if not bool(object_key):
        raise ValueError("Object key is not specified")

    return f"https://{bucket_name}.s3.{region_code}.{AWS_ENDPOINT}/{object_key}"


def s3_get_object_metadata(s3_input_config: S3InputConfig) -> List[FileMetadata]:
    """
    Gets the metadata of all objects under the <bucket>/<key_prefix> specified by s3_input_config.
    NOTE: We reuse FileMetadata to store the metadata of S3 objects where the object's key is stored
    as `path` in FileMetadata.

    :param s3_input_config:
    :return: List[FileMetadata] containing the object's metadata on success,
    :raises: Propagates `boto3.client`'s exceptions.
    :raises: Propagates `boto3.client.get_paginator`'s exceptions.
    :raises: Propagates `boto3.paginator`'s exceptions.
    """

    s3_client = _create_s3_client(s3_input_config)

    file_metadata_list: List[FileMetadata] = list()
    paginator = s3_client.get_paginator("list_objects_v2")
    pages = paginator.paginate(Bucket=s3_input_config.bucket, Prefix=s3_input_config.key_prefix)
    for page in pages:
        contents = page.get("Contents", None)
        if contents is None:
            continue

        for obj in contents:
            object_key = obj["Key"]
            if object_key.endswith("/"):
                # Skip any object that resolves to a directory-like path
                continue

            file_metadata_list.append(FileMetadata(Path(object_key), obj["Size"]))

    return file_metadata_list


def s3_put(s3_config: S3Config, src_file: Path, dest_file_name: str) -> None:
    """
    Uploads a local file to an S3 bucket using AWS's PutObject operation.
    :param s3_config: S3 configuration specifying the upload destination and credentials.
    :param src_file: Local file to upload.
    :param dest_file_name: The name for the uploaded file in the S3 bucket.
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
    s3_client = _create_s3_client(s3_config, boto3_config)

    with open(src_file, "rb") as file_data:
        s3_client.put_object(
            Bucket=s3_config.bucket, Body=file_data, Key=s3_config.key_prefix + dest_file_name
        )
