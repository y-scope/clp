# Using object storage

CLP can both compress logs from object storage (e.g., S3) and store archives on object storage. This
guide explains how to configure CLP for both use cases.

:::{note}
Currently, only the [clp-json][release-choices] release supports object storage. Support for
clp-text will be added in a future release.
:::

:::{note}
Currently, CLP only supports using S3 as object storage. Support for other object storage services
will be added in a future release.
:::

## Compressing logs from object storage

To compress logs from S3, use the `s3` subcommand of the `compress.sh` script:

```bash
sbin/compress.sh s3 s3://<bucket-name>/<path-prefix>
```

* `<bucket-name>` is the name of the S3 bucket containing your logs.
* `<path-prefix>` is the path prefix of all logs you wish to compress.

:::{note}
The `s3` subcommand only supports a single URL but will compress any logs that have the given path
prefix.

If you wish to compress a single log file, specify the entire path to the log file. However, if that
log file's path is a prefix of another log file's path, then both log files will be compressed. This
limitation will be addressed in a future release.
:::

## Storing archives on object storage

To configure CLP to store archives on S3, update the `archive_output.storage` key in
`<package>/etc/clp-config.yml`:

```yaml
archive_output:
  storage:
    type: "s3"
    staging_directory: "var/data/staged-archives"  # Or a path of your choosing
    s3_config:
      region: "<aws-region-code>"
      bucket: "<s3-bucket-name>"
      key-prefix: "<s3-key-prefix>"
      credentials:
        access_key_id: "<aws-access-key-id>"
        secret_access_key: "<aws-secret-access-key>"

  # archive_output's other config keys
```

* `s3_config` configures both the S3 bucket where archives should be stored as well as credentials
  for accessing it.
  * `<aws-region-code>` is the AWS region [code][aws-region-codes] for the bucket.
  * `<s3-bucket-name>` is the bucket's name.
  * `<s3-key-prefix>` is the "directory" where all archives will be stored within the bucket and
    must end with `/`.
  * `credentials` contains the S3 credentials necessary for accessing the bucket.

To configure CLP to be able to view compressed log files from S3, you'll need to configure a bucket
where CLP can store intermediate files that the log viewer can open. To do so, update the
`stream_output.storage` key in `<package>/etc/clp-config.yml`:

```yaml
stream_output:
  storage:
    type: "s3"
    staging_directory: "var/data/staged-streams"  # Or a path of your choosing
    s3_config:
      region: "<aws-region-code>"
      bucket: "<s3-bucket-name>"
      key-prefix: "<s3-key-prefix>"
      credentials:
        access_key_id: "<aws-access-key-id>"
        secret_access_key: "<aws-secret-access-key>"

  # stream_output's other config keys
```

The configuration keys above function identically to those in `archive_output.storage`, except they
should be configured to use a different S3 path (i.e., a different key-prefix in the same bucket or
a different bucket entirely).

:::{note}
To view compressed log files, clp-text currently converts them into IR streams that the log viewer
can open, while clp-json converts them into JSONL streams. These streams only need to be stored for
as long as the streams are being viewed in the viewer, however CLP currently doesn't explicitly
delete the streams. This limitation will be addressed in a future release.
:::

[aws-region-codes]: https://docs.aws.amazon.com/AmazonRDS/latest/UserGuide/Concepts.RegionsAndAvailabilityZones.html#Concepts.RegionsAndAvailabilityZones.Availability
[release-choices]: http://localhost:8080/user-guide/quick-start-cluster-setup/index.html#choosing-a-release
