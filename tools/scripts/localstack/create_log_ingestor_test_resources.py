#!/usr/bin/env python3
# /// script
# dependencies = [
#   "boto3>=1.40.70",
# ]
# ///
"""Script to create test resources for log ingestor."""

# To allow using try-except-pass pattern to detect whether resources are already created.
# ruff: noqa: TRY300, S110, BLE001

import argparse
import json
import logging
import sys

import boto3

_ACCESS_KEY_ID = "test"
# To allow using hardcoded password
# ruff: noqa: S105
_SECRETE_ACCESS_KEY = "test"
_REGION: str = "us-east-1"

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(message)s",
    datefmt="%Y-%m-%d %H:%M:%S",
)
logger = logging.getLogger("create-log-ingestor-test-resources")


def main() -> int:
    """Main"""
    parser = argparse.ArgumentParser(description="Create test resources for log ingestor.")
    parser.add_argument(
        "--port",
        type=int,
        default=4566,
        help="The port LocalStack is hosting on (default: %(default)d)",
    )
    parser.add_argument(
        "--bucket",
        type=str,
        default="log-ingestor-bucket",
        help="The S3 bucket name to create (default: %(default)s)",
    )
    parser.add_argument(
        "--queue",
        type=str,
        default="log-ingestor-queue",
        help="The SQS queue name to create (default: %(default)s)",
    )
    args = parser.parse_args()

    localstack_endpoint = f"http://localhost:{args.port}"
    logger.info("Using LocalStack endpoint: %s", localstack_endpoint)

    session = boto3.session.Session(
        aws_access_key_id=_ACCESS_KEY_ID,
        aws_secret_access_key=_SECRETE_ACCESS_KEY,
        region_name=_REGION,
    )
    s3_client = session.client("s3", endpoint_url=localstack_endpoint)
    sqs_client = session.client("sqs", endpoint_url=localstack_endpoint)

    # Check whether the resources have already been created
    try:
        _ = s3_client.head_bucket(Bucket=args.bucket)
        logger.warning("Bucket '%s' already exists.", args.bucket)
        return 1
    except Exception as _:
        pass

    try:
        _ = sqs_client.get_queue_url(QueueName=args.queue)
        logger.warning("Queue '%s' already exists.", args.queue)
        return 1
    except Exception as _:
        pass

    # Create S3 bucket
    try:
        s3_client.create_bucket(Bucket=args.bucket)
    except Exception as _:
        logger.exception("Failed to create S3 bucket.")
        return 1
    logger.info("S3 bucket '%s' created successfully.", args.bucket)

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
        logger.exception("Failed to create S3 bucket.")
        return 1
    logger.info("SQS queue '%s' created successfully with arn '%s'.", args.queue, queue_arn)

    # Set SQS queue to listen to S3 bucket events (creation only)
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

        s3_notification_config = {
            "QueueConfigurations": [{"QueueArn": queue_arn, "Events": ["s3:ObjectCreated:*"]}]
        }
        s3_client.put_bucket_notification_configuration(
            Bucket=args.bucket, NotificationConfiguration=s3_notification_config
        )
    except Exception as _:
        logger.exception("Failed to set the queue to listen to the bucket.")
        return 1

    logger.info(
        "S3 bucket '%s' configured to send events to SQS queue '%s'.", args.bucket, args.queue
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
