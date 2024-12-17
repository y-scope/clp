from pathlib import Path

import boto3
from botocore.config import Config
from botocore.exceptions import ClientError
from result import Err, Ok, Result

from typing import List

from clp_py_utils.clp_config import S3Config
from job_orchestration.scheduler.job_config import S3InputConfig
from clp_py_utils.compression import FileMetadata

def verify_s3_ingestion_config(s3_input_config: S3InputConfig) -> Result[boto3.client, str]:
    # Consider actually returning the S3 client?
    s3_client = boto3.client(
        "s3",
        region_name=s3_input_config.region_code,
        aws_access_key_id=s3_input_config.aws_access_key_id,
        aws_secret_access_key=s3_input_config.aws_secret_access_key,
    )
    return Ok(s3_client)

def get_file_metadata_with_bucket_prefix(
    s3_client: boto3.client,
    s3_config: S3InputConfig
) -> List[FileMetadata]:
    file_metadata_list: List[FileMetadata] = list()
    paginator = s3_client.get_paginator("list_objects_v2")
    pages = paginator.paginate(Bucket=s3_config.bucket, Prefix=s3_config.key_prefix)

    for page in pages:
        contents = page.get("Contents", None)
        if contents:
            for obj in contents:
                # we skip empty directory in s3.
                object_key = obj["Key"]
                if object_key.endswith("/"):
                    continue
                # Note, don't resolve this path
                # s3_path = Path(bucket_name + "/" + object_key)
                file_metadata_list.append(FileMetadata(Path(object_key), obj["Size"]))

    return file_metadata_list

def s3_put(
    s3_config: S3Config, src_file: Path, dest_file_name: str, total_max_attempts: int = 3
) -> Result[bool, str]:
    """
    Uploads a local file to an S3 bucket using AWS's PutObject operation.
    :param s3_config: S3 configuration specifying the upload destination and credentials.
    :param src_file: Local file to upload.
    :param dest_file_name: The name for the uploaded file in the S3 bucket.
    :param total_max_attempts: Maximum number of retry attempts for the upload.
    :return: Result.OK(bool) on success, or Result.Err(str) with the error message otherwise.
    """
    if not src_file.exists():
        return Err(f"{src_file} doesn't exist")
    if not src_file.is_file():
        return Err(f"{src_file} is not a file")
    if src_file.stat().st_size > 5 * 1024 * 1024 * 1024:
        return Err(f"{src_file} is larger than the limit (5GiB) for a single PutObject operation.")

    config = Config(retries=dict(total_max_attempts=total_max_attempts, mode="adaptive"))

    my_s3_client = boto3.client(
        "s3",
        region_name=s3_config.region_code,
        aws_access_key_id=s3_config.access_key_id,
        aws_secret_access_key=s3_config.secret_access_key,
        config=config,
    )

    with open(src_file, "rb") as file_data:
        try:
            my_s3_client.put_object(
                Bucket=s3_config.bucket, Body=file_data, Key=s3_config.key_prefix + dest_file_name
            )
        except ClientError as e:
            error_code = e.response["Error"]["Code"]
            error_message = e.response["Error"]["Message"]
            return Err(f"ClientError: {error_code} - {error_message}")
        except Exception as e:
            return Err(f"An unexpected error occurred: {e}")

    return Ok(True)
