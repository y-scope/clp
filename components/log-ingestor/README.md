# CLP Log Ingestor

The CLP Log Ingestor is a Rust-based server designed to efficiently manage and execute long-running
log ingestion jobs.

## Testing


### Quick Start

All test cases in this component will be called through the Rust test task:

```bash
task tests:rust-all
```

### Manual Testing

To run tests that don't require AWS services:

```bash
cargo nextest run --all-features
```

By default, all tests that require AWS service will be ignored. To run those tests, use the following command to start
a LocalStack container before calling the test runner.

```bash
export REPO_ROOT=$(git rev-parse --show-toplevel)
# Start a LocalStack container named `clp-log-ingestor-tests`
$REPO_ROOT/tools/scripts/localstack/start.py --name clp-log-ingestor-test

# Create an S3 bucket named `clp-log-ingestor-test-bucket` and an SQS queue named `clp-log-ingestor-test-queue`
# listening to this bucket
$REPO_ROOT/tools/scripts/localstack/create-bucket.py \
  --bucket clp-log-ingestor-test-bucket \
  --queue clp-log-ingestor-test-queue

# Define environment variables required for the tests
export CLP_LOG_INGESTOR_S3_BUCKET="clp-log-ingestor-test-bucket"
export CLP_LOG_INGESTOR_SQS_QUEUE="clp-log-ingestor-test-queue"

# Run all tests
cargo nextest run --all-features --run-ignored all

# Stop and remove the LocalStack container after tests are done
$REPO_ROOT/tools/scripts/localstack/stop.py --name clp-log-ingestor-test
```

For advanced configuration options and AWS service testing details, see the [AWS Config](tests/aws_config.rs) module.
