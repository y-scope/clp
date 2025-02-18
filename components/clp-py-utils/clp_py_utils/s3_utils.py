import re
from pathlib import Path
from typing import List, Tuple

import boto3
from botocore.config import Config
from job_orchestration.scheduler.job_config import S3InputConfig

from clp_py_utils.clp_config import S3Config
from clp_py_utils.compression import FileMetadata

# Constants
AWS_ENDPOINT = "amazonaws.com"


def parse_s3_url(s3_url: str) -> Tuple[str, str, str]:
    """
    Parses the region_code, bucket, and key_prefix from the given S3 URL.
    :param s3_url: A host-style URL or path-style URL.
    :return: A tuple of (region_code, bucket, key_prefix).
    :raise: ValueError if `s3_url` is not a valid host-style URL or path-style URL.
    """

    host_style_url_regex = re.compile(
        r"https://(?P<bucket_name>[a-z0-9.-]+)\.s3(\.(?P<region_code>[a-z0-9-]+))?"
        r"\.(?P<endpoint>[a-z0-9.-]+)/(?P<key_prefix>[^?]+).*"
    )
    match = host_style_url_regex.match(s3_url)

    if match is None:
        path_style_url_regex = re.compile(
            r"https://s3(\.(?P<region_code>[a-z0-9-]+))?\.(?P<endpoint>[a-z0-9.-]+)/"
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

    s3_client = boto3.client(
        "s3",
        region_name=s3_input_config.region_code,
        aws_access_key_id=s3_input_config.credentials.access_key_id,
        aws_secret_access_key=s3_input_config.credentials.secret_access_key,
    )

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


def s3_put(
    s3_config: S3Config, src_file: Path, dest_file_name: str, total_max_attempts: int = 3
) -> None:
    """
    Uploads a local file to an S3 bucket using AWS's PutObject operation.
    :param s3_config: S3 configuration specifying the upload destination and credentials.
    :param src_file: Local file to upload.
    :param dest_file_name: The name for the uploaded file in the S3 bucket.
    :param total_max_attempts: Maximum number of retry attempts for the upload.
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

    config = Config(retries=dict(total_max_attempts=total_max_attempts, mode="adaptive"))
    aws_access_key_id, aws_secret_access_key = s3_config.get_credentials()

    my_s3_client = boto3.client(
        "s3",
        region_name=s3_config.region_code,
        aws_access_key_id=aws_access_key_id,
        aws_secret_access_key=aws_secret_access_key,
        config=config,
    )

    with open(src_file, "rb") as file_data:
        my_s3_client.put_object(
            Bucket=s3_config.bucket, Body=file_data, Key=s3_config.key_prefix + dest_file_name
        )
