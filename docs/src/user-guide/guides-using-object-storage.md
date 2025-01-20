# Using object storage

CLP can both compress logs from object storage (e.g., S3) and store archives on object storage. This
guide explains how to configure CLP for both use cases.

:::{note}
Currently, only the [clp-json][release-choices] release supports object storage. Support for clp-text will be added
in a future release.
:::

:::{note}
Currently, CLP only supports using S3 as object storage. Support for other object storage services
will be added in a future release.
:::

# Compressing logs from object storage

To ingest logs from S3, you can use the `s3` subcommand of the `compress.sh` script:

```bash
sbin/compress.sh s3 s3://<bucket-name>/<path-prefix>
```

* `<bucket-name>` is the name of the S3 bucket containing your logs.
* `<path-prefix>` is the path prefix of all logs you wish to compress.

# Storing archives on object storage

[release-choices]: http://localhost:8080/user-guide/quick-start-cluster-setup/index.html#choosing-a-release
