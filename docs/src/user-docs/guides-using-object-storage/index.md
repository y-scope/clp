# Using object storage

CLP can:

* compress logs from object storage (e.g., S3 and S3-compatible storage services);
* store archives on object storage; and
* cache stream files (used for viewing compressed logs) on object storage.

:::{note}
Currently, only [clp-json][release-choices] supports object storage. Support for `clp-text` will be
added in a future release.
:::

:::{tip}
If you're using object storage because the host(s) on which you're running CLP are ephemeral,
consider also using [external databases](../guides-external-database.md) for metadata storage (to
ensure data persistence in case of host replacements).
:::

## Guides

Choose the guide that matches your object storage service:

::::{grid} 1 1 2 2
:gutter: 2

:::{grid-item-card}
:link: aws-s3/index
AWS S3
^^^
Using CLP with Amazon Web Services S3.
:::

:::{grid-item-card}
:link: s3-compatible-storage
S3-compatible storage
^^^
Using CLP with S3-compatible storage services (e.g., MinIO, Ceph).
:::
::::

:::{toctree}
:hidden:

aws-s3/index
s3-compatible-storage
:::

[release-choices]: ../quick-start/index.md#choosing-a-flavor
