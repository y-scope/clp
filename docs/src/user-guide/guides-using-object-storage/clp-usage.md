# Using CLP with object storage

To compress logs from S3, follow the steps in the section below. For all other operations, you
should be able to use CLP as described in the [quick start](../quick-start-overview.md) guide.

## Compressing logs from S3

To compress logs from S3, use the compress script as follows, replacing the fields in angle brackets
(`<>`) with the appropriate values:

```bash
sbin/compress.sh \
  --timestamp-key <timestamp-key> \
  <prefix>
```

* `<prefix>` is the prefix of all logs you wish to compress and must be relative to the prefix
  configured in the [logs-input s3_config][logs-input-s3-config].

:::{note}
Compressing from S3 only supports a single prefix but will compress any logs that have the given
prefix.

If you wish to compress a single log file, specify the entire path to the log file. However, if that
log file's path is a prefix of another log file's path, then both log files will be compressed
(e.g., with two files "logs/syslog" and "logs/syslog.1", a prefix like "logs/syslog" will cause
both logs to be compressed). This limitation will be addressed in a future release.
:::

[add-iam-policy]: https://docs.aws.amazon.com/IAM/latest/UserGuide/access_policies_manage-attach-detach.html#embed-inline-policy-console
[aws-region-codes]: https://docs.aws.amazon.com/AmazonRDS/latest/UserGuide/Concepts.RegionsAndAvailabilityZones.html#Concepts.RegionsAndAvailabilityZones.Availability
[compression-iam-policy]: ./object-storage-config.md#configuration-for-compression
[logs-input-s3-config]: ./clp-config.md#configuration-for-input-logs