# AWS S3

This guide explains how to configure and use CLP with AWS S3.

:::{note}
Currently, only [clp-json][release-choices] supports object storage. Support for `clp-text` will be
added in a future release.
:::

:::{note}
This guide focuses on AWS S3. For S3-compatible storage services (e.g., MinIO, Ceph), see the
[S3-compatible storage guide](../s3-compatible-storage.md).
:::

:::{tip}
If you're using object storage because the host(s) on which you're running CLP are ephemeral,
consider also using [external databases](../../guides-external-database.md) for metadata storage (to
ensure data persistence in case of host replacements).
:::

## Supported uses

[Table 1](#table-1) shows the supported uses of AWS S3 in CLP.

(table-1)=
::::{card}

:::{table}
:align: left

| Use case                                    | Supported                                               |
|---------------------------------------------|---------------------------------------------------------|
| Compress logs from S3-compatible storage    | <i class="fa fa-circle-check" style="color: green"></i> |
| Store archives on S3-compatible storage     | <i class="fa fa-circle-check" style="color: green"></i> |
| Cache stream files on S3-compatible storage | <i class="fa fa-circle-check" style="color: green"></i> |

:::

+++
**Table 1**: The supported uses of AWS S3 in CLP.
::::

:::{note}
You can choose to use AWS S3 for any combination of the three use cases above (e.g., compress logs
from AWS S3 and cache the stream files on AWS S3, but store archives on the local filesystem).
:::

## Prerequisites

1. This guide assumes you're able to configure, start, stop, and use CLP as described in the
   [clp-json quick-start guide](../../quick-start/clp-json.md).
2. An S3 bucket and [key prefix][aws-key-prefixes] containing the logs you wish to compress.
3. An S3 bucket and key prefix where you wish to store compressed archives.
4. An S3 bucket and key prefix where you wish to cache stream files.
5. A [supported AWS authentication method](#supported-aws-authentication-methods) configured with
   the necessary permissions to access the S3 buckets and prefixes mentioned above.

   :::{note}
   You may use a single authentication method for all the [use cases](#supported-uses) above, or a
   separate one for each.
   :::

### Supported AWS authentication methods

clp-json currently supports the AWS authentication methods described below.

:::{caution}
Short-term [STS credentials][aws-sts-credentials] (which include a Session Token) are not supported
directly. Instead, use [named profiles](#named-profiles) (with IAM Identity Center authentication or
IAM role assumption) which provide the required permissions and don't require specifying credentials
directly.
:::

#### Long-term IAM user credentials

clp-json can authenticate using long-term credentials for an IAM user.

* To create a user, follow [this guide][aws-create-iam-user].
  * You don't need to assign any groups or policies to the user at this stage since we will
    attach policies in later steps, depending on which object storage use cases you require.
* To generate the credentials, follow [this guide][aws-create-access-keys].
  * Choose the "Other" use case to generate long-term credentials.

#### Named profiles

clp-json can authenticate using AWS CLI named profiles. Named profiles can themselves make use of a
variety of AWS authentication mechanisms, including:

* [IAM Identity Center authentication][aws-iam-identity-center]
* [Assuming an IAM role][aws-iam-roles]
* Long-term IAM user credentials

Follow [this guide][aws-configure-profiles] for more information on configuring profiles with the
AWS CLI.

:::{note}
Profile configurations are stored in your AWS config directory (typically `~/.aws`).
:::

#### Environment variables for long-term credentials

clp-json can authenticate using [long-term IAM user credentials](#long-term-iam-user-credentials)
specified through the environment variables `AWS_ACCESS_KEY_ID` and `AWS_SECRET_ACCESS_KEY`.

#### EC2 instance IAM roles

clp-json can authenticate using IAM roles attached to an EC2 instance (that CLP is hosted on).

Follow [this guide][aws-ec2-attach-iam-role] to attach an IAM role to an instance.

## Configuration

The subsections below explain how to configure your AWS S3 bucket and CLP for each use case:

::::{grid} 1 1 1 1
:gutter: 2

:::{grid-item-card}
:link: aws-s3-config
Configuring AWS S3
^^^
Configuring your AWS S3 bucket and IAM permissions for each use case.
:::

:::{grid-item-card}
:link: clp-config
Configuring CLP
^^^
Configuring CLP to use AWS S3 for each use case.
:::
::::

## Using CLP with AWS S3

The subsection below explains how to use CLP with AWS S3 for each use case:

::::{grid} 1 1 1 1
:gutter: 2

:::{grid-item-card}
:link: using-clp-with-aws-s3
Using CLP with AWS S3
^^^
Using CLP to compress, search, and view log files from AWS S3.
:::
::::

:::{toctree}
:hidden:

aws-s3-config
clp-config
using-clp-with-aws-s3
:::

[aws-configure-profiles]: https://docs.aws.amazon.com/cli/latest/userguide/cli-configure-files.html
[aws-create-access-keys]: https://docs.aws.amazon.com/keyspaces/latest/devguide/create.keypair.html
[aws-create-iam-user]: https://docs.aws.amazon.com/IAM/latest/UserGuide/id_users_create.html
[aws-ec2-attach-iam-role]: https://docs.aws.amazon.com/AWSEC2/latest/UserGuide/attach-iam-role.html
[aws-iam-identity-center]: https://docs.aws.amazon.com/cli/latest/userguide/cli-configure-sso.html
[aws-iam-roles]: https://docs.aws.amazon.com/cli/latest/userguide/cli-configure-role.html
[aws-key-prefixes]: https://docs.aws.amazon.com/AmazonS3/latest/userguide/using-prefixes.html
[aws-sts-credentials]: https://docs.aws.amazon.com/IAM/latest/UserGuide/id_credentials_temp.html
[release-choices]: ../../quick-start/index.md#choosing-a-flavor
