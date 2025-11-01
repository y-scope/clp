# Using CLP with object storage

To compress logs from S3, follow the steps in the section below. For all other operations, you
should be able to use CLP as described in the [clp-json quick-start guide](../quick-start/clp-json).

## Compressing logs from S3

To compress logs from S3, use the `sbin/compress-from-s3.sh` script. The script supports two modes
of operation:

* [**s3-object** mode](#s3-object-compression-mode): Compress S3 objects specified by their full
  S3 URLs.
* [**s3-key-prefix** mode](#s3-key-prefix-compression-mode): Compress all S3 objects under a given
  S3 key prefix.

### `s3-object` compression mode

The `s3-object` mode allows you to specify individual S3 objects to compress by using their full
URLs. To use this mode, call the `sbin/compress-from-s3.sh` script as follows, and replace the
fields in angle brackets (`<>`) with the appropriate values:

```bash
sbin/compress-from-s3.sh \
  --timestamp-key <timestamp-key> \
  --dataset <dataset-name> \
  s3-object \
  <object-url> [<object-url> ...]
```

* `<object-url>` is a URL identifying the S3 object to compress. It can be written in either of two
  formats:
  * `https://<bucket-name>.s3.<region-code>.amazonaws.com/<object-key>`
  * `https://s3.<region-code>.amazonaws.com/<bucket-name>/<object-key>`
* The fields in `<object-url>` are as follows:
  * `<bucket-name>` is the name of the S3 bucket containing your logs.
  * `<region-code>` is the AWS region [code][aws-region-codes] for the S3 bucket containing your
    logs.
  * `<object-key>` is the [object key][aws-s3-object-key] of the log file object you wish to
    compress.

    :::{warning}
    There must be no duplicate object keys across all `<object-url>` arguments.
    :::


* For a description of other fields, see the [clp-json quick-start
  guide](../quick-start/clp-json.md#compressing-json-logs).

Instead of specifying input object URLs explicitly in the command, you may specify them in a text
file and then pass the file into the command using the `--inputs-from` flag, like so:

```bash
sbin/compress-from-s3.sh \
  --timestamp-key <timestamp-key> \
  --dataset <dataset-name> \
  s3-object \
  --inputs-from <input-file>
```

* `<input-file>` is a path to a text file containing one S3 object URL **per line**. The URLs must
  follow the same format as described above for `<object-url>`.

:::{note}
The `s3-object` mode requires the input object keys to share a non-empty common prefix. If the input
object keys do not share a common prefix, they will be rejected and no compression job will be
created. This limitation will be addressed in a future release.
:::

### `s3-key-prefix` compression mode

The `s3-key-prefix` mode allows you to compress all objects under a given S3 key prefix. To use this
mode, call the `sbin/compress-from-s3.sh` script as follows, and replace the fields in angle
brackets (`<>`) with the appropriate values:

```bash
sbin/compress-from-s3.sh \
  --timestamp-key <timestamp-key> \
  --dataset <dataset-name> \
  s3-key-prefix \
  <key-prefix-url>
```

* `<key-prefix-url>` is a URL identifying the S3 key prefix to compress. It can be written in either
  of two formats:
  * `https://<bucket-name>.s3.<region-code>.amazonaws.com/<key-prefix>`
  * `https://s3.<region-code>.amazonaws.com/<bucket-name>/<key-prefix>`
* The fields in `<key-prefix-url>` are as follows:
  * `<bucket-name>` is the name of the S3 bucket containing your logs.
  * `<region-code>` is the AWS region [code][aws-region-codes] for the S3 bucket containing your
    logs.
  * `<key-prefix>` is the prefix of all logs you wish to compress and must begin with the
    `<all-logs-prefix>` value from the [compression IAM policy][compression-iam-policy].

:::{note}
`s3-key-prefix` mode only accepts a single `<key-prefix-url>` argument. This limitation will be
addressed in a future release.
:::

[aws-region-codes]: https://docs.aws.amazon.com/AmazonRDS/latest/UserGuide/Concepts.RegionsAndAvailabilityZones.html#Concepts.RegionsAndAvailabilityZones.Availability
[aws-s3-object-key]: https://docs.aws.amazon.com/AmazonS3/latest/userguide/object-keys.html
[compression-iam-policy]: ./object-storage-config.md#configuration-for-compression
