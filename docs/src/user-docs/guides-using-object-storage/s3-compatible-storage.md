# S3-compatible storage

In addition to AWS S3, CLP supports S3-compatible object storage services such as MinIO and Ceph.

:::{note}
This guide covers S3-compatible storage specifically. For AWS S3, see the
[AWS S3 guide](aws-s3/index.md).
:::

## Supported uses

[Table 1](#table-1) shows the supported uses of S3-compatible storage in CLP.

(table-1)=
::::{card}

:::{table}
:align: left

| Use case                                    | Supported                                               |
|---------------------------------------------|---------------------------------------------------------|
| Compress logs from S3-compatible storage    | <i class="fa fa-circle-check" style="color: green"></i> |
| Store archives on S3-compatible storage     | <i class="fa fa-circle-check" style="color: green"></i> |
| Cache stream files on S3-compatible storage | <i class="fa fa-circle-xmark" style="color: red"></i>   |

:::

+++
**Table 1**: The supported uses of S3-compatible storage in CLP.
::::

:::{note}
Stream file caching on S3-compatible storage is not currently supported. Therefore, the UI cannot
view extracted streams stored on custom S3 endpoints. This limitation will be addressed in a future
release.
:::

:::{note}
CLP only supports authenticating with S3-compatible object storage using credentials. Other AWS
authentication methods (named profiles, environment variables, EC2 instance roles) are not supported
for S3-compatible storage.
:::

## Configuration

To use CLP with S3-compatible storage, you'll need to:

* [configure your S3-compatible storage](#configuring-your-s3-compatible-storage).
* configure CLP to:
  * [compress logs from S3-compatible storage](#configuring-clp-for-compression), and/or
  * [store archives on S3-compatible storage](#configuring-clp-for-archive-storage).

### Configuring your S3-compatible storage

Configure your S3-compatible storage service with bucket access policies equivalent to the
[AWS IAM policies](aws-s3/aws-s3-config.md) described for each use case:

* For compression: Read access (`GetObject`) and list access (`ListBucket`) to the bucket/prefix
  containing your logs.
* For archive storage: Read (`GetObject`), write (`PutObject`), delete (`DeleteObject`), and list
  (`ListBucket`) access to the bucket/prefix where archives will be stored.

The specific configuration steps depend on your S3-compatible storage service.

### Configuring CLP for compression

To configure CLP to compress logs from S3-compatible storage, update the `logs_input` key in
`<package>/etc/clp-config.yaml` with the values below, replacing the fields in angle brackets:
(`<>`) with the appropriate values:

```yaml
logs_input:
  type: "s3"
  aws_authentication:
    type: "credentials"
    credentials:
      access_key_id: "<access-key-id>"
      secret_access_key: "<secret-access-key>"
```

* `<access-key-id>` and `<secret-access-key>` are the credentials for accessing your S3-compatible
  storage service.

:::{note}
CLP will automatically determine the appropriate endpoint URL from the object URLs you provide
during compression.
:::

### Configuring CLP for archive storage

To configure CLP to store archives on S3-compatible storage, update the `archive_output.storage` key
in `<package>/etc/clp-config.yaml` with the values below, replacing the fields in angle brackets
(`<>`) with the appropriate values:

```yaml
archive_output:
  storage:
    type: "s3"
    staging_directory: "var/data/staged-archives"  # Or a path of your choosing
    s3_config:
      endpoint_url: "<endpoint-url>"
      bucket: "<bucket-name>"
      key_prefix: "<key-prefix>"
      aws_authentication:
        type: "credentials"
        credentials:
          access_key_id: "<access-key-id>"
          secret_access_key: "<secret-access-key>"

  # archive_output's other config keys
```

* `staging_directory` is the local filesystem directory where archives will be temporarily stored
  before being uploaded.
* `<endpoint-url>` is the endpoint URL of your S3-compatible storage service (e.g.,
  `http://10.0.0.1:9000` for a local MinIO instance).
* `<bucket-name>` is the bucket's name.
* `<key-prefix>` is the "directory" where all archives will be stored within the bucket and must end
  with a trailing forward slash (e.g., `archives/`).
* `<access-key-id>` and `<secret-access-key>` are the credentials for accessing your S3-compatible
  storage service.

## Compressing logs from S3-compatible storage

To compress logs from S3-compatible storage, use the `sbin/compress-from-s3.sh` script with
"path-style" object URLs:

```text
http://<host>:<port>/<bucket-name>/<object-key>
```

or for key prefix mode:

```text
http://<host>:<port>/<bucket-name>/<key-prefix>
```

```{code-block} bash
:caption: Example: Compressing specific objects

sbin/compress-from-s3.sh \
  --timestamp-key @timestamp \
  --dataset default \
  s3-object \
  http://10.0.0.1:9000/bucket/logs/app.log
```

```{code-block} bash
:caption: Example: Compressing all objects under a prefix

sbin/compress-from-s3.sh \
  --timestamp-key timestamp \
  --dataset default \
  s3-key-prefix \
  http://10.0.0.1:9000/bucket/logs/
```
