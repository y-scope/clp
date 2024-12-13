from pathlib import Path

import boto3
from botocore.config import Config
from botocore.exceptions import ClientError

from clp_py_utils.clp_config import S3Config
from clp_py_utils.result import Result


def s3_put(
    s3_config: S3Config, src_file: Path, dest_file_name: str, total_max_attempts: int = 3
) -> Result:

    if not src_file.exists():
        return Result(success=False, error=f"{src_file} doesn't exist")
    if not src_file.is_file():
        return Result(success=False, error=f"{src_file} is not a file")

    s3_client_args = {
        "region_name": s3_config.region_name,
        "aws_access_key_id": s3_config.access_key_id,
        "aws_secret_access_key": s3_config.secret_access_key,
    }

    config = Config(retries=dict(total_max_attempts=total_max_attempts, mode="adaptive"))

    my_s3_client = boto3.client("s3", **s3_client_args, config=config)

    with open(src_file, "rb") as file_data:
        try:
            my_s3_client.put_object(
                Bucket=s3_config.bucket, Body=file_data, Key=s3_config.key_prefix + dest_file_name
            )
        except ClientError as e:
            error_code = e.response["Error"]["Code"]
            error_message = e.response["Error"]["Message"]
            return Result(success=False, error=f"ClientError: {error_code} - {error_message}")
        except Exception as e:
            return Result(success=False, error=f"An unexpected error occurred: {e}")

    return Result(success=True)
