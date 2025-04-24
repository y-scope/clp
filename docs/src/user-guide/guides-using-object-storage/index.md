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
5. An AWS authentication method configured with the necessary permissions to access the
  S3 bucket(s) and prefixes mentioned above. Authentication methods include:
    * Long-term IAM user credentials.
      * To create a user, follow [this guide][aws-create-iam-user].
        * You don't need to assign any groups or policies to the user at this stage since we will
          attach policies in later steps, depending on which object storage use cases you require.
      * To generate the credentials, follow [this guide][aws-create-access-keys].
        * Choose the "Other" use case to generate long-term credentials.
    * A named profile configured by the AWS CLI in your AWS config directory (typically `~/.aws`).
      * Profiles may make use of a variety of AWS authentication mechanisms, including:
        * [IAM Identity Center][aws-iam-identity-center].
        * [Assuming an IAM role][aws-iam-roles].
        * Long-term IAM user credentials.
      * Follow [this guide][aws-configure-profiles] for more information on configuring profiles
        with the AWS CLI.
    * Long-term AWS credentials set as environment variables.
      * Set the `AWS_ACCESS_KEY_ID` and `AWS_SECRET_ACCESS_KEY` environmental variables
        with the corresponding credential keys.
    * An IAM role attached to an EC2 instance (that CLP is hosted on).

    :::{note}
    You may use a single authentication method for all use cases, or a separate one for each.
    :::

    :::{caution}
    Short-term [STS credentials][aws-sts-credentials] (that include a Session Token) are not
    supported directly. To use temporary credentials, use named profiles with IAM Identity Center
    or role assumption to generate them dynamically instead.
    :::

## Configuration

The subsections below explain how to configure your object storage bucket and CLP for each use case:

::::{grid} 1 1 1 1
:gutter: 2

:::{grid-item-card}
:link: object-storage-config
Configuring object storage
^^^
Configuring your object storage bucket and IAM permissions for each use case.
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

[aws-configure-profiles]: https://docs.aws.amazon.com/cli/v1/userguide/cli-configure-files.html
[aws-create-access-keys]: https://docs.aws.amazon.com/keyspaces/latest/devguide/create.keypair.html
[aws-create-iam-user]: https://docs.aws.amazon.com/IAM/latest/UserGuide/id_users_create.html
[aws-iam-identity-center]: https://docs.aws.amazon.com/cli/latest/userguide/cli-configure-sso.html
[aws-iam-roles]: https://docs.aws.amazon.com/cli/latest/userguide/cli-configure-role.html
[aws-key-prefixes]: https://docs.aws.amazon.com/AmazonS3/latest/userguide/using-prefixes.html
[aws-sts-credentials]: https://docs.aws.amazon.com/IAM/latest/UserGuide/id_credentials_temp.html
[release-choices]: ../quick-start-cluster-setup/index.md#choosing-a-release
