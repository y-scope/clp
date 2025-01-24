# Using object storage

CLP can:

* compress logs from object storage (e.g., S3);
* store archives on object storage; and
* cache stream files (used for viewing compressed logs) on object storage.

This guide explains how to configure and use CLP for all three use cases. Note that you can choose
to use object storage for any combination of the three use cases (e.g., compress logs from S3 and
cache the stream files on S3, but store archives on the local filesystem).

:::{note}
Currently, only the [clp-json][release-choices] release supports object storage. Support for
`clp-text` will be added in a future release.
:::

:::{note}
Currently, CLP only supports using S3 as object storage. Support for other object storage services
will be added in a future release.
:::

## Prerequisites

1. This guide assumes you're able to configure, start, stop, and use a CLP cluster as described in
   the [quick-start guide](../quick-start-overview.md).
2. An S3 bucket and [key prefix][aws-key-prefixes] containing the logs you wish to compress.
3. An S3 bucket and key prefix where you wish to store compressed archives.
4. An S3 bucket and key prefix where you wish to cache stream files.
5. An AWS IAM user with the necessary permissions to access the S3 bucket(s) and prefixes mentioned
   above.
    * To create a user, follow [this guide][aws-create-iam-user].
      * You don't need to assign any groups or policies to the user at this stage since we will
        attach policies in later steps, depending on which object storage use cases you require.
    * You may use a single IAM user for all use cases, or a separate one for each.
    * For brevity, we'll refer to this user as the "CLP IAM user" in the rest of this guide.
6. IAM user (long-term) credentials for the IAM user(s) created in step (4) above.
    * To create these credentials, follow [this guide][aws-create-access-keys].
      * Choose the "Other" use case to generate long-term credentials.

    :::{note}
    CLP currently requires IAM user (long-term) credentials to access the relevant S3 buckets.
    Support for other authentication methods (e.g., temporary credentials) will be added in a future
    release.
    :::

## Configuration

The subsections below explain how to configure your object storage bucket and CLP for each use case:

::::{grid} 1 1 1 1
:gutter: 2

:::{grid-item-card}
:link: object-storage-config
Configuring object storage
^^^
Configuring your object storage bucket for each use case.
:::

:::{grid-item-card}
:link: clp-config
Configuring CLP
^^^
Configuring CLP to use object storage for each use case.
:::
::::

## Using CLP with object storage

The subsection below explains how to use CLP with object storage for each use case:

::::{grid} 1 1 1 1
:gutter: 2

:::{grid-item-card}
:link: clp-usage
Using CLP with object storage
^^^
Using CLP to compress, search, and view log files from object storage.
:::
::::

:::{toctree}
:hidden:

object-storage-config
clp-config
clp-usage
:::

[aws-create-access-keys]: https://docs.aws.amazon.com/keyspaces/latest/devguide/create.keypair.html
[aws-create-iam-user]: https://docs.aws.amazon.com/IAM/latest/UserGuide/id_users_create.html
[aws-key-prefixes]: https://docs.aws.amazon.com/AmazonS3/latest/userguide/using-prefixes.html
[release-choices]: ../quick-start-cluster-setup/index.md#choosing-a-release
