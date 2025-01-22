# Using object storage

CLP can:

* compress logs from object storage (e.g., S3);
* store archives on object storage; and
* view the compressed logs from object storage.

This guide explains how to configure CLP for all three use cases. Note that you can choose to use
object storage for any combination of the three use cases (e.g., compress logs from S3 and view the
compressed logs from S3, but store archives on the local filesystem).

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
    * An S3 URL is a combination of a bucket name and a key prefix as shown below:
      
      :::{mermaid}
      %%{
        init: {
          "theme": "base",
          "themeVariables": {
            "primaryColor": "#0066cc",
            "primaryTextColor": "#fff",
            "primaryBorderColor": "transparent",
            "lineColor": "#9580ff",
            "secondaryColor": "#9580ff",
            "tertiaryColor": "#fff"
          }
        }
      }%%
      graph TD
        A["s3://my-bucket-name/my-logs-dir/"] --"Bucket name"--> B[my-bucket-name]
        A --"Key prefix"--> C[path/to/my/file.txt]
      :::

2. An S3 bucket and key prefix where you wish to store compressed archives.
3. An S3 bucket and key prefix where you wish to store intermediate files for viewing compressed
   logs.
4. An AWS IAM user with the necessary permissions to access the S3 prefixes mentioned above.
    * To create a user, follow [this guide][aws-create-iam-user].
    * You may use a different IAM user for each use case to follow the
      [principle of least privilege][least-privilege-principle], or you can use the same user for
      all three.
    * For brevity, we'll refer to this user as the "CLP IAM user" in the rest of this guide.

:::{note}
CLP currently requires IAM user (long-term) credentials to access the relevant S3 buckets. Support
for other authentication methods (e.g., temporary credentials) will be added in a future release.
:::

## Use cases

The following subsections below explain how to set up each use case:

::::{grid} 1 1 1 1
:gutter: 2

:::{grid-item-card}
:link: compress
Compression
^^^
Compressing logs from object storage
:::

:::{grid-item-card}
:link: archive-storage
Archive storage
^^^
Storing archives on object storage
:::

:::{grid-item-card}
:link: stream-storage
Stream storage
^^^
Viewing compressed logs from object storage
:::
::::

:::{toctree}
:hidden:

compress
archive-storage
stream-storage
:::

[add-iam-policy]: https://docs.aws.amazon.com/IAM/latest/UserGuide/access_policies_manage-attach-detach.html#embed-inline-policy-console
[aws-cors-guide]: https://docs.aws.amazon.com/AmazonS3/latest/userguide/enabling-cors-examples.html
[aws-create-iam-user]: https://docs.aws.amazon.com/IAM/latest/UserGuide/id_users_create.html
[aws-key-prefixes]: https://docs.aws.amazon.com/AmazonS3/latest/userguide/using-prefixes.html
[aws-region-codes]: https://docs.aws.amazon.com/AmazonRDS/latest/UserGuide/Concepts.RegionsAndAvailabilityZones.html#Concepts.RegionsAndAvailabilityZones.Availability
[least-privilege-principle]: https://en.wikipedia.org/wiki/Principle_of_least_privilege
[release-choices]: ../quick-start-cluster-setup/index.md#choosing-a-release
