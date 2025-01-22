# Using object storage

CLP can:

* compress logs from object storage (e.g., S3);
* store archives on object storage; and
* cache stream files (used for viewing compressed logs) on object storage.

This guide explains how to configure CLP for all three use cases. Note that you can choose to use
object storage for any combination of the three use cases (e.g., compress logs from S3 and cache the
stream files on S3, but store archives on the local filesystem).

:::{note}
Currently, only the [clp-json][release-choices] release supports object storage. Support for
`clp-text` will be added in a future release.
:::

:::{note}
Currently, CLP only supports using S3 as object storage. Support for other object storage services
will be added in a future release.
:::

## Prerequisites

1. An S3 bucket and [key prefix][aws-key-prefixes] containing the logs you wish to compress.
2. An S3 bucket and key prefix where you wish to store compressed archives.
3. An S3 bucket and key prefix where you wish to cache stream files.
4. An AWS IAM user with the necessary permissions to access the S3 prefixes mentioned above.
    * To create a user, follow [this guide][aws-create-iam-user].
    * You may use a different IAM user for each use case to follow the
      [principle of least privilege][least-privilege-principle], or you can use the same user for
      all three.
    * For brevity, we'll refer to this user as the "CLP IAM user" in the rest of this guide.
5. IAM user (long-term) credentials for the IAM user(s) created in step (4) above.
    * To create these credentials, follow [this guide][aws-create-access-keys].

    :::{note}
    CLP currently requires IAM user (long-term) credentials to access the relevant S3 buckets.
    Support for other authentication methods (e.g., temporary credentials) will be added in a future
    release.
    :::

## Use cases

The following subsections below explain how to set up each use case:

::::{grid} 1 1 1 1
:gutter: 2

:::{grid-item-card}
:link: compress
Compressing logs
^^^
Compressing logs from object storage
:::

:::{grid-item-card}
:link: archive-storage
Storing archives
^^^
Storing archives on object storage
:::

:::{grid-item-card}
:link: stream-storage
Caching stream files
^^^
Caching stream files on object storage
:::
::::

:::{toctree}
:hidden:

compress
archive-storage
stream-storage
:::

[aws-create-access-keys]: https://docs.aws.amazon.com/keyspaces/latest/devguide/create.keypair.html
[aws-create-iam-user]: https://docs.aws.amazon.com/IAM/latest/UserGuide/id_users_create.html
[aws-key-prefixes]: https://docs.aws.amazon.com/AmazonS3/latest/userguide/using-prefixes.html
[least-privilege-principle]: https://en.wikipedia.org/wiki/Principle_of_least_privilege
[release-choices]: ../quick-start-cluster-setup/index.md#choosing-a-release
