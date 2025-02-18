# Using CLP with object storage

To compress logs from S3, follow the steps in the section below. For all other operations, you
should be able to use CLP as described in the [quick start](../quick-start-overview.md) guide.

## Compressing logs from S3

To compress logs from S3, use the `s3` subcommand as follows, replacing the fields in angle brackets
(`<>`) with the appropriate values:

```bash
sbin/compress.sh \
  s3 \
  --aws-credentials-file <credentials-file> \
  --timestamp-key <timestamp-key> \
  https://<bucket-name>.s3.<region-code>.amazonaws.com/<prefix>
```

* `<credentials-file>` is the path to an AWS credentials file like the following:

  ```ini
  [default]
  aws_access_key_id = <aws-access-key-id>
  aws_secret_access_key = <aws-secret-access-key>
  ```

  * CLP expects the credentials to be in the `default` section.
  * `<aws-access-key-id>` and `<aws-secret-access-key>` are the access key ID and secret access
    key of the CLP IAM user.
  * If you don't want to use a credentials file, you can specify the credentials on the command
    line using the `--aws-access-key-id` and `--aws-secret-access-key` flags (note that this may
    expose your credentials to other users running on the system).

* `<timestamp-key>` is the field path of the kv-pair that contains the timestamp in each log event.
* `<bucket-name>` is the name of the S3 bucket containing your logs.
* `<region-code>` is the AWS region [code][aws-region-codes] for the S3 bucket containing your logs.
* `<prefix>` is the prefix of all logs you wish to compress and must begin with the
  `<all-logs-prefix>` value from the [compression IAM policy][compression-iam-policy].

:::{note}
The `s3` subcommand only supports a single URL but will compress any logs that have the given
prefix.

If you wish to compress a single log file, specify the entire path to the log file. However, if that
log file's path is a prefix of another log file's path, then both log files will be compressed
(e.g., with two files "logs/syslog" and "logs/syslog.1", a prefix like "logs/syslog" will cause
both logs to be compressed). This limitation will be addressed in a future release.
:::

[add-iam-policy]: https://docs.aws.amazon.com/IAM/latest/UserGuide/access_policies_manage-attach-detach.html#embed-inline-policy-console
[aws-region-codes]: https://docs.aws.amazon.com/AmazonRDS/latest/UserGuide/Concepts.RegionsAndAvailabilityZones.html#Concepts.RegionsAndAvailabilityZones.Availability
[compression-iam-policy]: ./object-storage-config.md#configuration-for-compression