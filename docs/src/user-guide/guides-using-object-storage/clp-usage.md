# Using CLP with object storage

To compress logs from S3, follow the steps in the section below. For all other operations, you
should be able to use CLP as described in the [clp-json quick-start guide](../quick-start/clp-json).

## Compressing logs from S3

To compress logs from S3, use the `sbin/compress.sh` script as follows, replacing the fields in
angle brackets (`<>`) with the appropriate values:

```bash
sbin/compress.sh \
  --timestamp-key <timestamp-key> \
  <url>
```

* `<url>` is a URL identifying the logs to compress. It can have one of two formats:
  * `https://<bucket-name>.s3.<region-code>.amazonaws.com/<prefix>`
  * `https://s3.<region-code>.amazonaws.com/<bucket-name>/<prefix>`
* The fields in `<url>` are as follows:
  * `<bucket-name>` is the name of the S3 bucket containing your logs.
  * `<region-code>` is the AWS region [code][aws-region-codes] for the S3 bucket containing your
    logs.
  * `<prefix>` is the prefix of all logs you wish to compress and must begin with the
    `<all-logs-prefix>` value from the [compression IAM policy][compression-iam-policy].

:::{note}
Compressing from S3 only supports a single URL but will compress any logs that have the given
prefix.

If you wish to compress a single log file, specify the entire path to the log file. However, if
that log file's path is a prefix of another log file's path, then both log files will be compressed
(e.g., with two files "logs/syslog" and "logs/syslog.1", a prefix like "logs/syslog" will cause
both logs to be compressed). This limitation will be addressed in a future release.
:::

[add-iam-policy]: https://docs.aws.amazon.com/IAM/latest/UserGuide/access_policies_manage-attach-detach.html#embed-inline-policy-console
[aws-region-codes]: https://docs.aws.amazon.com/AmazonRDS/latest/UserGuide/Concepts.RegionsAndAvailabilityZones.html#Concepts.RegionsAndAvailabilityZones.Availability
[compression-iam-policy]: ./object-storage-config.md#configuration-for-compression
