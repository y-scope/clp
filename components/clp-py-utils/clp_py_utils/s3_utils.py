import os
from pathlib import Path
from typing import Dict, List, Literal, Optional, Tuple

import boto3
from botocore.config import Config
from job_orchestration.scheduler.job_config import S3InputConfig

from clp_py_utils.clp_config import CLPConfig, S3Config, S3Credentials, StorageType
from clp_py_utils.compression import FileMetadata

# Constants
AWS_ENDPOINT = "amazonaws.com"


def get_session_credentials(aws_profile: Optional[str]) -> Optional[S3Credentials]:
    """
    Generates AWS credentials created by boto3 when starting a session with given profile.
    :param aws_profile: Name of profile configured in ~/.aws directory
    :return: An S3Credentials object with access key pair and session token if applicable.
    """
    aws_session = None
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


def get_credential_env_vars(config: S3Config) -> Dict[str, str]:
    """
    Generates AWS credential environment variables for tasks.
    :param config: S3Config or S3InputConfig from which to retrieve credentials.
    :return: A [str, str] Dict which access key pair and session token if applicable.
    :raise: ValueError if auth type is not supported
    """
    env_vars: Dict[str, str] = None
    auth = config.aws_authentication

    if auth.type == "profile":
        aws_credentials: S3Credentials = get_session_credentials(auth.profile)
        if aws_credentials is None:
            raise ValueError(f"Failed to authenticate with profile {auth.profile}")

        env_vars = {
            "AWS_ACCESS_KEY_ID": aws_credentials.access_key_id,
            "AWS_SECRET_ACCESS_KEY": aws_credentials.secret_access_key,
        }
        aws_session_token = aws_credentials.session_token
        if aws_session_token is not None:
            env_vars["AWS_SESSION_TOKEN"] = aws_session_token
        return env_vars

    elif auth.type == "credentials":
        credentials = auth.credentials
        env_vars = {
            "AWS_ACCESS_KEY_ID": credentials.access_key_id,
            "AWS_SECRET_ACCESS_KEY": credentials.secret_access_key,
        }
        if credentials.session_token:
            env_vars["AWS_SESSION_TOKEN"] = credentials.session_token
        return env_vars

    elif auth.type == "env_vars":
        # Environment variables are already set in the process
        return

    elif auth.type == "ec2":
        # EC2 instance role will be used automatically
        return

    else:
        raise ValueError(f"Unsupported authentication type: {auth.type}")


def get_container_authentication(
    clp_config: CLPConfig, type: Literal["compression", "log_viewer", "query"]
) -> Tuple[bool, List[str]]:
    """
    Generates Docker container authentication options for AWS S3 access based on the given type.
    Handles authentication methods that require extra configuration (profile, env_vars).
    
    :param clp_config: CLPConfig containing storage configurations
    :param type: Type of calling container (compression, log_viewer, or query)
    :return: Tuple of (whether aws config mount is needed, credential env_vars to set)
    :raises: ValueError if environment variables are not set correctly or if type is invalid
    """
    if type not in ["compression", "log_viewer", "query"]:
        raise ValueError(f"Unsupported authentication type: {type}")
    
    storages = {
        "compression": [clp_config.logs_input, clp_config.archive_output],
        "log_viewer": [clp_config.stream_output],
        "query": [clp_config.logs_input, clp_config.archive_output, clp_config.stream_output],
    }

    config_mount = False
    credentials_env_vars = []

    for storage in storages[type]:
        if StorageType.S3 == storage.type:
            auth = storage.s3_config.aws_authentication
            if "profile" == auth.type and not config_mount:
                config_mount = True
            elif "env_vars" == auth.type and 0 == len(credentials_env_vars):
                access_key_id = os.getenv("AWS_ACCESS_KEY_ID")
                secret_access_key = os.getenv("AWS_SECRET_ACCESS_KEY")
                if access_key_id and secret_access_key:
                    credentials_env_vars.extend(
                        (
                            f"AWS_ACCESS_KEY_ID={access_key_id}",
                            f"AWS_SECRET_ACCESS_KEY={secret_access_key}",
                        )
                    )
                else:
                    raise ValueError(
                        "AWS_ACCESS_KEY_ID and AWS_SECRET_ACCESS_KEY environment variables not set"
                    )
                if os.getenv("AWS_SESSION_TOKEN"):
                    raise ValueError(
                        "AWS_SESSION_TOKEN not supported for environmental variable credentials."
                    )

    return (config_mount, credentials_env_vars)


def _create_s3_client(s3_config: S3Config) -> boto3.client:
    config = Config(retries=dict(total_max_attempts=3, mode="adaptive"))
    auth = s3_config.aws_authentication
    aws_session = None

    if auth.type == "profile":
        aws_session = boto3.Session(
            profile_name=auth.profile,
            region_name=s3_config.region_code,
        )
    elif auth.type == "credentials":
        credentials = auth.credentials
        aws_session = boto3.Session(
            aws_access_key_id=credentials.access_key_id,
            aws_secret_access_key=credentials.secret_access_key,
            region_name=s3_config.region_code,
            aws_session_token=credentials.session_token,
        )
    elif auth.type == "env_vars" or auth.type == "ec2":
        # Use default session which will use environment variables or instance role
        aws_session = boto3.Session(region_name=s3_config.region_code)
    else:
        raise ValueError(f"Unsupported authentication type: {auth.type}")

    s3_client = aws_session.client("s3", config=config)
    return s3_client


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

    s3_client = _create_s3_client(s3_config)

    with open(src_file, "rb") as file_data:
        s3_client.put_object(
            Bucket=s3_config.bucket, Body=file_data, Key=s3_config.key_prefix + dest_file_name
        )
