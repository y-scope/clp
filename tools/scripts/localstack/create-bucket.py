#!/usr/bin/env -S uv run --script
# /// script
# dependencies = [
#   "boto3==1.40.70",
# ]
# ///
"""Script to create an S3 bucket with an optional SQS queue listening to the bucket."""

# To allow using try-except-pass pattern to detect whether resources are already created.
# ruff: noqa: TRY300, BLE001

# Silence `too-many-return-statements` for `main`.
# ruff: noqa: PLR0911

import argparse
import json
import logging
import sys
from typing import Any

import boto3

_ACCESS_KEY_ID = "test"
# To allow using hardcoded password
# ruff: noqa: S105
_SECRET_ACCESS_KEY = "test"
_REGION: str = "us-east-1"

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(message)s",
    datefmt="%Y-%m-%d %H:%M:%S",
)
logger = logging.getLogger(__name__)


def _bucket_already_exist(s3_client: Any, bucket_name: str) -> bool:
    """
    Checks whether the S3 bucket already exists.

    :param s3_client: Boto3 S3 client.
    :param bucket_name: The S3 bucket name.
    :return: Whether `bucket_name` already exists.
    """
    try:
        _ = s3_client.head_bucket(Bucket=bucket_name)
        return True
    except Exception as _:
        return False


def _queue_already_exist(sqs_client: Any, queue_name: str) -> bool:
    """
    Checks whether the SQS queue already exists.

    :param sqs_client: Boto3 SQS client.
    :param queue_name: The SQS queue name.
    :return: Whether `queue_name` already exists.
    """
    try:
        _ = sqs_client.get_queue_url(QueueName=queue_name)
        return True
    except Exception as _:
        return False


def _listen_to_bucket(
    s3_client: Any,
    sqs_client: Any,
    bucket_name: str,
    queue_url: str,
    queue_arn: str,
) -> bool:
    """
    Sets SQS queue to listen to all S3 bucket events.

    :param s3_client: Boto3 S3 client.
    :param sqs_client: Boto3 SQS client.
    :param bucket_name: The S3 bucket name.
    :param queue_url: The SQS queue URL.
    :param queue_arn: The SQS queue ARN.
    :return: Whether the operation was successful.
    """
    try:
        sqs_policy = {
            "Version": "2012-10-17",
            "Statement": [
                {
                    "Sid": "AllowS3SendMessage",
                    "Effect": "Allow",
                    "Principal": {"Service": "s3.amazonaws.com"},
                    "Action": "sqs:SendMessage",
                    "Resource": queue_arn,
                }
            ],
        }
        sqs_client.set_queue_attributes(
            QueueUrl=queue_url, Attributes={"Policy": json.dumps(sqs_policy)}
        )
    except Exception as _:
        logger.exception("Failed to set SQS policy.")
        return False

    try:
        s3_notification_config = {
            "QueueConfigurations": [
                {
                    "QueueArn": queue_arn,
                    "Events": [
                        "s3:ObjectCreated:*",
                        "s3:ObjectRemoved:*",
                        "s3:ObjectRestore:*",
                        "s3:Replication:*",
                    ],
                }
            ]
        }
        s3_client.put_bucket_notification_configuration(
            Bucket=bucket_name, NotificationConfiguration=s3_notification_config
        )
    except Exception as _:
        logger.exception("Failed to set S3 notification config.")
        return False

    return True


def main() -> int:
    """Main"""
    parser = argparse.ArgumentParser(
        description="Create an S3 bucket with an optional SQS queue listening to the bucket."
    )
    parser.add_argument(
        "--port",
        type=int,
        default=4566,
        help="The port LocalStack is hosting on (default: %(default)d)",
    )
    parser.add_argument(
        "--bucket",
        type=str,
        help="The name of S3 bucket to create",
    )
    parser.add_argument(
        "--queue",
        type=str,
        required=False,
        help="The name of the optional SQS queue to create",
    )
    args = parser.parse_args()

    localstack_endpoint = f"http://localhost:{args.port}"
    logger.info("Using LocalStack endpoint: %s", localstack_endpoint)

    session = boto3.session.Session(
        aws_access_key_id=_ACCESS_KEY_ID,
        aws_secret_access_key=_SECRET_ACCESS_KEY,
        region_name=_REGION,
    )
    s3_client = session.client("s3", endpoint_url=localstack_endpoint)
    sqs_client = session.client("sqs", endpoint_url=localstack_endpoint)

    # Check whether the resources have already been created
    if _bucket_already_exist(s3_client, args.bucket):
        logger.warning("Bucket '%s' already exists.", args.bucket)
        return 1

    if args.queue is not None and _queue_already_exist(sqs_client, args.queue):
        logger.warning("Queue '%s' already exists.", args.queue)
        return 1

    # Create S3 bucket
    try:
        s3_client.create_bucket(Bucket=args.bucket)
    except Exception as _:
        logger.exception("Failed to create S3 bucket.")
        return 1
    logger.info("S3 bucket '%s' created successfully.", args.bucket)

    if args.queue is None:
        return 0

    # Create SQS queue
    queue_url: str
    queue_arn: str
    try:
        sqs_client.create_queue(QueueName=args.queue)
        queue_url = sqs_client.get_queue_url(QueueName=args.queue)["QueueUrl"]
        queue_arn = sqs_client.get_queue_attributes(
            QueueUrl=queue_url, AttributeNames=["QueueArn"]
        )["Attributes"]["QueueArn"]
    except Exception as _:
        logger.exception("Failed to create SQS queue.")
        return 1
    logger.info("SQS queue '%s' created successfully with arn '%s'.", args.queue, queue_arn)

    if not _listen_to_bucket(s3_client, sqs_client, args.bucket, queue_url, queue_arn):
        return 1

    logger.info(
        "S3 bucket '%s' configured to send events to SQS queue '%s'.", args.bucket, args.queue
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
