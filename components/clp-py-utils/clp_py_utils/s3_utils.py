import boto3
from botocore.exceptions import ClientError

from clp_py_utils.clp_config import S3Config
from clp_py_utils.result import Result


def verify_s3_config_for_archive_output(s3_config: S3Config) -> Result:
    # TODO: need to verify:
    # 1. Have write priveldge so archive can be compressed
    # 2. Have read priviledge so archive can be readed
    # 3. bucket and region are the same, this should run into issue but not sure
    return Result(success=True)
