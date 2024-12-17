from pathlib import Path

import boto3
from botocore.config import Config
from botocore.exceptions import ClientError
from result import Err, Ok, Result

from clp_py_utils.clp_config import S3Config


def s3_put(
    s3_config: S3Config, src_file: Path, dest_file_name: str, total_max_attempts: int = 3
) -> Result[bool, str]:
    """
    Upload a local file to S3 bucket using AWS's PutObject operation.
    :param s3_config: S3 configuration specifies the upload destination and credentials.
    :param src_file: local file to upload.
    :param dest_file_name: The name for uploaded file in the S3 bucket.
    :param total_max_attempts: Maximum number of retry attempts for the upload. Defaults to 3.
    :return: Result.OK(bool) on success.
             Result.Err(str) with the error message if put operation fails.
    """
    if not src_file.exists():
        return Err(f"{src_file} doesn't exist")
    if not src_file.is_file():
        return Err(f"{src_file} is not a file")
    if src_file.stat().st_size > 5 * 1024 * 1024 * 1024:
        return Err("File size exceeds 5GB limit for single put_object operation")

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
